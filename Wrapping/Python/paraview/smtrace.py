"""smtrace module is used along with vtkSMTrace to generate Python trace for
ParaView. While this module is primarily designed to be used from the ParaView
GUI, Python scripts can use this module too to generate trace from the script
executed.

Typical usage is as follows::

    from paraview import smtrace

    tracer = ScopedTracer()
    with tracer:
      # configure the tracer for this scope
      tracer.config.SetFullyTraceSupplementalProxies = True
      # do actions to trace
      # ...

    txt = tracer.last_trace()

=========================
Developer Documentation
=========================

This section describes the module design for developers wanted to extend this
module or use this module for advance tracing/state generation.

The design can be described as follows:

C++ code (either in ServerManager or the GUI layer) should trace actions.
This is done using SM_SCOPED_TRACE() macro provided by vtkSMTrace. When tracing
is enabled, each SM_SCOPED_TRACE() call creates a :class:`.TraceItem`. The TraceItem
instance is scoped, i.e. the object is finalized and destroyed when the scope
exits.

There are various types of TraceItem, ranging from those that trace specific
action such as :class:`.Show`, or those that trace any modified properties
(:class:`.PropertiesModified`). Generic TraceItem types, such as
:class:`.CallMethod` and :class:`.CallFunction` can be used to trace methods
called on vtkObject instances or functions in the global namespace.

TraceItems create or use :class:`.Accessor` instances. Accessors are objects
created for Proxies and Properties in ParaView. Accessor knows how to access
that proxy or property in the Python trace. TraceItems that create new proxies
such as :class:`.RegisterPipelineProxy` and :class:`.RegisterViewProxy`, create
new :class:`.ProxyAccessor` instances. Other such as
:class:`.PropertiesModified` trace item rely on accessors already created.
:class:`.Trace` can provide access to already created accessor as well as create
new accessor for proxies create before the tracing began
(:meth:`.Trace.get_accessor`).

Additionally, there are filters such as :class:`.ProxyFilter`,
:class:`.PipelineProxyFilter`, etc. which are used to filter properties that get
traced and where they get traced i.e. in constructor call or right after it.

===============================
Notes about references
===============================

RealProxyAccessor keeps a hard reference to the servermanager.Proxy instance.
This is required. If we don't, then the Python object for vtkSMProxy also gets
garbage collected since there's no reference to it.

"""
from __future__ import absolute_import, division, print_function

import math
import weakref
import paraview.servermanager as sm
import paraview.simple as simple
from paraview.vtk import vtkTimeStamp
from paraview.modules.vtkRemotingCore import vtkPVSession
from paraview.decorator_utils import should_trace_based_on_decorators

def _get_skip_rendering():
    return sm.vtkSMTrace.GetActiveTracer().GetSkipRenderingComponents()


def _pretty_print(object, indent=4, width=80, prefix=""):
    from pprint import pformat
    offset = len(prefix)
    txt = pformat(object, indent=indent, width=(width - offset))

    # now lets indent all of the lines based on prefix length
    spaces = "\n" + " " * offset
    return prefix + spaces.join(txt.split("\n"))


class TraceOutput:
    """Internal class used to collect the trace output. Everytime anything is pushed into
    this using the append API, we ensure that the trace is updated. Trace
    doesn't put commands to the trace-output as soon as modifications are noticed
    to try to consolidate the state changes."""

    def __init__(self, data=None):
        self.__data = []
        self.append(data) if data else None

    def append(self, data):
        if isinstance(data, list):
            self.__data += data
            # print ("\n".join(data),"\n")
        elif isinstance(data, str):
            self.__data.append(data)
            # print (data,"\n")

    def append_separator(self):
        try:
            self.__data.append("") if self.__data[-1] != "" else None
        except IndexError:
            pass

    def append_separated(self, data):
        self.append_separator()
        self.append(data)

    def __str__(self):
        return '\n'.join(self.__data)

    def raw_data(self):
        return self.__data

    def reset(self):
        self.__data = []


class Trace(object):
    __REGISTERED_ACCESSORS = {}

    Output = None

    @classmethod
    def reset(cls):
        """Resets the Output and clears all register accessors."""
        cls.__REGISTERED_ACCESSORS.clear()
        cls.Output = TraceOutput()

    @classmethod
    def get_registered_name(cls, proxy, reggroup):
        """Returns the registered name for `proxy` in the given `reggroup`."""
        return proxy.SMProxy.GetSessionProxyManager().GetProxyName(reggroup, proxy.SMProxy)

    @classmethod
    def get_varname(cls, name):
        """returns a unique variable name given a suggested variable name. If
        the suggested variable name is already taken, this method will try to
        find a good suffix that's available."""
        name = sm._make_name_valid(name)
        name = name[0].lower() + name[1:]
        original_name = name
        suffix = 1
        # build a set of existing variable names
        varnameset = set([accessor.Varname for accessor in cls.__REGISTERED_ACCESSORS.values()])
        while name in varnameset:
            name = "%s_%d" % (original_name, suffix)
            suffix += 1
        return name

    @classmethod
    def register_accessor(cls, accessor):
        """Register an instance of an Accessor or subclass"""
        cls.__REGISTERED_ACCESSORS[accessor.get_object()] = accessor

    @classmethod
    def unregister_accessor(cls, accessor):
        if accessor.get_object() in cls.__REGISTERED_ACCESSORS:
            del cls.__REGISTERED_ACCESSORS[accessor.get_object()]

    @classmethod
    def get_accessor(cls, obj, selector=None):
        """Returns an accessor for obj. If none exists, a new one may be
        created, if possible. Currently obj is expected to be a
        :class:`servermanager.Proxy` instance. In the future, we may change this to
        be a vtkSMProxy instance instead."""
        if obj is None:
            return None
        assert isinstance(obj, sm.Proxy)
        try:
            return cls.__REGISTERED_ACCESSORS[obj]
        except KeyError:
            # Create accessor if possible else raise
            # "untraceable" exception.
            if cls.create_accessor(obj, selector):
                return cls.__REGISTERED_ACCESSORS[obj]
            # return "<unknown>"
            raise Untraceable(
                "%s is not 'known' at this point. Hence, we cannot trace " \
                "it. Skipping this action." % repr(obj))

    @classmethod
    def has_accessor(cls, obj):
        return obj in cls.__REGISTERED_ACCESSORS

    @classmethod
    def create_accessor(cls, obj, selector=None):
        """Create a new accessor for a proxy. This returns True when a
        ProxyAccessor has been created, other returns False. This is needed to
        bring into trace proxies that were either already created when the trace
        was started or were created indirectly and hence not explicitly traced."""
        skip_rendering = _get_skip_rendering()

        if isinstance(obj, sm.SourceProxy):
            # handle pipeline source/filter proxy.
            pname = cls.get_registered_name(obj, "sources")
            if pname:
                if obj == simple.GetActiveSource():
                    accessor = ProxyAccessor(cls.get_varname(pname), obj)
                    cls.Output.append_separated([ \
                        "# get active source.",
                        "%s = GetActiveSource()" % accessor])
                else:
                    accessor = ProxyAccessor(cls.get_varname(pname), obj)
                    cls.Output.append_separated([ \
                        "# find source",
                        "%s = FindSource('%s')" % (accessor, pname)])
                return True
        if not skip_rendering and obj.SMProxy.IsA("vtkSMViewProxy"):
            # handle view proxy.
            pname = obj.SMProxy.GetSessionProxyManager().GetProxyName("views", obj.SMProxy)
            if pname:
                trace = TraceOutput()
                accessor = ProxyAccessor(cls.get_varname(pname), obj)
                if obj == simple.GetActiveView():
                    trace.append("# get active view")
                    trace.append("%s = GetActiveViewOrCreate('%s')" % (accessor, obj.GetXMLName()))
                else:
                    ctor_args = "'%s', viewtype='%s'" % (pname, obj.GetXMLName())
                    trace.append("# find view")
                    trace.append("%s = FindViewOrCreate(%s)" % (accessor, ctor_args))
                cls.Output.append_separated(trace.raw_data())
                return True
        if not skip_rendering and obj.SMProxy.IsA("vtkSMRepresentationProxy"):
            # handle representations.
            if hasattr(obj, "Input"):
                inputAccsr = cls.get_accessor(obj.Input)
                view = simple.LocateView(obj)
                viewAccessor = cls.get_accessor(view)
                pname = obj.SMProxy.GetSessionProxyManager().GetProxyName("representations", obj.SMProxy)
                if pname:
                    varname = "%sDisplay" % inputAccsr
                    accessor = ProxyAccessor(cls.get_varname(varname), obj)
                    cls.Output.append_separated([ \
                        "# get display properties",
                        "%s = GetRepresentation(%s, view=%s)" % \
                        (accessor, inputAccsr, viewAccessor)])
                    return True
        if not skip_rendering and cls.get_registered_name(obj, "lookup_tables"):
            pname = cls.get_registered_name(obj, "lookup_tables")
            if cls._create_accessor_for_tf(obj, pname, selector):
                return True
        if not skip_rendering and cls.get_registered_name(obj, "piecewise_functions"):
            pname = cls.get_registered_name(obj, "piecewise_functions")
            if cls._create_accessor_for_tf(obj, pname):
                return True
        if not skip_rendering and cls.get_registered_name(obj, "transfer_2d_functions"):
            pname = cls.get_registered_name(obj, "transfer_2d_functions")
            if cls._create_accessor_for_tf(obj, pname):
                return True
        if not skip_rendering and cls.get_registered_name(obj, "scalar_bars"):
            # trace scalar bar.
            lutAccessor = cls.get_accessor(obj.LookupTable)
            view = simple.LocateView(obj)
            viewAccessor = cls.get_accessor(view)
            varname = cls.get_varname("%sColorBar" % lutAccessor)
            accessor = ProxyAccessor(varname, obj)
            trace = TraceOutput()
            trace.append( \
                "# get color legend/bar for %s in view %s" % (lutAccessor, viewAccessor))
            trace.append(accessor.trace_ctor( \
                "GetScalarBar",
                SupplementalProxy(ScalarBarProxyFilter()),
                ctor_args="%s, %s" % (lutAccessor, viewAccessor)))
            cls.Output.append_separated(trace.raw_data())
            return True
        if not skip_rendering and cls.get_registered_name(obj, "animation"):
            return cls._create_accessor_for_animation_proxies(obj)
        if obj.SMProxy.GetXMLName() == "RepresentationAnimationHelper":
            sourceAccessor = cls.get_accessor(obj.Source)
            varname = cls.get_varname("%sRepresentationAnimationHelper" % (sourceAccessor))
            accessor = ProxyAccessor(varname, obj)
            cls.Output.append_separated([ \
                " # get animation representation helper for '%s'" % (sourceAccessor),
                "%s = GetRepresentationAnimationHelper(%s)" % (accessor, sourceAccessor)])
            return True
        if not skip_rendering and cls.get_registered_name(obj, "layouts"):
            view = simple.GetActiveView()
            if view and obj.GetViewLocation(view.SMProxy) != -1:
                viewAccessor = cls.get_accessor(view)
                varname = cls.get_varname(cls.get_registered_name(obj, "layouts"))
                accessor = ProxyAccessor(varname, obj)
                cls.Output.append_separated([ \
                    "# get layout",
                    "%s = GetLayout()" % accessor])
                return True
            else:
                varname = cls.get_varname(cls.get_registered_name(obj, "layouts"))
                accessor = ProxyAccessor(varname, obj)
                cls.Output.append_separated([ \
                    "# get layout",
                    "%s = GetLayoutByName(\"%s\")" % (accessor, cls.get_registered_name(obj, "layouts"))])
                return True
        if obj.SMProxy.IsA("vtkSMTimeKeeperProxy"):
            tkAccessor = ProxyAccessor(cls.get_varname(cls.get_registered_name(obj, "timekeeper")), obj)
            cls.Output.append_separated([ \
                "# get the time-keeper",
                "%s = GetTimeKeeper()" % tkAccessor])
            return True
        if obj.SMProxy.IsA("vtkSMSettingsProxy"):
            pname = obj.SMProxy.GetSessionProxyManager().GetProxyName("settings", obj.SMProxy)
            accessor = ProxyAccessor(cls.get_varname(pname), obj)
            cls.Output.append_separated([ \
                "# find settings proxy",
                "%s = GetSettingsProxy('%s')" % (accessor, pname)])
            return True
        if not skip_rendering and obj.GetVTKClassName() == "vtkPVLight":
            view = simple.GetViewForLight(obj)
            if view:
                index = view.AdditionalLights.index(obj)
                viewAccessor = cls.get_accessor(view)
                accessor = ProxyAccessor(cls.get_varname(cls.get_registered_name(obj, "additional_lights")), obj)
                cls.Output.append_separated([ \
                    "# get light",
                    "%s = GetLight(%s, %s)" % (accessor, index, viewAccessor)])
            else:
                # create a new light, should be handled by RegisterLightProxy
                accessor = ProxyAccessor(cls.get_varname(cls.get_registered_name(obj, "additional_lights")), obj)
                cls.Output.append_separated([ \
                    "# create a new light",
                    "%s = CreateLight()" % (accessor)])
            return True
        if not skip_rendering and obj.SMProxy.IsA("vtkSMMaterialLibraryProxy"):
            tkAccessor = ProxyAccessor(cls.get_varname(cls.get_registered_name(obj, "materiallibrary")), obj)
            cls.Output.append_separated([ \
                "# get the material library",
                "%s = GetMaterialLibrary()" % tkAccessor])
            return True
        if not skip_rendering and obj.GetVTKClassName() == "vtkTexture":
            pname = obj.SMProxy.GetSessionProxyManager().GetProxyName("textures", obj.SMProxy)
            if pname:
                accessor = ProxyAccessor(cls.get_varname(pname), obj)
                mode = obj.Mode
                if mode == 'ReadFromFile':
                    filename = obj.FileName
                    # repr() will escape the path correctly, especially backslash on Windows.
                    cls.Output.append_separated([ \
                            "# a texture",
                            "%s = FindTextureOrCreate(registrationName=%s, filename=%s)" % (accessor, repr(pname), repr(filename))])
                else:
                    tp_key = obj.TrivialProducerKey
                    cls.Output.append_separated([ \
                            "# a texture",
                            "%s = FindTextureOrCreate(registrationName=%s, filename=None, trivial_producer_key=%s)" % (accessor, repr(pname), repr(tp_key))])
            return True
        if cls.get_registered_name(obj, "extractors"):
            regName = cls.get_registered_name(obj, "extractors")
            accessor = ProxyAccessor(cls.get_varname(regName), obj)
            cls.Output.append_separated([ \
                "# get extractor",
                "%s = FindExtractor('%s')" % (accessor, regName)])
        return False

    @classmethod
    def rename_separate_tf_and_get_representation(cls, arrayName, selector=None):
        import re
        representation = None
        varname = arrayName
        if selector is None:
            regex = re.compile(r"(^Separate_)([0-9]*)_(.*$)")
        else:
            regex = re.compile(r"(^Separate_)([0-9]*)_(.*)_(.*$)")
        if re.match(regex, arrayName):
            gid = re.sub(regex, "\\g<2>", arrayName)
            representation = next((value for key, value in simple.GetRepresentations().items() if key[1] == gid), None)
            if representation:
                repAccessor = Trace.get_accessor(representation)
                if selector is None:
                    arrayName = re.sub(regex, "\\g<3>", arrayName)
                    varname = ("Separate_%s_%s" % (repAccessor, arrayName))
                else:
                    arrayName = re.sub(regex, "\\g<4>", arrayName)
                    varname = ("Separate_%s_%s_%s" % (repAccessor, selector, arrayName))
        return arrayName, varname, representation

    @classmethod
    def _create_accessor_for_tf(cls, proxy, regname, selector=None):
        import re
        m = re.match("^[0-9.]*(.+)\\.%s$" % proxy.GetXMLName(), regname)
        if m:
            arrayName = m.group(1)
            if proxy.GetXMLGroup() == "lookup_tables":
                arrayName, varname, rep = cls.rename_separate_tf_and_get_representation(arrayName, selector)
                if selector is None:
                    if rep:
                        repAccessor = Trace.get_accessor(rep)
                        args = ("'%s', %s, separate=True" % (arrayName, repAccessor))
                        comment = "separate color transfer function/color map"
                    else:
                        args = ("'%s'" % arrayName)
                        comment = "color transfer function/color map"
                    method = "GetColorTransferFunction"
                    varsuffix = "LUT"
                else:
                    if rep:
                        repAccessor = Trace.get_accessor(rep)
                        args = ("'%s', '%s', %s, separate=True" % (selector, arrayName, repAccessor))
                        comment = "separate block color transfer function/block color map"
                    else:
                        args = ("'%s', '%s'" % (selector, arrayName))
                        comment = "block color block transfer function/block color map"
                    method = "GetBlockColorTransferFunction"
                    varsuffix = "BlockLUT"
            elif proxy.GetXMLGroup() == "piecewise_functions":
                arrayName, varname, rep = cls.rename_separate_tf_and_get_representation(arrayName)
                if rep:
                    repAccessor = Trace.get_accessor(rep)
                    args = ("'%s', %s, separate=True" % (arrayName, repAccessor))
                    comment = "separate opacity transfer function/opacity map"
                else:
                    args = ("'%s'" % arrayName)
                    comment = "opacity transfer function/opacity map"
                method = "GetOpacityTransferFunction"
                varsuffix = "PWF"
            else:
                arrayName, varname, rep = cls.rename_separate_tf_and_get_representation(arrayName)
                if rep:
                    repAccessor = Trace.get_accessor(rep)
                    args = ("'%s', %s, separate=True" % (arrayName, repAccessor))
                    comment = "separate 2D transfer function"
                else:
                    args = ("'%s'" % arrayName)
                    comment = "2D transfer function"
                method = "GetTransferFunction2D"
                varsuffix = "TF2D"

            varname = cls.get_varname("%s%s" % (varname, varsuffix))
            accessor = ProxyAccessor(varname, proxy)
            # cls.Output.append_separated([\
            #    "# get %s for '%s'" % (comment, arrayName),
            #    "%s = %s('%s')" % (accessor, method, arrayName)])
            trace = TraceOutput()
            trace.append("# get %s for '%s'" % (comment, arrayName))
            trace.append(accessor.trace_ctor( \
                method, SupplementalProxy(TransferFunctionProxyFilter()), ctor_args=args))
            cls.Output.append_separated(trace.raw_data())
            return True
        return False

    @classmethod
    def _create_accessor_for_animation_proxies(cls, obj):
        pname = cls.get_registered_name(obj, "animation")
        if obj == simple.GetAnimationScene():
            sceneAccessor = ProxyAccessor(cls.get_varname(pname), obj)
            cls.Output.append_separated([ \
                "# get animation scene",
                "%s = GetAnimationScene()" % sceneAccessor])
            return True
        if obj == simple.GetTimeTrack():
            accessor = ProxyAccessor(cls.get_varname(pname), obj)
            cls.Output.append_separated([ \
                "# get time animation track",
                "%s = GetTimeTrack()" % accessor])
            return True
        if obj.GetXMLName() == "CameraAnimationCue":
            # handle camera animation cue.
            view = obj.AnimatedProxy
            viewAccessor = cls.get_accessor(view)
            accessor = ProxyAccessor(cls.get_varname(pname), obj)
            cls.Output.append_separated([ \
                "# get camera animation track for the view",
                "%s = GetCameraTrack(view=%s)" % (accessor, viewAccessor)])
            return True
        if obj.GetXMLName() == "KeyFrameAnimationCue":
            animatedProxyAccessor = cls.get_accessor(obj.AnimatedProxy)
            animatedElement = int(obj.AnimatedElement)
            animatedPropertyName = obj.AnimatedPropertyName
            varname = cls.get_varname("%s%sTrack" % (animatedProxyAccessor, animatedPropertyName))
            accessor = ProxyAccessor(varname, obj)
            cls.Output.append_separated([ \
                "# get animation track",
                "%s = GetAnimationTrack('%s', index=%d, proxy=%s)" % \
                (accessor, animatedPropertyName, animatedElement, animatedProxyAccessor)])
            return True
        if obj.GetXMLName() == "PythonAnimationCue":
            raise Untraceable("PythonAnimationCue's are currently not supported in trace")
        if obj.GetXMLGroup() == "animation_keyframes":
            accessor = ProxyAccessor(cls.get_varname(pname), obj)
            ctor = sm._make_name_valid(obj.GetXMLLabel())
            cls.Output.append_separated("# create a new key frame")
            cls.Output.append(accessor.trace_ctor(ctor, ProxyFilter()))
            return True
        return False


class Untraceable(Exception):
    def __init__(self, logmessage="<unspecified>"):
        self.LogMessage = logmessage

    def __str__(self):
        return repr(self.LogMessage)


class Accessor(object):
    def __init__(self, varname, obj):
        self.Varname = varname
        self.__Object = obj
        Trace.register_accessor(self)

    def finalize(self):
        Trace.unregister_accessor(self)
        self.__Object = None

    def __str__(self):
        return self.Varname

    def get_object(self):
        return self.__Object


class PropertyFormatter:
    def format(self, data, proxy):
        raise NotImplementedError


class DefaultFormatter(PropertyFormatter):
    def format(self, data, proxy):
        return repr(data)


class TabulatedVectorFormatter(PropertyFormatter):
    def __init__(self, col_names):
        self.col_names = col_names

    def format(self, data, proxy):
        if len(data) == 0:
            return "[]"

        n_cols = len(self.col_names)

        if n_cols == 0:
            return repr(data)

        assert len(data) % n_cols == 0

        INDENT = "    "

        lines = []
        lines.append("[")
        lines.append(f"{INDENT}# {', '.join(self.col_names)}")
        for i in range(len(data) // n_cols):
            start = i * n_cols
            stop = start + n_cols
            lines.append(
                f"{INDENT}{', '.join(str(d) for d in data[start:stop])},"
            )
        lines.append("]")

        return "\n".join(lines)


class RGBPointsFormatter(TabulatedVectorFormatter):
    def __init__(self):
        # Initialize the tabulated vector formatter
        # We will fall back on using this if we can't figure out any
        # other way to re-generate the RGBPoints.
        super().__init__(["scalar", "red", "green", "blue"])

    def _is_default_preset(self, name: str) -> bool:
        # Check if `name` is one of the default presets
        presets = sm.vtkSMTransferFunctionPresets.GetInstance()
        preset_idx = -1
        for i in range(presets.GetNumberOfPresets()):
            if last_preset_name == presets.GetPresetName(i):
                preset_idx = i
                break

        return preset_idx != -1 and presets.IsPresetBuiltin(preset_idx)

    def _rgb_points_match(self, lut, kwargs: dict) -> bool:
        # Check if using the provided `kwargs` in `GenerateRGBPoints()`
        # produces the same RGBPoints as those on `lut`.
        new_points = simple.color.GenerateRGBPoints(**kwargs)
        current_points = lut.RGBPoints

        # Due to the unreliability of direct comparison between floating point
        # numbers, instead of doing `new_points == current_points`, use
        # `math.isclose()` between every pair of numbers.
        if len(new_points) != len(current_points):
            return False

        for v1, v2 in zip(new_points, current_points):
            # `math.isclose()` by default uses an absolute tolerance of 0
            # and a relative tolerance of 1e-09.
            if not math.isclose(v1, v2):
                return False

        return True

    def _format_generate_rgb_points(self, kwargs: dict) -> str:
        # Format `GenerateRGBPoints()`, along with its `kwargs`, as it will
        # appear in the Python state file or Python trace.
        indent = ' ' * 4
        kwargs_str = (
            f'\n{indent}' +
            f',\n{indent}'.join([f'{k}={repr(v)}' for k, v in kwargs.items()])
        )
        return f'GenerateRGBPoints({kwargs_str},\n)'

    def format(self, data, proxy):
        lut = proxy

        # Make aliases for the long function names
        matches = self._rgb_points_match
        format_out = self._format_generate_rgb_points

        kwargs = {}
        if matches(lut, kwargs):
            # Defaults worked!
            return format_out(kwargs)

        last_preset_name = lut.NameOfLastPresetApplied
        if last_preset_name:
            kwargs['preset_name'] = last_preset_name
            if matches(lut, kwargs):
                # Applying preset with no range specified worked!
                return format_out(kwargs)

        # Try to make the range match
        rgb_points = lut.RGBPoints
        if len(rgb_points) > 3:
            kwargs['range_min'] = lut.RGBPoints[0]
            kwargs['range_max'] = lut.RGBPoints[-4]
            if matches(lut, kwargs):
                # Specifying range, possibly with a preset, worked!
                return format_out(kwargs)

        # If all else fails, revert to writing them all out
        return super().format(data, proxy)


DEFAULT_FORMATTER = DefaultFormatter()
PROPERTY_FORMATTERS = {
    "RGBPoints": RGBPointsFormatter(),
}


def get_property_formatter(property_trace_helper):
    return PROPERTY_FORMATTERS.get(
        property_trace_helper.PropertyName, DEFAULT_FORMATTER
    )


def indent_lines(string, n_indent):
    if '"""' in string or "'''" in string:
        # Do not perform any indentation on multiline strings
        return string

    lines = string.splitlines(keepends=True)
    INDENT = "    " * n_indent
    return INDENT.join(lines)


class RealProxyAccessor(Accessor):
    __CreateCallbacks = []

    @classmethod
    def register_create_callback(cls, function):
        cls.__CreateCallbacks.insert(0, function)

    @classmethod
    def unregister_create_callback(cls, function):
        cls.__CreateCallbacks.remove(function)

    @classmethod
    def create(cls, *args, **kwargs):
        for x in cls.__CreateCallbacks:
            try:
                return x(*args, **kwargs)
            except NotImplementedError:
                pass
        return RealProxyAccessor(*args, **kwargs)

    def __init__(self, varname, proxy):
        Accessor.__init__(self, varname, proxy)

        self.OrderedProperties = []

        # Create accessors for properties on this proxy.
        oiter = sm.vtkSMOrderedPropertyIterator()
        oiter.SetProxy(proxy.SMProxy)
        while not oiter.IsAtEnd():
            prop_name = oiter.GetKey()
            prop_label = oiter.GetPropertyLabel()
            sanitized_label = sm._make_name_valid(prop_label)

            prop = proxy.GetProperty(prop_name)
            if not type(prop) == sm.Property:
                # Note: when PropertyTraceHelper for a property with ProxyListDomain is
                # created, it creates accessors for all proxies in the domain as well.
                prop_accessor = PropertyTraceHelper(sanitized_label, self)
                self.OrderedProperties.append(prop_accessor)
            oiter.Next()
        del oiter

    def finalize(self):
        for x in self.OrderedProperties:
            x.finalize()
        Accessor.finalize(self)

    def get_property(self, name):
        for x in self.OrderedProperties:
            if x.get_property_name() == name:
                return x
        return None

    def get_properties(self):
        return self.OrderedProperties[:]

    def get_ctor_properties(self):
        """Returns a list of property accessors that should be specified
           in the constructor."""
        return [x for x in self.OrderedProperties if self.is_ctor_property(x)]

    def is_ctor_property(self, prop):
        return prop.get_object().IsA("vtkSMInputProperty") or \
            prop.get_object().FindDomain("vtkSMFileListDomain") != None

    def trace_properties(self, props, in_ctor):
        if in_ctor:
            return ",\n    ".join([
                indent_lines(x.get_property_trace(in_ctor), 1) for x in props]
            )

        # Sort the props so that any multiline strings get moved to the back.
        # This is just because it looks nicer...
        def sort_key(prop):
            val = prop.get_value()
            return '"""' in val or "'''" in val

        props = sorted(props, key=sort_key)

        lines = []
        if len(props) > 1:
            proxy_name = props[0].ProxyAccessor
            lines.append(f"{proxy_name}.Set(")
            for prop in props:
                lines.append(f"    {indent_lines(prop.get_property_trace(True), 1)},")
            lines.append(f")")
        else:
            for prop in props:
                lines.append(prop.get_property_trace(False))

        return "\n".join(lines)

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False,
                   ctor_var=None, ctor_extra_args=None):
        args_in_ctor = str(ctor_args) if ctor_args is not None else ""
        # trace any properties that the 'filter' tells us should be traced
        # in ctor.
        # when ctor is null, add them to the other props to trace in creation
        other_props = []
        ctor_props = [x for x in self.OrderedProperties if filter.should_trace_in_ctor(x)]
        if not ctor is None:
          ctor_props_trace = self.trace_properties(ctor_props, in_ctor=True)
          if args_in_ctor and ctor_props_trace:
              args_in_ctor = "%s, %s" % (args_in_ctor, ctor_props_trace)
          else:
              args_in_ctor += ctor_props_trace
          if args_in_ctor and ctor_extra_args:
              args_in_ctor = "%s, %s" % (args_in_ctor, ctor_extra_args)
          elif ctor_extra_args:
              args_in_ctor = ctor_extra_args
        else:
          other_props += ctor_props

        # locate all the other properties that should be traced in create.
        other_props += [x for x in self.OrderedProperties \
                       if filter.should_trace_in_create(x) and not filter.should_trace_in_ctor(x)]

        trace = TraceOutput()
        if not ctor is None:
            if not skip_assignment:
                trace.append("%s = %s(%s)" % (self, ctor, args_in_ctor))
            else:
                assert len(other_props) == 0
                trace.append("%s(%s)" % (ctor, args_in_ctor))
                return trace.raw_data()

        # FIXME: would like trace_properties() to return a list instead of
        # a string.
        txt = self.trace_properties(other_props, in_ctor=False)
        if txt:
            trace.append(txt)

        # Now, if any of the props has ProxyListDomain, we should trace their
        # "ctors" as well. Tracing ctors for ProxyListDomain proxies simply
        # means tracing their property values.
        pld_props = [x for x in self.OrderedProperties if x.has_proxy_list_domain()]
        for prop in pld_props:
            paccessor = Trace.get_accessor(prop.get_property_value())
            if not prop.DisableSubTrace:
                sub_trace = paccessor.trace_ctor(None, filter)
                if sub_trace:
                    trace.append_separated( \
                        "# init the %s selected for '%s'" % (prop.get_value(), prop.get_property_name()))
                    trace.append(sub_trace)
        return trace.raw_data()


def ProxyAccessor(*args, **kwargs):
    return RealProxyAccessor.create(*args, **kwargs)


class PropertyTraceHelper(object):
    """PropertyTraceHelper is used by RealProxyAccessor to help with tracing
    properites. In its constructor, RealProxyAccessor creates a
    PropertyTraceHelper for each of its properties that could potentially need
    to be traced."""

    def __init__(self, propertyname, proxyAccessor):
        """Constructor.

          :param propertyname: name used to access the property. This is the
          sanitized property label.
          :param proxyAccessor: RealProxyAccessor instance for the proxy.
        """
        assert isinstance(proxyAccessor, RealProxyAccessor)
        assert type(propertyname) == str

        self.__PyProperty = None
        self.PropertyName = propertyname
        self.ProxyAccessor = proxyAccessor
        self.FullScopedName = "%s.%s" % (proxyAccessor, propertyname)

        pyprop = self.get_object()
        assert not pyprop is None

        pld_domain = pyprop.FindDomain("vtkSMProxyListDomain")
        self.HasProxyListDomain = isinstance(pyprop, sm.ProxyProperty) and pld_domain != None
        self.ProxyListDomainProxyAccessors = []
        self.DisableSubTrace = pyprop.GetDisableSubTrace()
        if self.HasProxyListDomain:
            # register accessors for proxies in the proxy list domain.
            # This is cheating. Since there's no accessor for a proxy in the domain
            # unless the proxy is "active" in the property. However, since ParaView
            # UI never modifies the other properties, we cheat
            for i in range(pld_domain.GetNumberOfProxies()):
                domain_proxy = pld_domain.GetProxy(i)
                plda = ProxyAccessor(self.get_varname(), sm._getPyProxy(domain_proxy))
                self.ProxyListDomainProxyAccessors.append(plda)

    def __del__(self):
        self.finalize()

    def finalize(self):
        for x in self.ProxyListDomainProxyAccessors:
            x.finalize()
        self.ProxyListDomainProxyAccessors = []

    def get_object(self):
        """Returns the servermanager.Property (or subclass) for the
        vtkSMProperty this trace helper is helping with."""
        if self.__PyProperty is None or self.__PyProperty() is None:
            # This will raise Untraceable exception is the ProxyAccessor cannot
            # locate the servermanager.Proxy for the SMProxy it refers to.
            pyproperty = self.ProxyAccessor.get_object().GetProperty(self.get_property_name())
            self.__PyProperty = weakref.ref(pyproperty)
            return pyproperty
        return self.__PyProperty()

    def get_property_trace(self, in_ctor):
        """return trace-text for the property.

        :param in_ctor: If False, the trace is generated trace will use
            fully-scoped name when referring to the property e.g.
            sphere0.Radius=2, else it will use just the property name, *e.g.*,
            Radius=2.
        """
        varname = self.get_varname(in_ctor)
        if in_ctor:
            return "%s=%s" % (varname, self.get_value())
        else:
            return "%s = %s" % (varname, self.get_value())

    def get_varname(self, not_fully_scoped=False):
        """Returns the variable name to use when referring to this property.

        :param not_fully_scoped: If False, this will return
            fully-scoped name when referring to the property e.g. sphere0.Radius,
            else it will use just the property name, *e.g.*, Radius.
        """
        return self.PropertyName if not_fully_scoped else self.FullScopedName

    def get_value(self):
        """Returns the property value as a string. For proxy properties, this
        will either be a string used to refer to another proxy or a string used
        to refer to the proxy in a proxy list domain."""
        myobject = self.get_object()
        fileListDomain = myobject.SMProperty.FindDomain("vtkSMFileListDomain")
        if isinstance(myobject, sm.ProxyProperty):
            data = myobject[:]
            if self.has_proxy_list_domain():
                data = ["'%s'" % x.GetXMLLabel() for x in data]
            else:
                data = [str(Trace.get_accessor(x)) for x in data]
                if isinstance(myobject, sm.InputProperty):
                    # this is an input property, we may have to hook on to a
                    # non-zero output port. If so, we trace `OutputPort(source,
                    # port)`, else we just trace `source`.
                    # Fixes #17035
                    ports = [myobject.GetOutputPortForConnection(x) for x in range(len(data))]
                    data = [src if port == 0 else "OutputPort(%s,%d)" % (src, port) \
                            for src, port in zip(data, ports)]

            if len(data) > 1:
                return "[%s]" % (", ".join(data))
            elif len(data) == 0:
                # Special case for repeatable, should return [] rather than None
                if myobject.SMProperty.GetRepeatable():
                    return "[]"
                else:
                    return "None"
            else:
                return data[0]

        elif (myobject.SMProperty.IsA("vtkSMStringVectorProperty") and not
        (fileListDomain and fileListDomain.GetIsOptional() == 0
         or myobject.Proxy.GetVTKClassName() == "vtkPythonAnnotationFilter")):
            # handle multiline properties (see #18480)
            # but, not if the property is a list of files (see #21100)
            # nor if the filter is a PythonAnnotation (see #21654)
            return self.create_multiline_string(repr(myobject))
        else:
            return self.format_object(myobject)

    def format_object(self, obj):
        formatter = get_property_formatter(self)
        proxy = self.get_proxy()

        try:
            return formatter.format(obj, proxy)
        except Exception:
            return DEFAULT_FORMATTER.format(obj, proxy)

    def has_proxy_list_domain(self):
        """Returns True if this property has a ProxyListDomain, else False."""
        return self.HasProxyListDomain

    def get_property_name(self):
        return self.PropertyName

    def get_property_value(self):
        """Return the Property value as would be returned by
        servermanager.Proxy.GetPropertyValue()."""
        return self.ProxyAccessor.get_object().GetPropertyValue(self.get_property_name())

    def get_proxy(self):
        return self.ProxyAccessor.get_object()

    def create_multiline_string(self, astr):
        """helper to convert a string representation into a multiline string"""
        if '\\n' in astr:
            # this happens for multiline string-vector properties.
            # for those, we ensure that the `astr` has raw \n's rather than
            # the escaped version. we also fix the string indicators.

            # replace '\\n' with real '\n'
            astr = astr.replace('\\n', '\n')

            # escape any `"""` in the script
            astr = astr.replace('"""', '\\"\\"\\"')

            # replace first and last characters with `"""`
            astr = '"""' + astr[1:-1] + '"""'
        return astr


# ===================================================================================================
# === Filters used to filter properties traced ===
# ===================================================================================================
class ProxyFilter(object):
    def __init__(self, trace_all_in_ctor=False):
        self.trace_all_in_ctor = trace_all_in_ctor

    def should_never_trace(self, prop, hide_gui_hidden=True):
        if prop.get_object().GetIsInternal() or prop.get_object().GetInformationOnly():
            return True
        # should we hide properties hidden from panels? yes, generally, except
        # Views.
        if hide_gui_hidden == True and prop.get_object().GetPanelVisibility() == "never":
            return True
        # if a property is "linked" to settings, then skip it here too. We
        # should eventually add an option for user to save, yes, save these too.
        # Note, however, any property that is linked with `unlink_if_modified`
        # set to 1 should be traced if the link no longer persists -- indicating
        # the user manually changed it (and hence broke the link).
        if prop.get_object().GetHints():
            plink = prop.get_object().GetHints().FindNestedElementByName("PropertyLink")
            if plink and plink.GetAttribute("group") == "settings":
                if plink.GetAttribute("unlink_if_modified") != "1":
                    return True
                proxy = prop.get_proxy()
                pxm = proxy.GetSessionProxyManager()
                settings = pxm.GetProxy("settings", plink.GetAttribute("proxy"))
                if settings and settings.GetSourcePropertyName(proxy.SMProxy, prop.get_property_name()):
                    # the settings link still exists, skip tracing
                    return True
        return False

    def should_trace_in_create(self, prop, user_can_modify_in_create=True):
        if self.should_never_trace(prop): return False

        setting = sm.vtkSMTrace.GetActiveTracer().GetPropertiesToTraceOnCreate()
        if setting == sm.vtkSMTrace.RECORD_USER_MODIFIED_PROPERTIES and not user_can_modify_in_create:
            # In ParaView, user never changes properties in Create. It's only
            # afterwords, so skip all properties.
            return False
        trace_props_with_default_values = True \
            if setting == sm.vtkSMTrace.RECORD_ALL_PROPERTIES else False

        if trace_props_with_default_values:
            # We are writing out every property
            return True

        obj = prop.get_object()
        if setting == sm.vtkSMTrace.RECORD_ACTIVE_MODIFIED_PROPERTIES:
          # Check whether the property decorators logic consider this property "relevant".
          # In this case relevant means that int the current state of paraview
          # this property is enabled (not grayed-out) and is visible. Visibility
          # is evaluated based on dependent properties and not "advanced" (i.e.
          # gear icon) state.
          proxy = prop.get_proxy()
          sanitized_label = sm._make_name_valid(obj.GetXMLName())
          should_trace = should_trace_based_on_decorators(proxy, sanitized_label)

          if should_trace is False:
              return False

        # Skip writing out `None` strings
        domain = prop.get_object().FindDomain("vtkSMArrayListDomain")
        if domain and domain.GetNoneString() == prop.get_property_value():
            return False

        if isinstance(obj, sm.ProxyProperty):
            # We already check every property when writing out the trace,
            # so we don't need to recursively check proxy properties. We only
            # need to check if the proxies themselves are the default ones.
            # For example, for a clipping plane, this prevents arguments such
            # as `ClipType='Plane'` from showing up in the trace when the
            # clip type in fact was not modified, but some of the plane's
            # properties were modified. This also applies to transfer
            # functions and many other proxy properties.
            return not obj.IsValueDefaultNonRecursive()

        return not obj.IsValueDefault()

    def should_trace_in_ctor(self, prop):
        return False if not self.trace_all_in_ctor else \
            (not self.should_never_trace(prop) and self.should_trace_in_create(prop))


class PipelineProxyFilter(ProxyFilter):
    def should_trace_in_create(self, prop):
        return ProxyFilter.should_trace_in_create(self, prop, user_can_modify_in_create=False)

    def should_never_trace(self, prop):
        """overridden to avoid hiding "non-gui" properties such as FileName."""
        # should we hide properties hidden from panels?
        if not prop.get_object().FindDomain("vtkSMFileListDomain") is None:
            return False
        else:
            return ProxyFilter.should_never_trace(self, prop)

    def should_trace_in_ctor(self, prop):
        if self.should_never_trace(prop): return False
        return prop.get_object().IsA("vtkSMInputProperty") or \
            prop.get_object().FindDomain("vtkSMFileListDomain") != None


class ExodusIIReaderFilter(PipelineProxyFilter):
    def should_never_trace(self, prop):
        if PipelineProxyFilter.should_never_trace(self, prop): return True
        # Exodus reader has way too many wacky properties tracing them causes
        # the reader to segfault. We need to either remove those properties
        # entirely or fix them. Until I get a chance to get to the bottom of it,
        # I am opting to ignore those properties when tracing.
        return prop.get_property_name() in [ \
            "FilePrefix", "XMLFileName", "FilePattern", "FileRange"]


class ExtractSelectionFilter(PipelineProxyFilter):
    def __init__(self, trace_all_in_ctor=False, save_selection=False):
        super().__init__(trace_all_in_ctor)
        self.save_selection = save_selection

    def should_never_trace(self, prop):
        if PipelineProxyFilter.should_never_trace(self, prop): return True

        # When saving state, Selection property should not be ignored
        if self.save_selection: return False

        # Selections are not registered with the proxy manager, so we will not try to trace them.
        return prop.get_property_name() in ["Selection"]


class RepresentationProxyFilter(PipelineProxyFilter):
    def should_trace_in_ctor(self, prop):
        return False

    def should_never_trace(self, prop):
        if PipelineProxyFilter.should_never_trace(self, prop): return True
        if prop.get_property_name() in ["Input", \
                                        "SelectionCellFieldDataArrayName", \
                                        "SelectionPointFieldDataArrayName"]: return True
        return False

    def should_trace_in_create(self, prop):
        """for representations, we always trace the 'Representation' property,
        even when it's same as the default value (see issue #17196)."""
        if prop.get_object().FindDomain("vtkSMRepresentationTypeDomain"):
            return True
        return PipelineProxyFilter.should_trace_in_create(self, prop)


class ViewProxyFilter(ProxyFilter):
    def should_trace_in_create(self, prop):
        # Camera parallel scale doesn't have a decorator, since it is never
        # displayed in the GUI. But we should still exclude it from the Python
        # state file if RECORD_ACTIVE_MODIFIED_PROPERTIES is being used and
        # CameraParallelProjection is not enabled.
        if prop.get_property_name() == 'CameraParallelScale':
            setting = sm.vtkSMTrace.GetActiveTracer().GetPropertiesToTraceOnCreate()
            if setting == sm.vtkSMTrace.RECORD_ACTIVE_MODIFIED_PROPERTIES:
                rv = prop.get_proxy()
                if rv and rv.CameraParallelProjection == 0:
                    # We should not include `CameraParallelScale` if parallel
                    # projection is disabled.
                    return False

        return super().should_trace_in_create(prop)

    def should_never_trace(self, prop):
        # skip "Representations" property and others.
        # The fact that we need to skip so many properties means that we are
        # missing something in the design of vtkSMProperties here. We need to
        # reclassify properties to cleanly address all its "roles".
        if prop.get_property_name() in [ \
                "ViewTime", "CacheKey", "Representations"]: return True
        return ProxyFilter.should_never_trace(self, prop, hide_gui_hidden=False)


class TimeKeeperProxyFilter(ProxyFilter):
    def should_never_trace(self, prop):
        if ProxyFilter.should_never_trace(self, prop): return True
        if prop.get_property_name() in ["Views", "TimeSources", "Time"]:
            return True
        return False


class AnimationProxyFilter(ProxyFilter):
    def should_never_trace(self, prop):
        if ProxyFilter.should_never_trace(self, prop): return True
        if prop.get_property_name() in ["AnimatedProxy", "AnimatedPropertyName",
                                        "AnimatedElement", "AnimatedDomainName", "TimeKeeper"]:
            return True
        return False


class ExporterProxyFilter(ProxyFilter):
    def should_trace_in_ctor(self, prop):
        return not self.should_never_trace(prop) and self.should_trace_in_create(prop)

    def should_never_trace(self, prop):
        if ProxyFilter.should_never_trace(self, prop): return True
        if prop.get_property_name() == "FileName": return True
        return False


class WriterProxyFilter(ProxyFilter):
    def should_trace_in_ctor(self, prop):
        return not self.should_never_trace(prop) and self.should_trace_in_create(prop)

    def should_never_trace(self, prop):
        if ProxyFilter.should_never_trace(self, prop): return True
        if prop.get_property_name() in ["FileName", "Input"]: return True
        return False


class ScreenShotHelperProxyFilter(ProxyFilter):
    def should_never_trace(self, prop):
        if prop.get_property_name() == "Format": return True
        return ProxyFilter.should_never_trace(self, prop)

    def should_trace_in_ctor(self, prop):
        return not self.should_never_trace(prop) and self.should_trace_in_create(prop)


class TransferFunctionProxyFilter(ProxyFilter):
    def should_trace_in_ctor(self, prop):
        return False

    def should_never_trace(self, prop):
        blacklist = [
            "ScalarOpacityFunction",
            # We apply this to the RGBPoints explicitly, so no need to
            # record it here.
            "NameOfLastPresetApplied",
        ]
        if ProxyFilter.should_never_trace(self, prop, hide_gui_hidden=False):
            return True

        if prop.get_property_name() in blacklist:
            return True

        return False


class ScalarBarProxyFilter(ProxyFilter):
    def should_trace_in_ctor(self, prop): return False

    def should_never_trace(self, prop):
        # despite being hidden from the panel, these properties should not be
        # skipped in trace.
        if prop.get_property_name() in ["Position", "Position2", "Orientation"]:
            return False
        return ProxyFilter.should_never_trace(self, prop)


class ExtractorFilter(ProxyFilter):
    def should_trace_in_ctor(self, prop): return False

    def should_never_trace(self, prop):
        if prop.get_property_name() in ["Writer"]:
            return True
        return super(ExtractorFilter, self).should_never_trace(prop)


class SaveExtractsFilter(ProxyFilter):
    def should_trace_in_ctor(self, prop): return True


def SupplementalProxy(cls):
    """This function decorates a ProxyFilter. Designed to be
    used for supplemental proxies, so that we can centralize the logic
    to decide whether to trace any of the properties on the supplemental
    proxies the first time that proxy is accessed."""
    setting = sm.vtkSMTrace.GetActiveTracer().GetFullyTraceSupplementalProxies()
    if setting: return cls

    def should_trace_in_ctor(self, *args, **kwargs):
        return False

    def should_trace_in_create(self, *args, **kwargs):
        return False

    cls.should_trace_in_create = should_trace_in_create
    cls.should_trace_in_ctor = should_trace_in_ctor
    return cls


# ===================================================================================================
# === TraceItem types ==
# TraceItems are units of traceable actions triggered by the application using vtkSMTrace
# ===================================================================================================
class RenderingMixin(object):
    @property
    def skip_from_trace(self):
        if _get_skip_rendering():
            return True
        return False


class TraceItem(object):
    def __init__(self):
        try:
            if self.skip_from_trace:
                raise Untraceable("skipped")
        except AttributeError:
            pass

    def finalize(self):
        pass


class NestableTraceItem(TraceItem):
    """Base class for trace item that can be nested i.e.
    can trace when some other trace item is active."""
    pass


class BookkeepingItem(NestableTraceItem):
    """Base class for trace items that are only used for
    book keeping and don't affect the trace itself."""
    pass


class RegisterPipelineProxy(TraceItem):
    """This traces the creation of a Pipeline Proxy such as
    sources/filters/readers etc."""

    def __init__(self, proxy, saving_state=False):
        TraceItem.__init__(self)
        self.Proxy = sm._getPyProxy(proxy)
        self.saving_state = saving_state

    def finalize(self):
        pname = Trace.get_registered_name(self.Proxy, "sources")
        varname = Trace.get_varname(pname)
        accessor = ProxyAccessor(varname, self.Proxy)

        ctor = sm._make_name_valid(self.Proxy.GetXMLLabel())
        trace = TraceOutput()
        trace.append("# create a new '%s'" % self.Proxy.GetXMLLabel())
        if isinstance(self.Proxy, sm.ExodusIIReaderProxy):
            filter_type = ExodusIIReaderFilter()
        elif self.Proxy.GetXMLName() == "ExtractSelection":
            filter_type = ExtractSelectionFilter(save_selection=self.saving_state)
        else:
            filter_type = PipelineProxyFilter()
        ctor_args = "registrationName='%s'" % pname
        trace.append(accessor.trace_ctor(ctor, filter_type, ctor_args=ctor_args))
        Trace.Output.append_separated(trace.raw_data())
        TraceItem.finalize(self)


class RegisterSelectionProxy(TraceItem):
    """This traces the creation of a Proxy for selection.
    This is used only when saving state for now."""

    def __init__(self, proxy):
        TraceItem.__init__(self)
        self.Proxy = sm._getPyProxy(proxy)

    def finalize(self):
        pname = Trace.get_registered_name(self.Proxy, "selection_sources")
        varname = Trace.get_varname(pname)
        accessor = ProxyAccessor(varname, self.Proxy)

        xmlname = self.Proxy.GetXMLName()
        trace = TraceOutput()
        trace.append("# create a new '%s'" % self.Proxy.GetXMLLabel())
        filter_type = ProxyFilter(trace_all_in_ctor=True)
        ctor_args = "proxyname='%s', registrationname='%s'" % (xmlname, pname)
        trace.append(accessor.trace_ctor("CreateSelection", filter_type, ctor_args=ctor_args))
        Trace.Output.append_separated(trace.raw_data())
        TraceItem.finalize(self)


class Delete(TraceItem):
    """This traces the deletion of a Pipeline proxy"""

    def __init__(self, proxy):
        TraceItem.__init__(self)
        proxy = sm._getPyProxy(proxy)
        accessor = Trace.get_accessor(proxy)
        Trace.Output.append_separated([ \
            "# destroy %s" % (accessor),
            "Delete(%s)" % (accessor),
            "del %s" % accessor])
        accessor.finalize()
        del accessor
        import gc
        gc.collect()


class CleanupAccessor(BookkeepingItem):
    def __init__(self, proxy):
        self.Proxy = sm._getPyProxy(proxy)

    def finalize(self):
        if Trace.has_accessor(self.Proxy):
            accessor = Trace.get_accessor(self.Proxy)
            accessor.finalize()
            del accessor
        import gc
        gc.collect()


class BlockTraceItems(NestableTraceItem):
    """Item to block further creation of trace items, even
    those that are `NestableTraceItem`. Simply create this and
    no trace items will be created by `_create_trace_item_internal`
    until this instance is cleaned up.
    """
    pass


class PropertiesModified(NestableTraceItem):
    """Traces properties modified on a specific proxy."""

    def __init__(self, proxy, selector=None, comment=None):
        TraceItem.__init__(self)

        proxy = sm._getPyProxy(proxy)
        self.ProxyAccessor = Trace.get_accessor(proxy, selector)
        self.MTime = vtkTimeStamp()
        self.MTime.Modified()
        self.Comment = "#%s" % comment if not comment is None else \
            "# Properties modified on %s" % str(self.ProxyAccessor)

    def finalize(self):
        props = self.ProxyAccessor.get_properties()
        props_to_trace = [k for k in props if self.MTime.GetMTime() < k.get_object().GetMTime()]
        if props_to_trace:
            Trace.Output.append_separated([
                self.Comment,
                self.ProxyAccessor.trace_properties(props_to_trace, in_ctor=False)])

        # Remember, we are monitoring a proxy to trace any properties on it that
        # are modified. When that's the case, properties on a proxy-property on
        # that proxy may have been modified too and it would make sense to trace
        # those as well (e.g. ScalarOpacityFunction on a PVLookupTable proxy).
        # This loop handles that. We explicitly skip "InputProperty"s, however
        # since tracing properties modified on the input should not be a
        # responsibility of this method.
        for prop in props:
            if not isinstance(prop.get_object(), sm.ProxyProperty) or \
                    isinstance(prop.get_object(), sm.InputProperty) or \
                    prop.DisableSubTrace:
                continue
            val = prop.get_property_value()
            try:
                # val can be None or list of proxies. We are not tracing list of
                # proxies since we don't want to trace properties like
                # `view.Representations`.
                if not val or not isinstance(val, sm.Proxy): continue
                valaccessor = Trace.get_accessor(val)
            except Untraceable:
                continue
            else:
                props = valaccessor.get_properties()
                props_to_trace = [k for k in props if self.MTime.GetMTime() < k.get_object().GetMTime()]
                if props_to_trace:
                    Trace.Output.append_separated([
                        "# Properties modified on %s" % valaccessor,
                        valaccessor.trace_properties(props_to_trace, in_ctor=False)])
        TraceItem.finalize(self)


class ScalarBarInteraction(RenderingMixin, NestableTraceItem):
    """Traces scalar bar interactions"""

    def __init__(self, proxy, comment=None):
        TraceItem.__init__(self)
        proxy = sm._getPyProxy(proxy)
        self.ProxyAccessor = Trace.get_accessor(proxy)
        self.MTime = vtkTimeStamp()
        self.MTime.Modified()
        self.Comment = "#%s" % comment if not comment is None else \
            "# Properties modified on %s" % str(self.ProxyAccessor)

    def finalize(self):
        props = self.ProxyAccessor.get_properties()
        props_to_trace = [k for k in props if self.MTime.GetMTime() < k.get_object().GetMTime()]
        afilter = ScalarBarProxyFilter()
        props_to_trace = [k for k in props_to_trace if not afilter.should_never_trace(k)]
        if props_to_trace:
            Trace.Output.append_separated([
                self.Comment,
                self.ProxyAccessor.trace_properties(props_to_trace, in_ctor=False)])


class Show(RenderingMixin, TraceItem):
    """Traces Show"""

    def __init__(self, producer, port, view, display, comment=None):
        TraceItem.__init__(self)

        producer = sm._getPyProxy(producer)
        view = sm._getPyProxy(view)
        display = sm._getPyProxy(display)

        self.ProducerAccessor = Trace.get_accessor(producer)
        self.ViewAccessor = Trace.get_accessor(view)
        self.OutputPort = port
        self.Display = display
        self.Comment = comment

    def finalize(self):
        display = self.Display
        output = TraceOutput()
        if not Trace.has_accessor(display):
            pname = "%sDisplay" % self.ProducerAccessor
            accessor = ProxyAccessor(Trace.get_varname(pname), display)
            trace_ctor = True
        else:
            accessor = Trace.get_accessor(display)
            trace_ctor = False
        port = self.OutputPort

        if not self.Comment is None:
            output.append("# %s" % self.Comment)
        else:
            output.append("# show data in view")
        if port > 0:
            output.append("%s = Show(OutputPort(%s, %d), %s, '%s')" % \
                          (str(accessor), str(self.ProducerAccessor), port, str(self.ViewAccessor), \
                           str(display.GetXMLName())))
        else:
            output.append("%s = Show(%s, %s, '%s')" % \
                          (str(accessor), str(self.ProducerAccessor), str(self.ViewAccessor), \
                           str(display.GetXMLName())))
        Trace.Output.append_separated(output.raw_data())

        output = TraceOutput()
        if trace_ctor:
            # Now trace default values.
            ctor_trace = accessor.trace_ctor(None, RepresentationProxyFilter())
            if ctor_trace:
                output.append("# trace defaults for the display properties.")
                output.append(ctor_trace)
        Trace.Output.append_separated(output.raw_data())
        TraceItem.finalize(self)


class Hide(RenderingMixin, TraceItem):
    """Traces Hide"""

    def __init__(self, producer, port, view):
        TraceItem.__init__(self)

        producer = sm._getPyProxy(producer)
        view = sm._getPyProxy(view)
        producerAccessor = Trace.get_accessor(producer)
        viewAccessor = Trace.get_accessor(view)

        Trace.Output.append_separated([ \
            "# hide data in view",
            "Hide(%s, %s)" % (str(producerAccessor), str(viewAccessor)) if port == 0 else \
                "Hide(OutputPort(%s, %d), %s)" % (str(producerAccessor), port, str(viewAccessor))])


class SetScalarColoring(RenderingMixin, TraceItem):
    """Trace vtkSMPVRepresentationProxy.SetScalarColoring"""

    def __init__(self, display, arrayname, attribute_type, component=None, separate=False, lut=None):
        TraceItem.__init__(self)

        self.Display = sm._getPyProxy(display)
        self.ArrayName = arrayname
        self.AttributeType = attribute_type
        self.Component = component
        self.Lut = sm._getPyProxy(lut)
        self.Separate = separate

    def finalize(self):
        TraceItem.finalize(self)

        if self.ArrayName:
            if self.Component is None:
                if self.Separate:
                    Trace.Output.append_separated([ \
                        "# set scalar coloring using an separate color/opacity maps",
                        "ColorBy(%s, ('%s', '%s'), %s)" % ( \
                            str(Trace.get_accessor(self.Display)),
                            sm.GetAssociationAsString(self.AttributeType),
                            self.ArrayName, self.Separate)])
                else:
                    Trace.Output.append_separated([ \
                        "# set scalar coloring",
                        "ColorBy(%s, ('%s', '%s'))" % ( \
                            str(Trace.get_accessor(self.Display)),
                            sm.GetAssociationAsString(self.AttributeType),
                            self.ArrayName)])
            else:
                if self.Separate:
                    Trace.Output.append_separated([ \
                        "# set scalar coloring using an separate color/opacity maps",
                        "ColorBy(%s, ('%s', '%s', '%s'), %s)" % ( \
                            str(Trace.get_accessor(self.Display)),
                            sm.GetAssociationAsString(self.AttributeType),
                            self.ArrayName, self.Component, self.Separate)])
                else:
                    Trace.Output.append_separated([ \
                        "# set scalar coloring",
                        "ColorBy(%s, ('%s', '%s', '%s'))" % ( \
                            str(Trace.get_accessor(self.Display)),
                            sm.GetAssociationAsString(self.AttributeType),
                            self.ArrayName, self.Component)])
        else:
            Trace.Output.append_separated([ \
                "# turn off scalar coloring",
                "ColorBy(%s, None)" % str(Trace.get_accessor(self.Display))])

        # only for "Fully Trace Supplemental Proxies" support
        if self.Lut:
            Trace.get_accessor(self.Lut)


class SetBlocksScalarColoring(RenderingMixin, TraceItem):
    """Trace vtkSMPVRepresentationProxy.SetBlocksScalarColoring"""

    def __init__(self, display, block_selectors, arrayname, attribute_type, component=None, separate=False, luts=None):
        TraceItem.__init__(self)

        self.Display = sm._getPyProxy(display)
        self.BlockSelectors = block_selectors
        self.ArrayName = arrayname
        self.AttributeType = attribute_type
        self.Component = component
        # a list of luts
        self.Luts = []
        if luts is not None:
            for lut in luts:
                self.Luts.append(sm._getPyProxy(lut))
        self.Separate = separate

    def finalize(self):
        TraceItem.finalize(self)

        if self.BlockSelectors:
            if self.ArrayName:
                if self.Component is None:
                    if self.Separate:
                        Trace.Output.append_separated([ \
                            "# set blocks scalar coloring using an separate color/opacity maps",
                            "ColorBlocksBy(%s, %s, ('%s', '%s'), %s)" % ( \
                                str(Trace.get_accessor(self.Display)),
                                str(self.BlockSelectors),
                                sm.GetAssociationAsString(self.AttributeType),
                                self.ArrayName, self.Separate)])
                    else:
                        Trace.Output.append_separated([ \
                            "# set blocks scalar coloring",
                            "ColorBlocksBy(%s, %s, ('%s', '%s'))" % ( \
                                str(Trace.get_accessor(self.Display)),
                                str(self.BlockSelectors),
                                sm.GetAssociationAsString(self.AttributeType),
                                self.ArrayName)])
                else:
                    if self.Separate:
                        Trace.Output.append_separated([ \
                            "# set blocks scalar coloring using an separate color/opacity maps",
                            "ColorBlocksBy(%s, %s, ('%s', '%s', '%s'), %s)" % ( \
                                str(Trace.get_accessor(self.Display)),
                                str(self.BlockSelectors),
                                sm.GetAssociationAsString(self.AttributeType),
                                self.ArrayName, self.Component, self.Separate)])
                    else:
                        Trace.Output.append_separated([ \
                            "# set blocks scalar coloring",
                            "ColorBlocksBy(%s, %s, ('%s', '%s', '%s'))" % ( \
                                str(Trace.get_accessor(self.Display)),
                                str(self.BlockSelectors),
                                sm.GetAssociationAsString(self.AttributeType),
                                self.ArrayName, self.Component)])
            else:
                Trace.Output.append_separated([ \
                    "# turn off blocks scalar coloring",
                    "ColorBlocksBy(%s, %s, None)" % (str(Trace.get_accessor(self.Display)), str(self.BlockSelectors))])

            # only for "Fully Trace Supplemental Proxies" support
            for i in range(len(self.Luts)):
                if self.Luts[i]:
                    Trace.get_accessor(self.Luts[i], self.BlockSelectors[i])


class RegisterViewProxy(RenderingMixin, TraceItem):
    """Traces creation of a new view (vtkSMParaViewPipelineController::RegisterViewProxy)."""

    def __init__(self, proxy):
        TraceItem.__init__(self)
        self.Proxy = sm._getPyProxy(proxy)
        assert not self.Proxy is None

    def finalize(self):
        pname = Trace.get_registered_name(self.Proxy, "views")
        varname = Trace.get_varname(pname)
        accessor = ProxyAccessor(varname, self.Proxy)

        trace = TraceOutput()
        # create dynamic lights as needed.
        if hasattr(self.Proxy, "AdditionalLights"):
            for light in self.Proxy.AdditionalLights:
                trace.append('# create light')
                lightTrace = RegisterLightProxy(light)
                lightTrace.finalize()

        # unlike for filters/sources, for views the CreateView function still takes the
        # xml name for the view, not its label.
        ctor_args = "'%s'" % self.Proxy.GetXMLName()
        trace.append("# Create a new '%s'" % self.Proxy.GetXMLLabel())
        filter = ViewProxyFilter()
        trace.append(accessor.trace_ctor("CreateView", filter, ctor_args))

        # append dynamic lights as needed.
        # if hasattr(self.Proxy, "AdditionalLights"):
        #     lightsList = []
        #     for light in self.Proxy.AdditionalLights:
        #         lightAccessor = Trace.get_accessor(light)
        #         lightsList.append(lightAccessor)
        #     trace.append("%s.AdditionalLights = [%s]" % (Trace.get_accessor(self.Proxy), ", ".join(lightsList)))

        Trace.Output.append_separated(trace.raw_data())
        # we assume views don't have proxy list domains for now, and ignore tracing them.
        TraceItem.finalize(self)


class RegisterLightProxy(RenderingMixin, TraceItem):
    """Traces creation of a new light (vtkSMParaViewPipelineController::RegisterLightProxy)."""

    def __init__(self, proxy, view=None):
        TraceItem.__init__(self)
        self.Proxy = sm._getPyProxy(proxy)
        self.View = sm._getPyProxy(view)
        assert not self.Proxy is None

    def finalize(self):
        pname = Trace.get_registered_name(self.Proxy, "additional_lights")
        varname = Trace.get_varname(pname)
        accessor = ProxyAccessor(varname, self.Proxy)

        trace = TraceOutput()
        trace.append("# Create a new '%s'" % self.Proxy.GetXMLLabel())
        filter = ProxyFilter()
        if self.View:
            viewAccessor = Trace.get_accessor(self.View)
            trace.append(accessor.trace_ctor("AddLight", filter, ctor_args="view=%s" % viewAccessor))
        else:
            trace.append(accessor.trace_ctor("CreateLight", filter))
        Trace.Output.append_separated(trace.raw_data())
        TraceItem.finalize(self)


class RegisterTextureProxy(RenderingMixin, TraceItem):
    """Traces creation of a new texture (vtkSMParaViewPipelineController::RegisterTextureProxy)"""
    def __init__(self, proxy, filename, trivial_producer_key, proxyname):
        TraceItem.__init__(self)
        self.Proxy = sm._getPyProxy(proxy)
        self.filename = filename
        self.trivial_producer_key = trivial_producer_key
        self.proxyname = proxyname
        self.read_from_file = len(filename) and not len(trivial_producer_key)
        self.read_from_memory = not self.read_from_file
        assert not self.Proxy is None

    def finalize(self):
        pname = Trace.get_registered_name(self.Proxy, "textures")
        varname = Trace.get_varname(pname)
        accessor = ProxyAccessor(varname, self.Proxy)
        trace = TraceOutput()
        trace.append("# Create a new '%s'" % self.Proxy.GetXMLLabel())
        filter = ProxyFilter()
        if self.read_from_file:
            trace.append(accessor.trace_ctor("CreateTexture", filter, ctor_args=f"filename={self.filename}"))
        elif self.read_from_memory:
            ctor_str = f"filename=None, trivial_producer_key={self.trivial_producer_key}, proxyname={self.proxyname}"
            trace.append(accessor.trace_ctor("CreateTexture", filter, ctor_args=ctor_str))
        Trace.Output.append_separated(trace.raw_data())
        TraceItem.finalize(self)


class TraceAnimationProxy(RenderingMixin, TraceItem):
    """Traces all changes in an animation proxy."""

    def __init__(self, proxy):
        TraceItem.__init__(self)
        self.Proxy = sm._getPyProxy(proxy)
        assert not self.Proxy is None

    def finalize(self):
        # trace timekeeper
        if hasattr(self.Proxy, "TimeKeeper"):
            tkTrace = TraceProxy(self.Proxy.TimeKeeper, TimeKeeperProxyFilter(), "# initialize the timekeeper")
            tkTrace.finalize()

        # create dynamic animation cues as needed.
        if hasattr(self.Proxy, "Cues"):
            for cue in self.Proxy.Cues:
                cueTrace = TraceProxy(cue, AnimationProxyFilter(), "# initialize the animation track")
                cueTrace.finalize()

        animTrace = TraceProxy(self.Proxy, AnimationProxyFilter(), "# initialize the animation scene")
        animTrace.finalize()
        TraceItem.finalize(self)


class TraceProxy(TraceItem):
    """Traces all changes in a provided proxy."""

    def __init__(self, proxy, filter, description):
        TraceItem.__init__(self)
        self.Proxy = sm._getPyProxy(proxy)
        self.Filter = filter
        self.Description = description
        assert not self.Proxy is None

    def finalize(self):
        TraceItem.finalize(self)

        # We let Trace create an accessor for the proxy. We will then simply log the
        # default property values.
        accessor = Trace.get_accessor(self.Proxy)  # type: RealProxyAccessor

        # Now trace properties on the proxy.
        trace = TraceOutput()
        trace.append_separated(self.Description)
        trace.append(accessor.trace_ctor(None, self.Filter))
        Trace.Output.append_separated(trace.raw_data())


class ExportView(RenderingMixin, TraceItem):
    def __init__(self, view, exporter, filename):
        TraceItem.__init__(self)

        view = sm._getPyProxy(view)
        exporter = sm._getPyProxy(exporter)

        viewAccessor = Trace.get_accessor(view)
        exporterAccessor = ProxyAccessor("temporaryExporter", exporter)

        trace = TraceOutput()
        trace.append("# export view")
        trace.append( \
            exporterAccessor.trace_ctor("ExportView", ExporterProxyFilter(),
                                        ctor_args="%s, view=%s" % (repr(filename), viewAccessor),
                                        skip_assignment=True))
        exporterAccessor.finalize()  # so that it will get deleted
        del exporterAccessor
        Trace.Output.append_separated(trace.raw_data())


class ImportView(RenderingMixin, TraceItem):
    def __init__(self, view, importer, filename):
        TraceItem.__init__(self)

        view = sm._getPyProxy(view)
        importer = sm._getPyProxy(importer)

        viewAccessor = Trace.get_accessor(view)
        importerAccessor = ProxyAccessor("temporaryImporter", importer)

        trace = TraceOutput()
        trace.append("# import file")
        trace.append(\
            importerAccessor.trace_ctor("ImportView", ExporterProxyFilter(),
              ctor_args="%s, view=%s" % (repr(filename), viewAccessor),
              skip_assignment=True))
        importerAccessor.finalize() # so that it will get deleted
        del importerAccessor
        Trace.Output.append_separated(trace.raw_data())


class SaveData(TraceItem):
    def __init__(self, writer, filename, source, port):
        TraceItem.__init__(self)

        source = sm._getPyProxy(source, port)
        sourceAccessor = Trace.get_accessor(source)
        writer = sm._getPyProxy(writer)
        writerAccessor = ProxyAccessor("temporaryWriter", writer)

        if port > 0:
            ctor_args_1 = "OutputPort(%s, %d)" % (sourceAccessor, port)
        else:
            ctor_args_1 = "%s" % sourceAccessor

        trace = TraceOutput()
        trace.append("# save data")
        trace.append( \
            writerAccessor.trace_ctor("SaveData", WriterProxyFilter(),
                                      ctor_args="%s, proxy=%s" % (repr(filename), ctor_args_1),
                                      skip_assignment=True))
        writerAccessor.finalize()  # so that it will get deleted.
        del writerAccessor
        del writer
        Trace.Output.append_separated(trace.raw_data())


class SaveScreenshotOrAnimation(RenderingMixin, TraceItem):
    def __init__(self, helper, filename, view, layout, mode_screenshot=False, location=vtkPVSession.CLIENT):
        TraceItem.__init__(self)
        assert (view != None or layout != None)

        helper = sm._getPyProxy(helper)
        helperAccessor = ProxyAccessor("temporaryHelper", helper)

        if view:
            view = sm._getPyProxy(view)
            ctor_args_1 = "%s" % Trace.get_accessor(view)
        elif layout:
            layout = sm._getPyProxy(layout)
            ctor_args_1 = "%s" % Trace.get_accessor(layout)

        trace = TraceOutput()
        if mode_screenshot:
            trace.append("# save screenshot")
        else:
            trace.append("# save animation")

        _filter = ScreenShotHelperProxyFilter()

        # tracing "Format" is handled specially. PLD properties are not traced
        # in ctor, but we trick it as follows:
        formatAccessor = ProxyAccessor("temporaryHelperFormat", helper.Format)
        formatProps = [x for x in formatAccessor.get_properties() if _filter.should_trace_in_ctor(x)]
        format_txt = formatAccessor.trace_properties(formatProps, in_ctor=True)
        if format_txt:
            format_txt = "\n    # %s options\n    %s" % (helper.Format.GetXMLLabel(), format_txt)

        trace.append( \
            helperAccessor.trace_ctor( \
                "SaveScreenshot" if mode_screenshot else "SaveAnimation",
                ScreenShotHelperProxyFilter(),
                ctor_args="filename='%s', viewOrLayout=%s, location=%s" % (filename, ctor_args_1, location),
                ctor_extra_args=format_txt,
                skip_assignment=True))
        helperAccessor.finalize()
        del helperAccessor
        del helper
        Trace.Output.append_separated(trace.raw_data())


class SaveAnimationExtracts(TraceItem):
    """Used by vtkSMSaveAnimationExtractsProxy to trace saving of extracts
    generation."""

    def __init__(self, proxy):
        TraceItem.__init__(self)
        assert (proxy is not None)

        proxy = sm._getPyProxy(proxy)
        accessor = ProxyAccessor("temporaryWriter", proxy)

        trace = TraceOutput()
        trace.append("# save extracts")
        trace.append( \
            accessor.trace_ctor("SaveExtracts", SaveExtractsFilter(),
                                skip_assignment=True))
        del accessor
        del proxy
        Trace.Output.append_separated(trace.raw_data())


class LoadState(TraceItem):
    def __init__(self, filename, options):
        TraceItem.__init__(self)
        self._filename = filename
        self._options = options

    def finalize(self):
        import re

        filename = self._filename
        options = self._options

        options = sm._getPyProxy(options)
        trace = TraceOutput()
        mode = options.SMProxy.GetProperty("LoadStateDataFileOptions").GetElement(0)
        if mode == options.SMProxy.USE_FILES_FROM_STATE:
            trace.append("# load state")
            trace.append("LoadState('%s')" % filename)
        elif mode == options.SMProxy.USE_DATA_DIRECTORY:
            restrict_to_data_directory = 'True' if options.OnlyUseFilesInDataDirectory else 'False'
            trace.append("# load state using data from chosen directory")
            trace.append( \
                ["LoadState('%s'," % filename,
                 "    data_directory='%s'," % options.DataDirectory,
                 "    restrict_to_data_directory=%s)" % restrict_to_data_directory])
        elif mode == options.SMProxy.CHOOSE_FILES_EXPLICITLY:
            iter = sm.PropertyIterator(options)
            params = {}

            # Get the property groups constructed for each reader. We'll parse the label
            # of the group to get the file name and proxy id in the XML. This label is
            # formatted and set in vtkSMLoadStateOptionsProxy::vtkInternals::AddProperties()
            for i in range(options.GetNumberOfPropertyGroups()):
                group = options.GetPropertyGroup(i)
                smprop = group.GetProperty(0)
                property_name = group.GetPropertyName(0)
                group_label = group.GetXMLLabel()
                m = re.search(r'^([^(]+).*id=(\d+)', group_label)
                registration_name = m.group(1)[:-1]
                sid = int(m.group(2))
                if m and options.IsPropertyModified(sid, property_name):
                    readername = options.GetReaderName(sid)
                    d = params.get(sid, {'name': registration_name, 'id': sid})
                    if smprop.GetNumberOfElements() == 1:
                        d[property_name] = smprop.GetElement(0)
                    else:
                        d[property_name] = [smprop.GetElement(x) for x in range(smprop.GetNumberOfElements())]
                    params[sid] = d
            if params:
                trace.append("# load state using specified data files")
                trace.append("LoadState('%s'," % filename)
                trace.append(_pretty_print(list(params.values()), prefix="    filenames=") + ")")
            else:
                # this happens when user didn't modify any paths.
                trace.append("# load state")
                trace.append("LoadState('%s')" % filename)
        del options
        Trace.Output.append_separated(trace.raw_data())


class RegisterLayoutProxy(RenderingMixin, TraceItem):
    def __init__(self, layout):
        TraceItem.__init__(self)
        self.Layout = sm._getPyProxy(layout)

    def finalize(self, filter=None):
        if filter is None:
            filter = lambda x: True
        pname = Trace.get_registered_name(self.Layout, "layouts")
        accessor = ProxyAccessor(Trace.get_varname(pname), self.Layout)
        Trace.Output.append_separated([ \
            "# create new layout object '%s'" % pname,
            "%s = CreateLayout(name='%s')" % (accessor, pname)])

        # Let's trace out the state for the layout.
        def _trace_layout(layout, laccessor, location):
            sdir = layout.GetSplitDirection(location)
            sfraction = layout.GetSplitFraction(location)
            if sdir == layout.SMProxy.VERTICAL:
                Trace.Output.append([ \
                    "%s.SplitVertical(%d, %f)" % (laccessor, location, sfraction)])
                _trace_layout(layout, laccessor, layout.GetFirstChild(location))
                _trace_layout(layout, laccessor, layout.GetSecondChild(location))
            elif sdir == layout.SMProxy.HORIZONTAL:
                Trace.Output.append([ \
                    "%s.SplitHorizontal(%d, %f)" % (laccessor, location, sfraction)])
                _trace_layout(layout, laccessor, layout.GetFirstChild(location))
                _trace_layout(layout, laccessor, layout.GetSecondChild(location))
            elif sdir == layout.SMProxy.NONE:
                view = layout.GetView(location)
                if view and filter(view):
                    vaccessor = Trace.get_accessor(view)
                    Trace.Output.append([ \
                        "%s.AssignView(%d, %s)" % (laccessor, location, vaccessor)])

        _trace_layout(self.Layout, accessor, 0)

        # save size, if non-empty.
        lsize = self.Layout.GetSize()
        if lsize[0] > 0 and lsize[1] > 0:
            Trace.Output.append("%s.SetSize(%d, %d)" % (accessor, lsize[0], lsize[1]))
        TraceItem.finalize(self)


class LoadPlugin(TraceItem):
    def __init__(self, filename, remote):
        Trace.Output.append_separated([ \
            "# load plugin",
            "LoadPlugin('%s', remote=%s, ns=globals())" % (filename, remote)])


class CreateAnimationTrack(TraceProxy):
    # FIXME: animation tracing support in general needs to be revamped after moving
    # animation control logic to the server manager from Qt layer.
    def __init__(self, cue):
        TraceProxy.__init__(self, cue, AnimationProxyFilter(), "# initialize the animation track")


class RenameProxy(TraceItem):
    "Trace renaming of a source proxy."

    def __init__(self, proxy):
        TraceItem.__init__(self)
        proxy = sm._getPyProxy(proxy)

        if Trace.get_registered_name(proxy, "sources"):
            self.Accessor = Trace.get_accessor(proxy)
            self.Proxy = proxy
        else:
            raise Untraceable("Only source proxy renames are traced.")

    def finalize(self):
        if self.Accessor:
            newname = Trace.get_registered_name(self.Proxy, "sources")
            Trace.Output.append_separated([ \
                "# rename source object",
                "RenameSource('%s', %s)" % (newname, self.Accessor)])
        TraceItem.finalize(self)


class SetCurrentProxy(TraceItem):
    """Traces change in active view/source etc."""

    def __init__(self, selmodel, proxy, command):
        TraceItem.__init__(self)
        if proxy and proxy.IsA("vtkSMOutputPort"):
            # FIXME: need to handle port number.
            proxy = sm._getPyProxy(proxy.GetSourceProxy())
        else:
            proxy = sm._getPyProxy(proxy)
        accessor = Trace.get_accessor(proxy)
        pxm = selmodel.GetSessionProxyManager()
        if selmodel is pxm.GetSelectionModel("ActiveView"):
            if RenderingMixin().skip_from_trace:
                raise Untraceable("skipped")
            Trace.Output.append_separated([ \
                "# set active view",
                "SetActiveView(%s)" % accessor])
        elif selmodel is pxm.GetSelectionModel("ActiveSources"):
            Trace.Output.append_separated([ \
                "# set active source",
                "SetActiveSource(%s)" % accessor])
        else:
            raise Untraceable("Unknown selection model")


class CallMethod(TraceItem):
    def __init__(self, proxy, methodname, *args, **kwargs):
        TraceItem.__init__(self)
        trace = self.get_trace(proxy, methodname, args, kwargs)
        if trace:
            Trace.Output.append_separated(trace)

    def get_trace(self, proxy, methodname, args, kwargs):
        to_trace = []
        try:
            to_trace.append("# " + kwargs["comment"])
            del kwargs["comment"]
        except KeyError:
            pass
        accessor = Trace.get_accessor(sm._getPyProxy(proxy))
        args = [str(CallMethod.marshall(x)) for x in args]
        args += ["%s=%s" % (key, CallMethod.marshall(val)) for key, val in kwargs.items()]
        to_trace.append("%s.%s(%s)" % (accessor, methodname, ", ".join(args)))
        return to_trace

    @classmethod
    def marshall(cls, x):
        try:
            if x.IsA("vtkSMProxy"):
                return Trace.get_accessor(sm._getPyProxy(x))
        except AttributeError:
            return "'%s'" % x if type(x) == str else x


def _bind_on_event(ref):
    def _callback(obj, string):
        ref().on_event(obj, string)

    return _callback


class CallMethodIfPropertiesModified(CallMethod):
    """Similar to CallMethod, except that the trace will get logged only
    if the proxy fires PropertiesModified event before the trace-item is
    finalized."""

    def __init__(self, proxy, methodname, *args, **kwargs):
        self.proxy = proxy
        self.methodname = methodname
        self.args = args
        self.kwargs = kwargs
        self.tag = proxy.AddObserver("PropertyModifiedEvent", _bind_on_event(weakref.ref(self)))
        self.modified = False

    def on_event(self, obj, string):
        self.modified = True

    def finalize(self):
        self.proxy.RemoveObserver(self.tag)
        self.tag = None
        if self.modified:
            trace = self.get_trace(self.proxy, self.methodname, self.args, self.kwargs)
            Trace.Output.append_separated(trace)
        CallMethod.finalize(self)

    def __del__(self):
        if self.proxy and self.tag:
            self.proxy.RemoveObserver(self.tag)


class CallFunction(TraceItem):
    def __init__(self, functionname, *args, **kwargs):
        TraceItem.__init__(self)
        to_trace = []
        try:
            to_trace.append("# " + kwargs["comment"])
            del kwargs["comment"]
        except KeyError:
            pass
        args = [str(CallMethod.marshall(x)) for x in args]
        args += ["%s=%s" % (key, CallMethod.marshall(val)) for key, val in kwargs.items()]
        to_trace.append("%s(%s)" % (functionname, ", ".join(args)))
        Trace.Output.append_separated(to_trace)


class TraceText(TraceItem):
    """Add text directly to the trace. For paraview client applications to use with non-proxy objects."""

    def __init__(self, text, *args, **kwargs):
        TraceItem.__init__(self)
        to_trace = []
        try:
            to_trace.append("# " + kwargs["comment"])
            del kwargs["comment"]
        except KeyError:
            pass
        to_trace.append(text)
        Trace.Output.append_separated(to_trace)


class ChooseTexture(RenderingMixin, TraceItem):
    """Traces changes of texture object selection. For example renderview background."""

    def __init__(self, owner, texture, prop):
        TraceItem.__init__(self)
        owner = sm._getPyProxy(owner)
        texture = sm._getPyProxy(texture)
        ownerAccessor = Trace.get_accessor(owner)
        textureAccessor = Trace.get_accessor(texture)

        Trace.Output.append_separated([ \
            "# change texture",
            "%s.%s = %s" % (str(ownerAccessor), prop.GetXMLName(), str(textureAccessor))])


class SaveCameras(RenderingMixin, BookkeepingItem):
    """This is used to request recording of cameras in trace"""

    # This is a little hackish at this point. We'll figure something cleaner out
    # in time.
    def __init__(self, proxy=None, **kwargs):
        trace = self.get_trace(proxy)
        try:
            Trace.Output.append(["# " + kwargs["comment"]])
            del kwargs["comment"]
        except KeyError:
            pass
        if trace:
            Trace.Output.append_separated(trace)

    @classmethod
    def get_trace(cls, proxy=None):
        trace = TraceOutput()
        proxy = sm._getPyProxy(proxy)
        if proxy is None:
            views = [x for x in simple.GetViews() if Trace.has_accessor(x)]
            for v in views:
                trace.append_separated(cls.get_trace(proxy=v))
        elif proxy.IsA("vtkSMViewLayoutProxy"):
            views = simple.GetViewsInLayout(proxy)
            for v in views:
                trace.append_separated(cls.get_trace(proxy=v))
        elif proxy.IsA("vtkSMViewProxy"):
            if proxy.GetProperty("CameraPosition"):
                accessor = Trace.get_accessor(proxy)
                prop_names = ["CameraPosition", "CameraFocalPoint",
                              "CameraViewUp", "CameraViewAngle",
                              "CameraParallelScale", "CameraParallelProjection",
                              "EyeAngle", "InteractionMode"]
                props = [x for x in accessor.get_properties() \
                         if x.get_property_name() in prop_names and \
                         not x.get_object().IsValueDefault()]
                if props:
                    trace.append("# current camera placement for %s" % accessor)
                    trace.append(accessor.trace_properties(props, in_ctor=False))
            else:
                pass  # non-camera views
        elif proxy.IsA("vtkSMAnimationSceneProxy"):
            for view in proxy.GetProperty("ViewModules"):
                trace.append_separated(cls.get_trace(proxy=view))
        else:
            raise Untraceable("Invalid argument type %r" % proxy)
        return trace.raw_data()


class SaveLayoutSizes(RenderingMixin, BookkeepingItem):
    """
    A bookkeeping item to trace sizes for all layouts used by the trace (or
    the one explicitly passed to the constructor).

    This ensures that all the layouts (and consequently views in those layout)
    are setup with sizes similar to those in the UI (BUG #20102).
    """

    def __init__(self, proxy=None):
        trace = self.get_trace(proxy)
        if trace:
            Trace.Output.append_separated(trace)

    @classmethod
    def get_trace(cls, proxy=None):
        trace = TraceOutput()
        proxy = sm._getPyProxy(proxy)
        if proxy is None:
            # scan all views we have created trace-accessors for and get layouts
            # for those views.
            views = [x for x in simple.GetViews() if Trace.has_accessor(x)]
            layouts = set([simple.GetLayout(v) for v in views \
                           if simple.GetLayout(v) is not None])
            for l in layouts:
                trace.append_separated(cls.get_trace(l))
        elif proxy.IsA("vtkSMViewLayoutProxy"):
            layoutAccessor = Trace.get_accessor(proxy)
            lsize = proxy.GetSize()
            trace.append_separated([ \
                "# layout/tab size in pixels",
                "%s.SetSize(%d, %d)" % (layoutAccessor, lsize[0], lsize[1])])
        elif proxy.IsA("vtkSMViewProxy"):
            l = simple.GetLayout(proxy)
            if l:
                trace.append_separated(cls.get_trace(l))
        else:
            raise Untraceable("Invalid argument type %r" % proxy)
        return trace.raw_data()


class CreateExtractor(TraceItem):
    """Traces creation of extractors"""

    def __init__(self, xmlname, producer, extractor, registrationName, comment=None):
        super(CreateExtractor, self).__init__()

        producer = sm._getPyProxy(producer)
        extractor = sm._getPyProxy(extractor)

        self.ProducerAccessor = Trace.get_accessor(producer)
        self.OutputPort = producer.Port
        self.Extractor = extractor
        self.XMLName = xmlname
        self.Comment = comment
        self.Name = registrationName

    def finalize(self):
        output = TraceOutput()
        port = self.OutputPort
        regName = self.Name
        accessor = ProxyAccessor(Trace.get_varname(regName), self.Extractor)

        if self.Comment is not None:
            output.append("# %s", self.Comment)
        else:
            output.append("# create extractor")

        if port > 0:
            output.append( \
                "%s = CreateExtractor('%s', OutputPort(%s, %d), registrationName='%s')" % \
                (str(accessor), self.XMLName, str(self.ProducerAccessor), port, regName))
        else:
            output.append( \
                "%s = CreateExtractor('%s', %s, registrationName='%s')" % \
                (str(accessor), self.XMLName, str(self.ProducerAccessor), regName))

        ctor_trace = accessor.trace_ctor(None, ExtractorFilter())
        if ctor_trace:
            output.append("# trace defaults for the extractor.")
            output.append(ctor_trace)
        Trace.Output.append_separated(output.raw_data())
        super(CreateExtractor, self).finalize()


# __ActiveTraceItems is simply used to keep track of items that are currently
# active to avoid non-nestable trace items from being created when previous
# items are active.
__ActiveTraceItems = []


def _create_trace_item_internal(key, args=None, kwargs=None):
    global __ActiveTraceItems

    # trim __ActiveTraceItems to remove None references.
    __ActiveTraceItems = [x for x in __ActiveTraceItems if not x() is None]

    g = globals()
    if key in g and callable(g[key]):
        args = args if args else []
        kwargs = kwargs if kwargs else {}
        traceitemtype = g[key]
        if len(__ActiveTraceItems) == 0 or \
                issubclass(traceitemtype, NestableTraceItem):
            if len(__ActiveTraceItems) > 0 and \
                    isinstance(__ActiveTraceItems[-1](), BlockTraceItems):
                raise Untraceable("Not tracing since `BlockTraceItems` is active.")
            instance = traceitemtype(*args, **kwargs)
            if not issubclass(traceitemtype, BookkeepingItem):
                __ActiveTraceItems.append(weakref.ref(instance))
            return instance
        raise Untraceable("Non-nestable trace item. Ignoring in current context.")
    raise Untraceable("Unknown trace item type %s" % key)
    # print ("Hello again", key, args)
    # return A(key)


def _start_trace_internal(preamble=None):
    """**internal** starts tracing. Called by vtkSMTrace::StartTrace()."""
    Trace.reset()
    if preamble:
        Trace.Output.append(preamble)
    Trace.Output.append([ \
        "#### import the simple module from the paraview",
        "from paraview.simple import *"])
    if not _get_skip_rendering():
        Trace.Output.append([ \
            "#### disable automatic camera reset on 'Show'",
            "paraview.simple._DisableFirstRenderCameraReset()"])

    return True


def _stop_trace_internal():
    """**internal** stops trace. Called by vtkSMTrace::StopTrace()."""
    if not _get_skip_rendering():
        # ensure we trace the active view, so camera changes will be recorded.
        Trace.get_accessor(simple.GetActiveView())

        Trace.Output.append_separated([ \
            "#================================================================",
            "# addendum: following script captures some of the application",
            "# state to faithfully reproduce the visualization during playback",
            "#================================================================"])

        # save layout sizes
        layout_trace = SaveLayoutSizes.get_trace(None)
        if layout_trace:
            Trace.Output.append_separated([ \
                "#--------------------------------",
                "# saving layout sizes for layouts"])
            Trace.Output.append_separated(layout_trace)

        camera_trace = SaveCameras.get_trace(None)
        if camera_trace:
            Trace.Output.append_separated([ \
                "#-----------------------------------",
                "# saving camera placements for views"])
            Trace.Output.append_separated(camera_trace)
        Trace.Output.append_separated(_get_standard_postamble_comment())
    trace = str(Trace.Output)
    Trace.reset()

    # essential to ensure any obsolete accessor don't linger can cause havoc
    # when saving state following a Python trace session
    # (paraview/paraview#18994)
    import gc
    gc.collect()
    gc.collect()
    return trace


def _get_standard_postamble_comment():
    """**internal** get a standard postamble comment."""
    return """
##--------------------------------------------
## You may need to add some code at the end of this python script depending on your usage, eg:
#
## Render all views to see them appears
# RenderAllViews()
#
## Interact with the view, usefull when running from pvpython
# Interact()
#
## Save a screenshot of the active view
# SaveScreenshot("path/to/screenshot.png")
#
## Save a screenshot of a layout (multiple splitted view)
# SaveScreenshot("path/to/screenshot.png", GetLayout())
#
## Save all "Extractors" from the pipeline browser
# SaveExtracts()
#
## Save a animation of the current active view
# SaveAnimation()
#
## Please refer to the documentation of paraview.simple
## https://www.paraview.org/paraview-docs/latest/python/paraview.simple.html
##--------------------------------------------"""


# ------------------------------------------------------------------------------
# Public methods
# ------------------------------------------------------------------------------
def start_trace(preamble=None):
    """Starting tracing.
    On successful start, will return a vtkSMTrace object.
    One can set tracing options on it to control how the tracing. If tracing was
    already started, calling this contine with the same trace.
    NOTE: please prefer using ScopedTracer, to ensure stop_trace call."""
    return sm.vtkSMTrace.StartTrace(preamble)


def stop_trace():
    """Stops the trace and returns the generated trace output string.
    NOTE: please prefer using ScopedTracer"""
    return sm.vtkSMTrace.StopTrace()


def get_current_trace_output(raw=False):
    """Returns the trace generated so far in the tracing process."""
    return str(Trace.Output) if not raw else Trace.Output.raw_data()


def get_current_trace_output_and_reset(raw=False):
    """
    Equivalent to calling the following code:

        get_current_trace_output(raw)
        reset_trace_output()

    """

    output = get_current_trace_output(raw)
    reset_trace_output()
    return output


def reset_trace_output():
    """Resets the trace output without resetting the tracing datastructures
    themselves."""
    Trace.Output.reset()


# ------------------------------------------------------------------------------
class ScopedTracer():
    """A tracer class that implements context management,
    to ensure start_trace() call on enter and stop_trace on exit.
    """
    def __init__(self, preamble = ""):
        self._preamble = preamble
        self.config = None
        self._last_trace = ""

    def __enter__(self):
        self.config = start_trace(self._preamble)

    def __exit__(self, type, value, tb):
        self._last_trace = stop_trace()
        self.config = None

    def last_trace(self):
        return self._last_trace


# ------------------------------------------------------------------------------
if __name__ == "__main__":
    print("Running test")
    tracer = ScopedTracer("")
    with tracer.trace() as active_trace:
        s = simple.Sphere()
        c = simple.Clip()
        simple.Show()
        print("***** TRACE RESULT *****")

    print(tracer.last_trace())
