/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSource - A super class for filter objects.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.
// This class should probably be merged with vtkPVComposite.


#ifndef __vtkPVSource_h
#define __vtkPVSource_h

#include "vtkPVTracedWidget.h"
#include "vtkClientServerStream.h"  // needed for vtkClientServerID

class vtkCollection;
class vtkKWFrame;
class vtkKWFrameWithScrollbar;
class vtkKWView;
class vtkPVApplication;
class vtkPVColorMap;
class vtkPVDataInformation;
class vtkPVDataSetAttributesInformation;
class vtkPVDisplayGUI;
class vtkPVInputProperty;
class vtkPVLookmark;
class vtkPVNumberOfOutputsInformation;
class vtkPVRenderView;
class vtkPVSourceNotebook;
class vtkPVWidget;
class vtkPVWidgetCollection;
class vtkPVWindow;
class vtkSMCubeAxesDisplayProxy;
class vtkSMDataObjectDisplayProxy;
class vtkSMDisplayProxy;
class vtkSMPart;
class vtkSMPointLabelDisplayProxy;
class vtkSMProxy;
class vtkSMSourceProxy;
class vtkSource;
class vtkStringList;


class VTK_EXPORT vtkPVSource : public vtkPVTracedWidget
{
public:
  static vtkPVSource* New();
  vtkTypeRevisionMacro(vtkPVSource,vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the Window for this class.  This is used for creating input menus.
  // The PVSource list is in the window.
  vtkPVWindow *GetPVWindow();
  
  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties();

  // Description:
  // This method updates the piece that has been assigned to this process.
  // It update all parts and gathers data information.
  void Update();

  // Description:
  // Methods to indicate when this source is selected in the window..
  // The second version of Deselect allows to user to specify whether
  // the main frame of the source parameters should be unpacked or
  // not. It was necessary to add this option to work around some
  // Tk packing problems.
  virtual void Select();
  virtual void Deselect(int doPackForget = 1);
  virtual void Pack();

  // Description: 
  // This flag turns the visibility of the prop on and off.
  // These methods transmit the state change to all of the satellite
  // processes.
  void SetVisibility(int v);
  int GetVisibility();
  virtual void SetVisibilityNoTrace(int v);
  
  // Description:
  // If we are the last source to unregister a color map,
  // this method will turn its scalar bar visibility off.
  void SetPVColorMap(vtkPVColorMap *colorMap);  
  
  // Description:
  // This method sets the color mode based on arrays set in scalars and
  // the input color mode.  The source must be current to call this method.
  // The rules are:
  // If the source created a NEW point scalar array, use it.
  // Else if the source created a NEW cell scalar array, use it.
  // Else if the input clolor by array exists in this source, use it.
  // Else color by property.
  void SetDefaultColorParameters();
  
  // Description:
  // Actual cube axis visibility is a combination of this variable and
  // overall visibility.
  void SetCubeAxesVisibility(int val);
  void SetCubeAxesVisibilityNoTrace(int val);
  vtkGetMacro(CubeAxesVisibility, int);

  // Description:
  // Using a display for point labels.
  void SetPointLabelVisibility(int val);
  void SetPointLabelVisibilityNoTrace(int val);
  vtkGetMacro(PointLabelVisibility, int);

  void SetPointLabelFontSize(int val);
  void SetPointLabelFontSizeNoTrace(int val);
  vtkGetMacro(PointLabelFontSize, int);

  // Description:
  // Connect an input to this pvsource. 
  // Note: SetPVInput() first disconnects all inputs of the
  // internal VTK filter whereas AddPVInput() does not.
  // See vtkPVGroupInputsWidgets for an example of how
  // AddPVInput() is used.
  void AddPVInput(vtkPVSource *input);
  void SetPVInput(const char* name, int idx, vtkPVSource *input);

  // Description:
  // Return an input given idx.
  vtkPVSource* GetPVInput(int idx) {return this->GetNthPVInput(idx);}

  // Description:
  // Return the number of inputs.
  vtkGetMacro(NumberOfPVInputs, int);

  // Description:
  // Remove all inputs of this pvsource and disconnect all the
  // inputs of all VTK filters in this pvsource.
  void RemoveAllPVInputs();

  // Description:
  // Get the number of consumers
  vtkGetMacro(NumberOfPVConsumers, int);

  // Description:
  // Add, remove, get, or check a consumer.
  void AddPVConsumer(vtkPVSource *c);
  void RemovePVConsumer(vtkPVSource *c);
  vtkPVSource *GetPVConsumer(int i);
  int IsPVConsumer(vtkPVSource *c);

  // Description:
  // For legacy scripts
  vtkPVDisplayGUI* GetPVOutput();
  
  // Description:
  // Access to this object from a script.
  vtkGetObjectMacro(PVColorMap, vtkPVColorMap);
   
  // Description:
  // This name is used in the source list to identify this source.
  // This name is passed to vtkPVWindow::GetPVSource() to get a
  // particular source.
  virtual void SetName(const char *name);
  char* GetName();

  //Description:
  //Overrides the AutoAccept. Very usefull for class like vtkArrayCalculator
  vtkSetMacro(OverideAutoAccept,int);
  vtkGetMacro(OverideAutoAccept,int);
    
  // Description:
  // The (short) label that can be used to give a more descriptive
  // name to the object. Note that if the label is empty when
  // GetLabel() is called, the Ivar is automatically initialized to
  // the name of the composite (GetName()).
  virtual void SetLabelNoTrace(const char *label);
  virtual void SetLabel(const char *label);
  virtual char* GetLabel();
  virtual void SetLabelOnce(const char *label);
    
  // Description:
  // This just returns the application typecast correctly.
  vtkPVApplication* GetPVApplication();  

  // Description:
  // Called when the accept button is pressed.
  void AcceptCallback();
  virtual void PreAcceptCallback();

  // Description:
  // Determine if this source can be deleted. This depends
  // on two conditions:
  // 1. Does the source have any consumers,
  // 2. Is it permanent (for example, a glyph source is permanent)
  virtual int IsDeletable();

  // Description:
  // Internal method; called by AcceptCallbackInternal.
  // Hide flag is used for hiding creation of 
  // the glyph sources from the user.
  virtual void Accept() { this->Accept(0); }
  virtual void Accept(int hideFlag) { this->Accept(hideFlag, 1); }
  virtual void Accept(int hideFlag, int hideSource);
  
  // Description:
  // Called when the reset button is pressed.
  void ResetCallback();

  // Description:
  // Called when the delete button is pressed.
  virtual void DeleteCallback();

  // Description:
  // This method resets the UI values (Widgets added with the following
  // methods).  It uses the GetCommand supplied to the interface.
  virtual void Reset();
  
  // Description:
  // Updates delete button and description frame.
  void UpdateProperties();  

  // Description:
  // This method gets called to set the VTK source parameters
  // from the widget values.
  virtual void UpdateVTKSourceParameters();

  //BTX
  // Description:
  // Given and ID, add a VTK source to the list of maintained
  // VTK sources.
  void AddVTKSource(vtkClientServerID);
  //ETX

  // Description:
  // Returns the number of VTK sources referenced by the PVSource.
  int GetNumberOfVTKSources();

  //BTX
  // Description:
  // Given an index, return the ID of a VTK source.
  vtkClientServerID GetVTKSourceID(int idx);
  //ETX
  // Description:
  // Give an index, return the ID of a VTK source as unsigned int.
  unsigned int GetVTKSourceIDAsInt(int idx);

  //BTX
  vtkGetObjectMacro(Widgets, vtkPVWidgetCollection);
  //ETX
  
  // Description:
  // Save the pipeline to a batch file which can be run without
  // a user interface.
  virtual void SaveInBatchScript(ofstream *file);

  // Description:
  // Saves the pipeline in a ParaView script.  This is similar
  // to saveing a trace, except only the last state is stored.
  virtual void SaveState(ofstream *file);
  virtual void SaveStateVisibility(ofstream *file);
  virtual void SaveStateDisplay(ofstream* file);

  
  // Description:
  // This flag determines whether a source will make its input invisible or
  // not.  By default, this flag is on.
  vtkSetMacro(ReplaceInput, int);
  vtkGetMacro(ReplaceInput, int);
  vtkBooleanMacro(ReplaceInput, int);

  // Description:
  // Access to PVWidgets indexed by their name.
  vtkPVWidget *GetPVWidget(const char *name);

  // Description:
  // These are used for drawing the navigation window.
  vtkPVSource **GetPVInputs() { return this->PVInputs; };

  // Description:
  // The notebook that is displayed when the source is selected.
  void SetNotebook(vtkPVSourceNotebook* notebook);
  vtkGetObjectMacro(Notebook, vtkPVSourceNotebook);

  // Desription:
  // This is just a flag that is used to mark that the source has been saved
  // into the tcl script (visited) during the recursive saving process.
  vtkSetMacro(VisitedFlag,int);
  vtkGetMacro(VisitedFlag,int);

  // Description:
  // This adds the PVWidget and sets up the callbacks to initialize its trace.
  void AddPVWidget(vtkPVWidget *pvw);

  // Description: 
  // Convenience methid which calls ClonePrototype() and InitializeClone()
  int CloneAndInitialize(int makeCurrent, vtkPVSource*& clone);

  // Description: 
  // Creates and returns (by reference) a copy of this
  // source. It will create a new instance of the same type as the current
  // object using NewInstance() and then call ClonePrototype() on all
  // widgets and add these clones to it's widget list. The return
  // value is VTK_OK is the cloning was successful.
  int ClonePrototype(vtkPVSource*& clone);

  // Description: 
  // This method is usually called on a clone created using ClonePrototype().
  // It: 1. sets the input, 2. calls CreateProperties(), 3. make the source
  // current (if makeCurrent is true), 4. creates output (vtkPVDisplayGUI) which
  // contains a vtk data object of type outputDataType, 5. assigns or
  // creates an extent translator to the output.
  virtual int InitializeClone(int makeCurrent);

  // Description:
  // This sets up the PVData.  This method is called when
  // the accept button is pressed for the first time.
  virtual int InitializeData();  

  // Description:
  // This method can be used by subclasses of PVSource to do
  // special initialization (for example creating widgets). 
  // It is called by PVWindow before adding a prototype to the main list.
  virtual void InitializePrototype() {}

  // Description:
  // Description of the VTK filter.
  vtkSetStringMacro(SourceClassName);
  vtkGetStringMacro(SourceClassName);

  // Description:
  // Access to individual parts.
  vtkSMPart *GetPart() {return this->GetPart(0);} 
  vtkSMPart *GetPart(int idx); 
  int GetNumberOfParts();

  // Descriptions:
  // Properties that describe the inputs to the filter of type
  // "SourceCLassName".  Name is the string used to format
  // Set/Add/Get methods.  "Input" or "Source" ...
  // First you get the input property using the name,
  // then you and fill it in (name is already set).
  // The first get creates the property and returns it.
  // The order the properties are created are important.
  // The first one will be the default input.
  vtkIdType GetNumberOfInputProperties();
  vtkPVInputProperty* GetInputProperty(int idx);
  vtkPVInputProperty* GetInputProperty(const char* name);

  // Description:
  // Option set by the xml filter input element "quantity".
  // When 1 (xml: Multiple), then VTK source takes one input which has multiple
  // inputs (uses AddInput).  
  // If 0, then PVSource Inputs match each VTK filter inputs.
  vtkSetMacro(VTKMultipleInputsFlag, int);
  vtkGetMacro(VTKMultipleInputsFlag, int);
  
  // Description:
  // Option set by the xml filter input element "multiprocess_support".
  // When 0 (xml: single_process), the VTK source only runs in a single
  // process.
  // If 1, (xml: multiple_processes), the VTK source only runs in parallel.
  // If 2, (xml: both), the VTK source runs in either mode.
  vtkSetMacro(VTKMultipleProcessFlag, int);
  vtkGetMacro(VTKMultipleProcessFlag, int);
  
  // Description:
  // Determine whether the single- or multi-process support for this filter
  // matches the current state of ParaView.
  // Returns 1 if this source can run with the current number of processors;
  // if not, returns 0.
  int GetNumberOfProcessorsValid();
  
  // Description:
  // Certain modules are not deletable (for example, glyph sources).
  // Such modules should be marked as such (IsPermanent = 1).
  vtkSetMacro(IsPermanent, int);
  vtkGetMacro(IsPermanent, int);
  vtkBooleanMacro(IsPermanent, int);

  // Description:
  // Set or get the module name. This name is used to store the
  // prototype in the sources/filters/readers maps. It is passed
  // to CreatePVSource when creating a new instance.
  vtkSetStringMacro(ModuleName);
  vtkGetStringMacro(ModuleName);

  // Description:
  // Set or get the label to be used in the Source/Filter menus.
  vtkSetStringMacro(MenuName);
  vtkGetStringMacro(MenuName);

  // Description:
  // A short help string describing the module.
  vtkSetStringMacro(ShortHelp);
  vtkGetStringMacro(ShortHelp);

  // Description:
  // A longer help string describing the module.
  vtkSetStringMacro(LongHelp);
  vtkGetStringMacro(LongHelp);

  // Description:
  // Check whether the source has been initialized 
  // (Accept has been called at least one)
  vtkGetMacro(Initialized, int);

  // Description:
  // If ToolbarModule is true, the instances of this module
  // can be created by using a button on the toolbar. This
  // variable is used mainly to prevent the addition of such
  // modules to Advanced->(Filter/Source) menus.
  vtkSetMacro(ToolbarModule, int);
  vtkGetMacro(ToolbarModule, int);
  vtkBooleanMacro(ToolbarModule, int);

  // Description:
  // When a module is first created, the user should not be able
  // to use certain menus, buttons etc. before accepting the first
  // time or deleting the module. GrabFocus() is used to tell the
  // module to disable certain things in the window.
  void GrabFocus();
  void UnGrabFocus();

  // Description:
  // Moving away from direct access to VTK data objects.
  vtkPVDataInformation* GetDataInformation();
  
  // Description:
  // Called by source EndEvent to schedule another Gather.
  void InvalidateDataInformation();

  // Description:
  // Convenience method for rendering.
  vtkPVRenderView *GetPVRenderView();

  // Description:
  // Access to the vtkPVNumberOfOutputsInformation object.
  vtkGetObjectMacro(NumberOfOutputsInformation,
                    vtkPVNumberOfOutputsInformation);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Is the source grabbed.
  vtkGetMacro(SourceGrabbed, int);
 
  // Description:
  // Returns the proxy object (server manager) used
  // by PVSource.
  void SetProxy(vtkSMSourceProxy* proxy);
  vtkGetObjectMacro(Proxy, vtkSMSourceProxy);

  // Description:
  void RegisterProxy(const char* sourceList, vtkPVSource* clone);

  // Description:
  // Calls UpdateVTKObjects on the proxy as well as on all widgets.
  virtual void UpdateVTKObjects();
  
  // Description
  // This is the Display for this source.
  void SetDisplayProxy(vtkSMDataObjectDisplayProxy* pdisp);
  vtkGetObjectMacro(DisplayProxy, vtkSMDataObjectDisplayProxy);

  // Description
  // Accessor to the point label text display.
  vtkGetObjectMacro(PointLabelDisplayProxy, vtkSMPointLabelDisplayProxy);

  // Description:
  // Set the volume rendering array and Scalar Mode.
  void VolumeRenderByArray(const char* arrayname, int field);

  // Description:
  // Sets the array name and field type to color with.
  // This also sets the LUT on the mapper.
  // field (3 == PointFieldData, 4==CellFieldData)
  // enums in vtkSMDataObjectDisplayProxy
  void ColorByArray(const char* arrayname, int field);

  // Description:
  // This method is now in the vtkPVSourceNotebook.
  // This legacy method is for backward compatibility.
  void SetAcceptButtonColorToModified();

  // Description:
  virtual void MarkSourcesForUpdate();

  // Description
  // If this source was created from a lookmark (state script), need to delete it from the lookmark's
  // source collection when manually deleted by user (otherwise we end up with some funky residual images
  // in render window as well as crashes when modifying an input of this source).
  void SetLookmark(vtkPVLookmark *lookmark);
  vtkGetObjectMacro(Lookmark,vtkPVLookmark);

protected:
  vtkPVSource();
  ~vtkPVSource();

  // Description:
  // Setups up the displays for this source
  virtual void SetupDisplays();
  virtual void CleanupDisplays();

  virtual void SaveStateDisplayInternal(ofstream *file, 
    const char* display_tcl_name, vtkSMProxy* display);

  // Description:
  // Set the color map and the field to use.
  void ColorByArray(vtkPVColorMap* colorMap, int field);

  void SetPVInputInternal(const char* name, 
                          int idx, 
                          vtkPVSource *input, 
                          int doInit);

  void SaveFilterInBatchScript(ofstream *file);

  virtual void InitializeWidgets();  


  vtkPVColorMap *PVColorMap;
  
  int DataInformationValid;

  vtkPVNumberOfOutputsInformation *NumberOfOutputsInformation;
  
  // This flag gets set after the user hits accept for the first time.
  int Initialized;

  // This is different than the CubeAxesDisplay visibility.
  // This is the state of the user selection. The CubeAxesDisplay
  // visibility is the value "visibility and CubeAxesVisibility".
  int CubeAxesVisibility;

  // This is different than the PointLabelDisplay visibility.
  // This is the state of the user selection. The PointLabelDisplay
  // visibility is the value "visibility and PointLabelVisibility".
  int PointLabelVisibility;
  int PointLabelFontSize;

  vtkKWFrameWithScrollbar *ParameterFrame;
  vtkSMDataObjectDisplayProxy* DisplayProxy;

  // Description:
  // Helper methods for subclasses to add displays to the
  // render module.
  void AddDisplayToRenderModule(vtkSMDisplayProxy* pDisp);
  void RemoveDisplayFromRenderModule(vtkSMDisplayProxy* pDisp);

  // Called to allocate the input array.  Copies old inputs.
  void SetNumberOfPVInputs(int num);
  vtkPVSource **PVInputs;
  int NumberOfPVInputs; 
  vtkPVSource *GetNthPVInput(int idx);
  void SetNthPVInput(int idx, vtkPVSource *input);

  // Keep a list of sources that are using this data.
  vtkPVSource **PVConsumers;
  int NumberOfPVConsumers;
     
  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();
  
  vtkPVSourceNotebook *Notebook;
  // Since the notebook is share between all sources,
  // we remember the raised page to restore when the source is selected.
  // This ivar is set and used by vtkPVWindow.
  int SavedRaisedNotebookPageId;

  char* SourceList;
  vtkSetStringMacro(SourceList);
  vtkGetStringMacro(SourceList);

  // The name is just for display.
  char      *Name;
  char      *MenuName;
  char      *Label;
  char      *ShortHelp;
  char      *LongHelp;
  int       OverideAutoAccept;

  // This is the module name.
  char      *ModuleName;

  
  vtkPVWidgetCollection *Widgets;

  char *SourceClassName;

  vtkSMSourceProxy* Proxy;

  // Whether the source should make its input invisible.
  int ReplaceInput;

  int VisitedFlag;

  // Number of instances cloned from this prototype
  int PrototypeInstanceCount;

  int IsPermanent;
  
  // Flag for whether we are currently in AcceptCallback.
  int AcceptCallbackFlag;

  // Flag to tell whether the source is grabbed or not.
  int SourceGrabbed;

  virtual int ClonePrototypeInternal(vtkPVSource*& clone);

  int ToolbarModule;

  // This ivar is here so that Probe will not have any sources which
  // have more than one part. A value of -1 means no restrictions.
  int RequiredNumberOfInputParts;
  // This value is set by the xml <input quantity="..."/> attributes
  // to characterize the VTK source.  a value of 1 (set by "Multiple")
  // indicates any number of inputs can be had with the AddInput method.
  int VTKMultipleInputsFlag;
  vtkCollection* InputProperties;

  int VTKMultipleProcessFlag;
  
  // Taking responsibility of saving inputs away from input menu.
  void SetInputsInBatchScript(ofstream *file);

  // Save Widgets to batch script.
  virtual void SaveWidgetsInBatchScript(ofstream* file);

  int UpdateSourceInBatch;

  int LabelSetByUser;
  int ResetInSelect;
  
  // CubeAxes should be moved into a display of its own.
  vtkSMCubeAxesDisplayProxy* CubeAxesDisplayProxy;
  vtkSMPointLabelDisplayProxy* PointLabelDisplayProxy;

//BTX
  friend class vtkPVWindow;
//ETX 

  vtkPVLookmark *Lookmark;

  int ColorByScalars(
    vtkPVDataSetAttributesInformation* attrInfo,
    vtkPVDataSetAttributesInformation* inAttrInfo,
    int field);

private:
  vtkPVSource(const vtkPVSource&); // Not implemented
  void operator=(const vtkPVSource&); // Not implemented
};

#endif
