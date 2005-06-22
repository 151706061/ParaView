/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplication.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkToolkits.h"
#include "vtkPVConfig.h"
#include "vtkPVApplication.h"
#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIGroup.h"
#endif

#include "vtkPVServerInformation.h"
#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCollectionIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkIntArray.h"
#include "vtkKWApplicationSettingsInterface.h"
#include "vtkKWDialog.h"
#include "vtkKWEvent.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWPushButton.h"
#include "vtkKWSplashScreen.h"
#include "vtkLongArray.h"
#include "vtkMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPVApplicationSettingsInterface.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVConfig.h"
#include "vtkPVHelpPaths.h"
#include "vtkPVProcessModule.h"
#include "vtkPVProcessModuleGUIHelper.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSourceInterfaceDirectories.h"
#include "vtkPVTraceFileDialog.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProbeFilter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkShortArray.h"
#include "vtkTclUtil.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerInterpreter.h"
#include "vtkGraphicsFactory.h"
#include "vtkImagingFactory.h"
#include "vtkSocketController.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include "vtkProcessModule.h"
// #include "vtkPVRenderGroupDialog.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkPVServerFileDialog.h"
#include <stdarg.h>
#include <signal.h>
#include "vtkPVProgressHandler.h"

#include "vtkPVGUIClientOptions.h"

#ifdef _WIN32

#include "ParaViewRC.h"

#include "htmlhelp.h"
#include "direct.h"
#else
#include "unistd.h"
#endif

#include <vtkstd/vector>
#include <vtkstd/string>
#include <sys/stat.h>

#include <vtksys/SystemTools.hxx>

#define PVAPPLICATION_PROGRESS_TAG 31415

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVApplication);
vtkCxxRevisionMacro(vtkPVApplication, "1.365");

//----------------------------------------------------------------------------
//****************************************************************************
class vtkPVApplicationObserver : public vtkCommand
{
public:
  static vtkPVApplicationObserver *New() 
    {return new vtkPVApplicationObserver;};

  vtkPVApplicationObserver()
    {
    this->Application= 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
    void* calldata)
    {
    if ( this->Application)
      {
      this->Application->ExecuteEvent(wdg, event, calldata);
      }
    this->AbortFlagOn();
    }

  void SetApplication(vtkPVApplication* app)
    {
    this->Application = app;
    }

private:
  vtkPVApplication* Application;

};
//****************************************************************************
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
extern "C" int Vtkkwparaview_Init(Tcl_Interp *interp);
extern "C" int Vtkpvservermanagertcl_Init(Tcl_Interp *interp); 
extern "C" int Vtkpvservercommontcl_Init(Tcl_Interp *interp); 

vtkPVApplication* vtkPVApplication::MainApplication = 0;

static void vtkPVAppProcessMessage(vtkObject* vtkNotUsed(object),
                                   unsigned long vtkNotUsed(event), 
                                   void *clientdata, void *calldata)
{
  if (!clientdata || !calldata)
    {
    return;
    }
  vtkPVApplication *self = static_cast<vtkPVApplication*>( clientdata );
  const char* message = static_cast<char*>( calldata );
  cerr << "# Error or warning: " << message << endl;
  vtkPVTraceHelper::OutputSimpleEntry(
    self->GetTraceFile(), "# Error or warning:");
  size_t cc;
  ostrstream str;
  for ( cc= 0; cc < strlen(message); cc ++ )
    {
    str << message[cc];
    if ( message[cc] == '\n' )
      {
      str << "# ";
      }
    }
  str << ends;
  vtkPVTraceHelper::OutputEntry(
    self->GetTraceFile(), "# %s\n#", str.str());
  //cout << "# " << str.str() << endl;
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
// Output window which prints out the process id
// with the error or warning messages
class VTK_EXPORT vtkPVOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkPVOutputWindow,vtkOutputWindow);
  
  static vtkPVOutputWindow* New();


  void DisplayDebugText(const char* t)
    {
      this->PVDisplayText(t);
    }
  
  void DisplayWarningText(const char* t)
    {
      this->PVDisplayText(t);
    }
  
  void DisplayErrorText(const char* t)
    {
      this->PVDisplayText(t, 1);
    }
  
  void DisplayGenericWarningText(const char* t)
    {
      this->PVDisplayText(t);
    }
  void DisplayText(const char* t)
  {
    this->PVDisplayText(t, 0);
  }
  void PVDisplayText(const char* t, int error = 0)
  {
#ifdef _WIN32
    // if this is a windows application, then always
    // send the output to stderr.  Most running paraview
    // won't see the output, but if you use a terminal that supports
    // the output from a windows program like rxvt then you
    // can see the output in the console.
    cerr << t << "\n";
#endif
    if ( this->Application && this->Application->GetNumberOfWindows())
      {
      vtkKWWindowBase *win = this->Application->GetNthWindow(
        this->Application->GetNumberOfWindows() - 1);
      const char *message = strstr(t, "): ");
      char type[1024], file[1024];
      int line;
      sscanf(t, "%[^:]: In %[^,], line %d", type, file, &line);
      if ( message )
        {
        message += 3;
        char *rmessage = vtksys::SystemTools::DuplicateString(message);
        int last = strlen(rmessage) - 1;
        while ( last > 0 && 
                (rmessage[last] == ' ' || rmessage[last] == '\n' || 
                 rmessage[last] == '\r' || rmessage[last] == '\t') )
          {
          rmessage[last] = 0;
          last--;
          }
        char *buffer = new char[strlen(file) + strlen(rmessage) + 256];
        sprintf(buffer, "There was a VTK %s in file: %s (%d)\n %s", 
                (error ? "Error" : "Warning"),
                file, line,
                rmessage);
        if ( error )
          {
          win->ErrorMessage(buffer);
          if ( this->TestErrors )
            {
            this->ErrorOccurred = 1;
            }
          }
        else 
          {
          win->WarningMessage(buffer);
          }
        delete [] rmessage;
        delete [] buffer;
        }
      }
    else
      {
      this->Errors.push_back(t);
      if ( this->CrashOnErrors )
        {
        vtkPVApplication::Abort();
        }
      }
  }
  
  vtkPVOutputWindow()
  {
    this->Application = 0;
    this->ErrorOccurred = 0;
    this->TestErrors = 1;
    this->CrashOnErrors = 0;
  }

  ~vtkPVOutputWindow()
    {
      if ( this->Errors.size() > 0 )
        {
        cerr << "Errors while exiting ParaView:" << endl;
        this->FlushErrors(cerr);
        }
    }
  
  void FlushErrors(ostream& os)
    {
    vtkstd::string::size_type cc;
    for ( cc = 0; cc < this->Errors.size(); cc ++ )
      {
      os << this->Errors[cc].c_str() << endl;
      }
    this->Errors.erase(this->Errors.begin(), this->Errors.end());
    }

  void SetApplication(vtkKWApplication *app)
    {
    vtkKWApplication* old_app = this->Application;
    this->Application = app;
    if (!old_app && this->Application && this->Errors.size() > 0)
      {
      ostrstream str;
      this->FlushErrors(str);
      str << ends;
      vtkKWWindowBase *win = this->Application->GetNthWindow(
        this->Application->GetNumberOfWindows() - 1);
      if (win)
        {
        win->ErrorMessage(str.str());
        }
      str.rdbuf()->freeze(0);
      }
    }

  int GetErrorOccurred()
    {
    return this->ErrorOccurred;
    }

  vtkSetClampMacro(CrashOnErrors, int, 0, 1);
  vtkBooleanMacro(CrashOnErrors, int);

  void EnableTestErrors() { this->TestErrors = 1; }
  void DisableTestErrors() { this->TestErrors = 0; }
protected:
  vtkKWApplication *Application;
  int ErrorOccurred;
  int TestErrors;
  int CrashOnErrors;
  vtkstd::vector<vtkstd::string> Errors;
private:
  vtkPVOutputWindow(const vtkPVOutputWindow&);
  void operator=(const vtkPVOutputWindow&);
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOutputWindow);

//----------------------------------------------------------------------------
Tcl_Interp *vtkPVApplication::InitializeTcl(int argc, 
                                            char *argv[], 
                                            ostream *err)
{

  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc, argv, err);
  if (!interp)
    {
    return interp;
    }

#ifdef PARAVIEW_BUILD_DEVELOPMENT
  Vtkpvdevelopmenttcl_Init(interp);
#endif

  Vtkkwparaview_Init(interp);
  Vtkpvservermanagertcl_Init(interp); 
  Vtkpvservercommontcl_Init(interp); 

  char* script = 
    vtksys::SystemTools::DuplicateString(vtkPVApplication::ExitProc);  
  if (Tcl_GlobalEval(interp, script) != TCL_OK)
    {
    if (err)
      {
      *err << Tcl_GetStringResult(interp) << endl;
      }
    // ????
    }  
  delete [] script;
  
  return interp;
}

//----------------------------------------------------------------------------
vtkPVApplication::vtkPVApplication()
{
  this->Options = 0;
  this->ApplicationInitialized = 0;
  this->Observer = vtkPVApplicationObserver::New();
  this->Observer->SetApplication(this);
  vtkPVApplication::MainApplication = this;
  vtkPVOutputWindow *window = vtkPVOutputWindow::New();
  this->OutputWindow = window;
  vtkOutputWindow::SetInstance(this->OutputWindow);
  this->MajorVersion = PARAVIEW_VERSION_MAJOR;
  this->MinorVersion = PARAVIEW_VERSION_MINOR;
  this->SetName("ParaView");
  char name[128];
  sprintf(name, "ParaView%d.%d", this->MajorVersion, this->MinorVersion);
  this->SetVersionName(name);
  char patch[128];
  sprintf(patch, "%d", PARAVIEW_VERSION_PATCH);
  this->SetReleaseName(patch);

  this->Display3DWidgets = 0;

  this->ProcessModule = NULL;

  this->NumberOfPipes = 1;

  // GUI style & consistency

  vtkKWFrameLabeled::AllowShowHideOn();
  vtkKWFrameLabeled::BoldLabelOn();
  
  // The following is necessary to make sure that the tcl object
  // created has the right command function. Without this,
  // the tcl object has the vtkKWApplication's command function
  // since it is first created in vtkKWApplication's constructor
  // (in vtkKWApplication's constructor GetClassName() returns
  // the wrong value because the virtual table is not setup yet)
  char* tclname = vtksys::SystemTools::DuplicateString(this->GetTclName());
  vtkTclUpdateCommand(this->MainInterp, tclname, this);
  delete[] tclname;

  this->SupportSplashScreen = 1;

  this->TraceFileName = 0;
  this->TraceFile = NULL;
  this->Argv0 = 0;

  this->StartGUI = 1;

  this->SourcesBrowserAlwaysShowName = 0;
  this->ShowSourcesLongHelp = 1;


  this->SMApplication = vtkSMApplication::New();

  this->SaveRuntimeInfoButton = NULL;
  this->RenderModuleProxy = 0;
  this->RenderModuleProxyName = 0;
}

//----------------------------------------------------------------------------
vtkPVApplication::~vtkPVApplication()
{
  this->SetRenderModuleProxy(0);

  // Remove the ParaView output window so errors during exit will
  // still be displayed.
  vtkOutputWindow::SetInstance(0);

  if (this->TraceFile)
    {
    this->TraceFile->close();
    delete this->TraceFile;
    this->TraceFile = NULL;
    }
  this->SetTraceFileName(0);
  this->SetArgv0(0);
  vtkOutputWindow::SetInstance(0);
  this->OutputWindow->Delete();
  this->Observer->Delete();
  this->Observer = 0;

  this->SetProcessModule(NULL);
  if (this->SMApplication)
    {
    this->SMApplication->Finalize();
    this->SMApplication->Delete();
    this->SMApplication = NULL;
    }
  if (this->SaveRuntimeInfoButton)
    {
    this->SaveRuntimeInfoButton->Delete();
    this->SaveRuntimeInfoButton = 0;
    }
  

}

//----------------------------------------------------------------------------
void vtkPVApplication::SetProcessModule(vtkPVProcessModule *pm)
{
  this->ProcessModule = pm;
  vtkPVProcessModule::SetProcessModule(pm);
  if(pm)
    {  
    // copy all the command line settings from the application
    // to the process module
    pm->SetOptions(this->Options);

    // Juggle the compositing flag to let server in on the decision
    // whether to allow compositing / rendering on the server.
    // Put the flag on the process module.
    if (this->Options->GetDisableComposite())
      {
      pm->GetServerInformation()->SetRemoteRendering(0);
      }
    pm->GetProgressHandler()->SetClientMode(this->Options->GetClientMode());
    pm->GetProgressHandler()->SetServerMode(this->Options->GetServerMode());
    }
}
//----------------------------------------------------------------------------
int vtkPVApplication::SetupRenderModule()
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkPVProcessModule *pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  pm->SynchronizeServerClientOptions();

  const char* renderModuleName = 0;

  renderModuleName = pm->GetOptions()->GetRenderModuleName();
  if (!renderModuleName)
    {
    renderModuleName = pm->GetServerInformation()->GetRenderModuleName();
    }
  
  if (!renderModuleName)
    {
    // User didn't specify the render module to use.
    if (pm->GetOptions()->GetTileDimensions()[0])
      {
      // Server/client says we are rendering for a Tile Display.
      // Now decide if we must use IceT or not.
      if (pm->GetServerInformation()->GetUseIceT())
        {
        renderModuleName = "IceTRenderModule";
        }
      else
        {
        renderModuleName = "MultiDisplayRenderModule";
        }
      }
    else if (pm->GetOptions()->GetClientMode())
      {
      // Client server configuration without Tiles.
      if (pm->GetServerInformation()->GetUseIceT())
        {
        renderModuleName = "IceTDesktopRenderModule";
        }
      else
        {
        renderModuleName = "MPIRenderModule"; 
        // TODO: if I separated the MPI and ClientServer
        // render modules, this is where I will use
        // the ClientServerRenderModule.
        }
      }
    else
      {
      // Not running in Client Server Mode.
      // Use local information to choose render module.
#ifdef VTK_USE_MPI
      renderModuleName = "MPIRenderModule";
#else
      renderModuleName = "LODRenderModule";
#endif
      }
    }
  
  vtkSMProxy *p = pxm->NewProxy("rendermodules", renderModuleName);
  
  if (!p)
    {
    return 0;
    }
  
  vtkSMRenderModuleProxy* rm = vtkSMRenderModuleProxy::SafeDownCast(p);
  if (!rm)
    {
    vtkErrorMacro("Render Module must be a subclass of vtkSMRenderModuleProxy.");
    p->Delete();
    return 0;
    }
  rm->SetName("Ren1");
  rm->UpdateVTKObjects();
  this->SetRenderModuleProxy(rm);
  pm->GetOptions()->SetRenderModuleName(renderModuleName);
  rm->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVApplication::SetRenderModuleProxy(vtkSMRenderModuleProxy* rm)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  if (this->RenderModuleProxy)
    {
    if (this->RenderModuleProxyName && pxm)
      {
      pxm->UnRegisterProxy("rendermodules",this->RenderModuleProxyName);
      }
    this->SetRenderModuleProxyName(0);
    this->RenderModuleProxy->UnRegister(this);
    }
  this->RenderModuleProxy = rm;
  if (this->RenderModuleProxy)
    {
    this->RenderModuleProxy->Register(this);
    if (pxm)
      {
      static int numObjects = 0;
      ostrstream str;
      str << "RenderModule" << numObjects << ends;
      pxm->RegisterProxy("rendermodules",str.str(), this->RenderModuleProxy);
      this->SetRenderModuleProxyName(str.str());
      str.rdbuf()->freeze(0);
      }
    }
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVApplication::GetController()
{
  return this->ProcessModule->GetController();
}

//----------------------------------------------------------------------------
vtkSocketController* vtkPVApplication::GetSocketController()
{
  vtkPVClientServerModule *csm;
  csm = vtkPVClientServerModule::SafeDownCast(this->ProcessModule);
  if (csm)
    {
    return csm->GetSocketController();
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkPVWindow *vtkPVApplication::GetMainWindow()
{
  return vtkPVWindow::SafeDownCast(this->GetNthWindow(0));
}

//----------------------------------------------------------------------------
vtkPVRenderView *vtkPVApplication::GetMainView()
{
  return this->GetMainWindow()->GetMainView();
}

//----------------------------------------------------------------------------
int vtkPVApplication::AcceptLicense()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::AcceptEvaluation()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::PromptRegistration(char* vtkNotUsed(name), 
                                         char* vtkNotUsed(IDS))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::CheckRegistration()
{
  return 1;
}

const char vtkPVApplication::ArgumentList[vtkPVApplication::NUM_ARGS][128] = 
{ "--client" , "-c", 
  "", 
  "--client-render-server" , "-crs", 
  "", 
  "--connect-data-to-render" , "-d2r", 
  "", 
  "--connect-render-to-data" , "-r2d", 
  "", 
  "--machines" , "-m", 
  "",
  "--cave-configuration" , "-cc", 
  "",
  "--server" , "-v", 
  "",
  "--render-server" , "-rs", 
  "",
  "--render-server-host", "-rsh",
  "", 
  "--host", "-h",
  "", 
  "--user", "",
  "",
  "--always-ssh", "",
  "",
  "--render-port", "",
  "", 
  "--render-node-port", "",
  "", 
  "--port", "",
  "", 
  "--reverse-connection", "-rc",
  "", 
  "--stereo", "",
  "",
  "--render-module", "",
  "",
  "--start-empty" , "-e", 
  "", 
  "--disable-registry", "-dr", 
  "", 
  "--batch", "-b", 
  "", 
#ifdef VTK_USE_MPI
/* Temporarily disabling - for release
  "--use-rendering-group", "-p",
  "",
  "--group-file", "-gf",
  "",
*/
#endif
  "--use-tiled-display", "-td",
  "",
  "--tile-dimensions-x", "-tdx",
  "",
  "--tile-dimensions-y", "-tdy",
  "",
  "--crash-on-errors", "",
  "",
#ifdef VTK_USE_MANGLED_MESA
  "--use-software-rendering", "-r", 
  "", 
  "--use-satellite-software", "-s", 
  "", 
#endif
  "--use-offscreen-rendering", "-os", 
  "", 
  "--play-demo", "-pd",
  "",
  "--disable-composite", "-dc",
  "",
  "--connect-id", "",
  "",
  "--help", "",
  "",
  "" 
};

//----------------------------------------------------------------------------
char* vtkPVApplication::CreateHelpString()
{
  ostrstream error;
  error << "Valid arguments are: " << endl;

  int j=0;
  const char* argument1 = vtkPVApplication::ArgumentList[j];
  const char* argument2 = vtkPVApplication::ArgumentList[j+1];
  const char* help = vtkPVApplication::ArgumentList[j+2];
  while (argument1 && argument1[0])
    {
    if ( strlen(help) > 0 )
      {
      error << argument1;
      if (argument2[0])
        {
        error << ", " << argument2;
        }
      error << " : " << help << endl;
      }
    j += 3;
    argument1 = vtkPVApplication::ArgumentList[j];
    if (argument1 && argument1[0]) 
      {
      argument2 = vtkPVApplication::ArgumentList[j+1];
      help = vtkPVApplication::ArgumentList[j+2];
      }
    }
  error << this->Options->GetHelp();
  error << ends;
  return error.str();
  
}

//----------------------------------------------------------------------------
int vtkPVApplication::CheckForExtension(const char* arg,
                                        const char* ext)
{
  if (!ext)
    {
    return 0;
    }
  int extLen = static_cast<int>(strlen(ext));
  if (extLen <= 0)
    {
    return 0;
    }
  if (!arg || static_cast<int>(strlen(arg)) < extLen)
    {
    return 0;
    }
  if (strcmp(arg + strlen(arg) - extLen, ext) == 0)
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVApplication::DeleteTraceFiles(char* name, int all)
{
  if ( !all )
    {
    unlink(name);
    return;
    }
  char buf[256];
  if ( vtkDirectory::GetCurrentWorkingDirectory(buf, 256) )
    {
    vtkDirectory* dir = vtkDirectory::New();
    if ( !dir->Open(buf) )
      {
      dir->Delete();
      return;
      }
    int numFiles = dir->GetNumberOfFiles();
    int len = (int)(strlen("ParaViewTrace"));
    for(int i=0; i<numFiles; i++)
      {
      const char* fname = dir->GetFile(i);
      if ( strncmp(fname, "ParaViewTrace", len) == 0 )
        {
        unlink(fname);
        }
      }
    dir->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkPVApplication::CheckForTraceFile(char* name, unsigned int maxlen)
{
  if ( this->GetRegistryValue(2,"RunTime", VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY, 0) &&
    !this->GetIntRegistryValue(2,"RunTime", VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY) )
    {
    return 0;
    }
  char buf[256];
  if ( vtkDirectory::GetCurrentWorkingDirectory(buf, 256) )
    {
    vtkDirectory* dir = vtkDirectory::New();
    if ( !dir->Open(buf) )
      {
      dir->Delete();
      return 0;
      }
    int retVal = 0;
    int numFiles = dir->GetNumberOfFiles();
    int len = (int)(strlen("ParaViewTrace"));
    for(int i=0; i<numFiles; i++)
      {
      const char* fname = dir->GetFile(i);
      if ( strncmp(fname, "ParaViewTrace", len) == 0 )
        {
        if ( retVal == 0 )
          {
          strncpy(name, fname, maxlen);
          }
        retVal++;
        }
      }
    dir->Delete();
    return retVal;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::SaveTraceFile(const char* fname)
{
  vtkKWLoadSaveDialog* exportDialog = vtkKWLoadSaveDialog::New();
  this->GetApplication()->RetrieveDialogLastPathRegistryValue(exportDialog, "SaveTracePath");
  exportDialog->SetParent(this->GetMainWindow());
  exportDialog->Create(this);
  exportDialog->SaveDialogOn();
  exportDialog->SetTitle("Save ParaView Trace");
  exportDialog->SetDefaultExtension(".pvs");
  exportDialog->SetFileTypes("{{ParaView Scripts} {.pvs}} {{All Files} {*}}");
  if ( exportDialog->Invoke() && 
       exportDialog->GetFileName() && 
       strlen(exportDialog->GetFileName()) > 0 )
    {
    if (rename(fname, exportDialog->GetFileName()) != 0)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetApplication(), this->GetMainWindow(),
        "Error Saving", "Could not save trace file.",
        vtkKWMessageDialog::ErrorIcon);
      }
    else
      {
      this->SaveDialogLastPathRegistryValue(exportDialog, "SaveTracePath");
      }
    }
  exportDialog->Delete();
}

//----------------------------------------------------------------------------
int vtkPVApplication::ParseCommandLineArguments(int vtkNotUsed(argc), char*argv[])
{
  // This should really be part of Parsing !!!!!!
  if (argv)
    {
    this->SetArgv0(argv[0]);
    }

  if ( this->Options->GetDisableRegistry() )
    {
    this->RegistryLevel = 0;
    }

  // Set the tiled display flag if any tiled display option is used.
  if ( this->Options->GetTileDimensions()[0] )
    {
    if (!this->Options->GetClientMode())
      {
      vtkErrorMacro("Tiled display is supported only in client/server mode. Please re-run with --client option. Disabling tiled display");
      }
    }

  if ( this->Options->GetCrashOnErrors() )
    {
    this->OutputWindow->CrashOnErrorsOn();
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVApplication::Initialize()
{
  if ( this->ApplicationInitialized )
    {
    return;
    }

  if ( ! this->ProcessModule )
    {
    vtkErrorMacro("No process module");
    vtkPVApplication::Abort();
    }

  // Find the installation directory (now that we have the app name)
  this->FindInstallationDirectory();

#ifdef VTK_USE_MANGLED_MESA
  if (this->Options->GetUseSoftwareRendering())
    {
    vtkClientServerStream stream;
    vtkPVProcessModule* pm = this->GetProcessModule();
    vtkClientServerID gf = pm->NewStreamObject("vtkGraphicsFactory", stream);
    stream << vtkClientServerStream::Invoke
           << gf << "SetUseMesaClasses" << 1
           << vtkClientServerStream::End;
    pm->DeleteStreamObject(gf, stream);
    vtkClientServerID imf = pm->NewStreamObject("vtkImagingFactory", stream);
    stream << vtkClientServerStream::Invoke
           << imf << "SetUseMesaClasses" << 1
           << vtkClientServerStream::End;
    pm->DeleteStreamObject(imf, stream);
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);

    if (this->Options->GetUseSatelliteSoftwareRendering())
      {
      vtkGraphicsFactory::SetUseMesaClasses(0);
      vtkImagingFactory::SetUseMesaClasses(0);
      }
    }
#endif

  this->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);

  vtkSMProxyManager* proxm = vtkSMObject::GetProxyManager();
  proxm->InstantiateGroupPrototypes("filters");

  this->ApplicationInitialized = 1;
}

//----------------------------------------------------------------------------
void vtkPVApplication::Start(int argc, char*argv[])
{
  this->Initialize();
  if (!this->SetupRenderModule())
    {
    vtkErrorMacro("Failed to setup Render Module. Aborting.");
    abort();
    }
  // set the font size to be small
#ifdef _WIN32
  this->Script("option add *font {{Tahoma} 8}");
#else
  // Specify a font only if there isn't one in the database
  this->Script(
    "toplevel .tmppvwindow -class ParaView;"
    "if {[option get .tmppvwindow font ParaView] == \"\"} {"
    "option add *font "
    "-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1};"
    "destroy .tmppvwindow");
  this->Script("option add *highlightThickness 0");
  this->Script("option add *highlightBackground #ccc");
  this->Script("option add *activeBackground #eee");
  this->Script("option add *activeForeground #000");
  this->Script("option add *background #ccc");
  this->Script("option add *foreground #000");
  this->Script("option add *Entry.background #ffffff");
  this->Script("option add *Text.background #ffffff");
  this->Script("option add *Button.padX 6");
  this->Script("option add *Button.padY 3");
  this->Script("option add *selectColor #666");
#endif


// Temporarily removing this (for the release - it has bugs)
//    // Handle setting up the SGI pipes.
//    if (this->Options->GetUseRenderingGroup())
//      {
//      int numProcs = this->ProcessModule->GetNumberOfPartitions();
//      int numPipes = 1;
//      int id;
//      int fileFound=0;
//      // Until I add a user interface to set the number of pipes,
//      // just read it from a file.
//      if (this->GroupFileName)
//        {
//        ifstream ifs;
//        ifs.open(this->GroupFileName,ios::in);
//        if (ifs.fail())
//          {
//          vtkErrorMacro("Could not find the file " << this->GroupFileName);
//          }
//        else
//          {
//          char bfr[1024];
//          ifs.getline(bfr, 1024);
//          numPipes = atoi(bfr);
//          if (numPipes > numProcs) { numPipes = numProcs; }
//          if (numPipes < 1) { numPipes = 1; }
//          this->BroadcastScript("$Application SetNumberOfPipes %d", numPipes);   
        
//          for (id = 0; id < numPipes; ++id)
//            {
//            ifs.getline(bfr, 1024);
//            this->RemoteScript(
//              id, "$Application SetEnvironmentVariable {DISPLAY=%s}", 
//              bfr);
//            }
//          fileFound = 1;
//          }
//        }

    
//      if (!fileFound)
//        {
//        numPipes = numProcs;
//        vtkPVRenderGroupDialog *rgDialog = vtkPVRenderGroupDialog::New();
//        const char *displayString;
//        displayString = getenv("DISPLAY");
//        if (displayString)
//          {
//          rgDialog->SetDisplayString(0, displayString);
//          }
//        rgDialog->SetNumberOfProcessesInGroup(numPipes);
//        rgDialog->Create(this);
      
//        rgDialog->Invoke();
//        numPipes = rgDialog->GetNumberOfProcessesInGroup();
      
//        this->BroadcastScript("$Application SetNumberOfPipes %d", numPipes);    
      
//        if (displayString)
//          {    
//          for (id = 1; id < numPipes; ++id)
//            {
//            // Format a new display string based on process.
//            displayString = rgDialog->GetDisplayString(id);
//            this->RemoteScript(
//              id, "$Application SetEnvironmentVariable {DISPLAY=%s}", 
//              displayString);
//            }
//          }
//        rgDialog->Delete();
//        }
//      }

  // Splash screen ?

  if (this->SupportSplashScreen && this->ShowSplashScreen)
    {
    this->CreateSplashScreen();
    this->GetSplashScreen()->SetProgressMessage("Initializing application...");
    }

  // Application Icon 
#ifdef _WIN32
  this->Script("vtkKWSetApplicationIcon {} %d big",
               IDI_PARAVIEWICO);
  // No, we can't set the same icon, even if it has both 32x32 and 16x16
  this->Script("vtkKWSetApplicationIcon {} %d small",
               IDI_PARAVIEWICOSMALL);
#endif

  vtkPVWindow *ui = vtkPVWindow::New();
  this->AddWindow(ui);

  vtkCallbackCommand *ccm = vtkCallbackCommand::New();
  ccm->SetClientData(this);
  ccm->SetCallback(::vtkPVAppProcessMessage);  
  ui->AddObserver(vtkKWEvent::WarningMessageEvent, ccm);
  ui->AddObserver(vtkKWEvent::ErrorMessageEvent, ccm);
  ccm->Delete();

  if (this->SupportSplashScreen && this->ShowSplashScreen)
    {
    this->GetSplashScreen()->SetProgressMessage("Creating icons...");
    }

  this->CreateButtonPhotos();

  if (this->Options->GetStartEmpty())
    {
    ui->InitializeDefaultInterfacesOff();
    }

  if (this->SupportSplashScreen && this->ShowSplashScreen)
    {
    this->GetSplashScreen()->SetProgressMessage("Creating UI...");
    }

  ui->Create(this);

  // ui has ref. count of at least 1 because of AddItem() above
  ui->Delete();

  this->Script(
    "proc bgerror { m } "
    "{ global Application errorInfo; "
    "$Application DisplayTCLError \"$m $errorInfo\"}");

  this->OutputWindow->SetApplication(this);

  this->Script(
    "proc smGet {group source property {index 0} {form s}} {\n"
    "    vtkSMObject smGetTemp\n"
    "    set proxyManager [smGetTemp GetProxyManager]\n"
    "    smGetTemp Delete\n"
    "    set proxyName \"$group.$source\"\n"
    "    set proxy [$proxyManager GetProxy animateable $proxyName]\n"
    "    if {$proxy == \"\"} {\n"
//    "        return \"?no proxy: $group.$source?\"\n"
    "        return \"unknown\"\n"
    "    }\n"
    "    set prop [$proxy GetProperty $property]\n"
    "    if {$prop == \"\"} {\n"
    "        return \"unknown\"\n"
    "    }\n"
    "    vtkSMPropertyAdaptor smGetAdaptor\n"
    "    smGetAdaptor SetProperty $prop\n"
    "    if {$index >= [smGetAdaptor GetNumberOfRangeElements]} {\n"
    "        smGetAdaptor Delete\n"
//    "        return \"?no element: $index?\"\n"
    "        return \"unknown\"\n"
    "    }\n"
    "    set retVal [smGetAdaptor GetRangeValue $index]\n"
    "    smGetAdaptor Delete\n"
    "    return [format %%$form $retVal]\n"
    "}\n");

  // Check if there is an existing ParaViewTrace file.

  if (this->SupportSplashScreen && this->ShowSplashScreen)
    {
    this->GetSplashScreen()->SetProgressMessage("Looking for old trace files...");
    }
  char traceName[128];
  int foundTrace = this->CheckForTraceFile(traceName, 128);

  if (this->SupportSplashScreen && this->ShowSplashScreen)
    {
    this->GetSplashScreen()->Withdraw();
    }

  const char* loadedTraceName = 0;

  // If there is already an existing trace file, ask the
  // user what she wants to do with it.
  if (foundTrace && ! this->Options->GetParaViewScriptName()) 
    {
    vtkPVTraceFileDialog *dlg2 = vtkPVTraceFileDialog::New();
    dlg2->SetMasterWindow(ui);
    dlg2->Create(this);
    ostrstream str;
    str << "Do you want to save the existing tracefile?\n\n"
        << "A tracefile called " << traceName << " was found in "
        << "the current directory. This might mean that there is "
        << "another instance of ParaView running or ParaView crashed "
        << "previously and failed to delete it.\n"
        << "If ParaView crashed previously, this tracefile can "
        << "be useful in tracing the problem and you should consider "
        << "saving it.\n"
        << "If there is another instance of ParaView running, select "
        << "\"Do Nothing\" to avoid potential problems."
        << ends;
    dlg2->SetText(str.str());
    str.rdbuf()->freeze(0);
    dlg2->SetTitle("Tracefile found");
    dlg2->SetIcon();
    int shouldSave = dlg2->Invoke();
    dlg2->Delete();
    if ( shouldSave == 3 )
      {
      loadedTraceName = traceName;
      }
    else if (shouldSave == 2)
      {
      this->SaveTraceFile(traceName);
      }
    else if (shouldSave == 1)
      {
      if ( foundTrace > 1 )
        {
        char buffer[1024];
        sprintf(buffer, "Do you want to delete all the tracefiles? Answering No will only delete \"%s\".",
          traceName);
        int all = vtkKWMessageDialog::PopupYesNo(
          this, ui, "Delete Trace Files",
          buffer,
          vtkKWMessageDialog::QuestionIcon);
        this->DeleteTraceFiles(traceName, all);
        }
      else
        {
        this->DeleteTraceFiles(traceName, 0);
        }
      }
    // Having trouble with tiled display.
    // Not all tiles rendered properly after this dialoge at startup.
    // It must have been a race condition.
    this->GetMainView()->EventuallyRender();
    }

  // Open the trace file.
  int count = 0;
  struct stat fs;
  while(1)
    {
    ostrstream str;
    str << "ParaViewTrace" << count++ << ".pvs" << ends;
    if ( stat(str.str(), &fs) != 0 || count >= 10 )
      {
      this->SetTraceFileName(str.str());
      str.rdbuf()->freeze(0);
      break;
      }
    str.rdbuf()->freeze(0);
    }

  this->TraceFile = new ofstream(this->TraceFileName, ios::out);
    
  // Initialize a couple of variables in the trace file.
  vtkPVTraceHelper::OutputEntry(
    this->GetTraceFile(), 
    "set kw(%s) [$Application GetMainWindow]", ui->GetTclName());
  ui->GetTraceHelper()->SetInitialized(1);
  // We have to set this variable after the window variable is set,
  // so it has to be done here.
  vtkPVTraceHelper::OutputEntry(
    this->GetTraceFile(), 
    "set kw(%s) [$kw(%s) GetMainView]",
    ui->GetMainView()->GetTclName(), ui->GetTclName());
  ui->GetMainView()->GetTraceHelper()->SetInitialized(1);

  // Some scripts were hanging due to event loop issues.
  // This update prevents such problems.
  this->Script("update");
  if (loadedTraceName)
    {
    this->LoadScript(loadedTraceName);
    }
  else if (this->Options->GetParaViewScriptName())
    {
    this->LoadScript(this->Options->GetParaViewScriptName());
    }
  if ( this->Options->GetParaViewDataName() )
    {
    ui->Open(this->Options->GetParaViewDataName());
    }
  
  if (this->Options->GetPlayDemoFlag())
    {
    this->Script("set pvDemoCommandLine 1");
    this->PlayDemo(0);
    }
  else
    {
    this->vtkKWApplication::Start(argc,argv);
    }
  this->OutputWindow->SetApplication(NULL);

  // Break a circular reference.
  this->SetRenderModuleProxy(0);
}

//----------------------------------------------------------------------------
void vtkPVApplication::RestoreApplicationSettingsFromRegistry()
{ 
  this->Superclass::RestoreApplicationSettingsFromRegistry();

  // Show sources description ?

  if (this->HasRegistryValue(
    2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY))
    {
    this->ShowSourcesLongHelp = this->GetIntRegistryValue(
      2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY);
    }

  // Show name in sources description browser

  if (this->HasRegistryValue(
    2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY))
    {
    this->SourcesBrowserAlwaysShowName = this->GetIntRegistryValue(
      2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY);
    }
}

//-----------------------------------------------------------------------------
void vtkPVApplication::SetShowSourcesLongHelp(int v)
{
  if (this->ShowSourcesLongHelp == v)
    {
    return;
    }

  this->ShowSourcesLongHelp = v;
  this->Modified();

  // Update the properties of all the sources previously created
  // so that the Description label can be removed/brought back.
  
  if (this->GetMainWindow())
    {
    vtkPVSource *pvs;
    vtkPVSourceCollection *col = 
      this->GetMainWindow()->GetSourceList("Sources");
    vtkCollectionIterator *cit = col->NewIterator();
    cit->InitTraversal();
    while (!cit->IsDoneWithTraversal())
      {
      pvs = static_cast<vtkPVSource*>(cit->GetCurrentObject()); 
      pvs->UpdateProperties();
      cit->GoToNextItem();
      }
    cit->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVApplication::SetSourcesBrowserAlwaysShowName(int v)
{
  if (this->SourcesBrowserAlwaysShowName == v)
    {
    return;
    }

  this->SourcesBrowserAlwaysShowName = v;
  this->Modified();

  if (this->GetMainWindow())
    {
    this->GetMainWindow()->UpdateSelectMenu();
    }

  if (this->GetMainView())
    {
    this->GetMainView()->SetSourcesBrowserAlwaysShowName(v);
    }
}

//----------------------------------------------------------------------------
int vtkPVApplication::RemoveWindow(vtkKWWindowBase *win)
{
  if (this->GetNumberOfWindows() == 1)
    {
    // Try to get the render window to destruct before the render widget.
    this->SetRenderModuleProxy(0);
    }

  return this->Superclass::RemoveWindow(win);
}

//----------------------------------------------------------------------------
void vtkPVApplication::DestroyGUI()
{  
  this->Superclass::Exit();
  this->InExit = 0;
}

//----------------------------------------------------------------------------
int vtkPVApplication::Exit()
{  
  // If errors were reported to the output window, return a bad
  // status.

  if (vtkPVOutputWindow* w =
      vtkPVOutputWindow::SafeDownCast(vtkOutputWindow::GetInstance()))
    {
    if (w->GetErrorOccurred())
      {
      this->SetExitStatus(1);
      }
    }

  if (!this->Superclass::Exit())
    {
    return 0;
    }

  if (this->ProcessModule)
    {
    this->ProcessModule->Exit();
    }

  // This is a normal exit.  Close trace file here and delete it.

  if (this->TraceFile)
    {
    this->TraceFile->close();
    delete this->TraceFile;
    this->TraceFile = NULL;
    }

  if (this->TraceFileName)
    {
    unlink(this->TraceFileName);
    }
  if (this->SaveRuntimeInfoButton)
    {
    this->SaveRuntimeInfoButton->Delete();
    this->SaveRuntimeInfoButton = 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVApplication::StartRecordingScript(char *filename)
{
  if (this->TraceFile)
    {
    *this->TraceFile << "$Application StartRecordingScript " << filename << endl;
    this->StopRecordingScript();
    }

  this->TraceFile = new ofstream(filename, ios::out);
  if (this->TraceFile && this->TraceFile->fail())
    {
    vtkErrorMacro("Could not open trace file " << filename);
    delete this->TraceFile;
    this->TraceFile = NULL;
    return;
    }

  // Initialize a couple of variables in the trace file.
  vtkPVTraceHelper::OutputEntry(
    this->GetTraceFile(), 
    "set kw(%s) [$Application GetMainWindow]",
    this->GetMainWindow()->GetTclName());
  this->GetMainWindow()->GetTraceHelper()->SetInitialized(1);
  this->SetTraceFileName(filename);
}

//----------------------------------------------------------------------------
void vtkPVApplication::StopRecordingScript()
{
  if (this->TraceFile)
    {
    this->TraceFile->close();
    delete this->TraceFile;
    this->TraceFile = NULL;
    }
}


//============================================================================
// Make instances of sources.
//============================================================================

void vtkPVApplication::DisplayHelpDialog(vtkKWWindowBase* master)
{
  vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
  dlg->SetTitle("ParaView Help");
  dlg->SetMasterWindow(master);
  dlg->Create(this);
  dlg->SetText("\"The ParaView Guide\", covering both the use and development of ParaView, is available for purchase from Kitware's online store at http://store.yahoo.com/kitware/paraviewguide.html");
  dlg->Invoke();  
  dlg->Delete();
}


#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
//----------------------------------------------------------------------------
void vtkPVApplication::SetupTrapsForSignals(int nodeid)
{
#ifndef _WIN32
  signal(SIGHUP, vtkPVApplication::TrapsForSignals);
#endif
  signal(SIGINT, vtkPVApplication::TrapsForSignals);
  if ( nodeid == 0 )
    {
    signal(SIGILL,  vtkPVApplication::TrapsForSignals);
    signal(SIGABRT, vtkPVApplication::TrapsForSignals);
    signal(SIGSEGV, vtkPVApplication::TrapsForSignals);

#ifndef _WIN32
    signal(SIGQUIT, vtkPVApplication::TrapsForSignals);
    signal(SIGPIPE, vtkPVApplication::TrapsForSignals);
    signal(SIGBUS,  vtkPVApplication::TrapsForSignals);
#endif
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::TrapsForSignals(int signal)
{
  if ( !vtkPVApplication::MainApplication )
    {
    exit(1);
    }
  
  switch ( signal )
    {
#ifndef _WIN32
    case SIGHUP:
      return;
      break;
#endif
    case SIGINT:
#ifndef _WIN32
    case SIGQUIT: 
#endif
      break;
    case SIGILL:
    case SIGABRT: 
    case SIGSEGV:
#ifndef _WIN32
    case SIGPIPE:
    case SIGBUS:
#endif
      vtkPVApplication::ErrorExit(); 
      break;      
    }
  
  if ( vtkPVApplication::MainApplication->GetProcessId() )
    {
    return;
    }
  vtkPVWindow *win = vtkPVApplication::MainApplication->GetMainWindow();
  if ( !win )
    {
    vtkPVApplication::MainApplication->Exit();
    }
  win->Exit();
}

//----------------------------------------------------------------------------
void vtkPVApplication::ErrorExit()
{
  // This { is here because compiler is smart enough to know that exit
  // exits the code without calling destructors. By adding this,
  // destructors are called before the exit.
  {
  cerr << "There was a major error! Trying to exit..." << endl;
  char name[] = "ErrorApplication";
  char *n = name;
  char** args = &n;
  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(1, args);
  ostrstream str;
  char buffer[1024];
#ifdef _WIN32
  _getcwd(buffer, 1023);
#else
  getcwd(buffer, 1023);
#endif

  Tcl_GlobalEval(interp, "wm withdraw .");
#ifdef _WIN32
  str << "option add *font {{MS Sans Serif} 8}" << endl;
#else
  str << "option add *font "
    "-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1" << endl;
  str << "option add *highlightThickness 0" << endl;
  str << "option add *highlightBackground #ccc" << endl;
  str << "option add *activeBackground #eee" << endl;
  str << "option add *activeForeground #000" << endl;
  str << "option add *background #ccc" << endl;
  str << "option add *foreground #000" << endl;
  str << "option add *Entry.background #ffffff" << endl;
  str << "option add *Text.background #ffffff" << endl;
  str << "option add *Button.padX 6" << endl;
  str << "option add *Button.padY 3" << endl;
#endif
  str << "tk_messageBox -type ok -message {It looks like ParaView "
      << "or one of its libraries performed an illegal opeartion and "
      << "it will be terminated. Please report this error to "
      << "bug-report@kitware.com. You may want to include a small "
      << "description of what you did when this happened and your "
      << "ParaView trace file: " << buffer
      << "/ParaViewTrace.pvs} -icon error"
      << ends;
  Tcl_GlobalEval(interp, str.str());
  str.rdbuf()->freeze(0);
  }
  exit(1);
}
#endif // PV_HAVE_TRAPS_FOR_SIGNALS

//----------------------------------------------------------------------------
void vtkPVApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ProcessModule: " << this->ProcessModule << endl;;
  os << indent << "NumberOfPipes: " << this->NumberOfPipes << endl;
  os << indent << "StartGUI: " << this->StartGUI << endl;

  os << indent << "Display3DWidgets: " << (this->Display3DWidgets?"on":"off") 
     << endl;
  os << indent << "TraceFileName: " 
     << (this->TraceFileName ? this->TraceFileName : "(none)") << endl;
  os << indent << "Argv0: " 
     << (this->Argv0 ? this->Argv0 : "(none)") << endl;

  os << indent << "ShowSourcesLongHelp: " 
     << (this->ShowSourcesLongHelp?"on":"off") << endl;
  os << indent << "SourcesBrowserAlwaysShowName: " 
     << (this->SourcesBrowserAlwaysShowName?"on":"off") << endl;

  os << indent << "SMApplication: ";
  if (this->SMApplication)
    {
    this->SMApplication->PrintSelf(os << endl, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "Options: ";
  if (this->Options)
    {
    this->Options->PrintSelf(os << endl, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "RenderModuleProxy: " << this->RenderModuleProxy << endl;
}

//----------------------------------------------------------------------------
void vtkPVApplication::DisplayTCLError(const char* message)
{
  vtkErrorMacro("TclTk error: "<<message);
}

//----------------------------------------------------------------------------
const char* const vtkPVApplication::ExitProc =
"proc exit {} { global Application; $Application Exit }";

//----------------------------------------------------------------------------
void vtkPVApplication::EnableTestErrors()
{
  this->OutputWindow->EnableTestErrors();
}

//----------------------------------------------------------------------------
void vtkPVApplication::DisableTestErrors()
{
  this->OutputWindow->DisableTestErrors();
}

//----------------------------------------------------------------------------
void vtkPVApplication::Abort()
{
  vtkPVApplication::MainApplication->OutputWindow->FlushErrors(cerr);
  abort();
}

//----------------------------------------------------------------------------
int vtkPVApplication::GetNumberOfPartitions()
{
  return this->GetProcessModule()->GetNumberOfPartitions();
}

//----------------------------------------------------------------------------
void vtkPVApplication::ExecuteEvent(vtkObject *o, unsigned long event, void* calldata)
{
  (void)event;
  (void)calldata;
  (void)o;
  // Placeholder for future events
  /*
  switch ( event ) 
    {
  default:
    vtkPVApplication::Abort();
    break;
    }
    */
}

//----------------------------------------------------------------------------
int vtkPVApplication::SelfTest()
{
  int res = 0;
  this->Script("eval set _foo_ foo");
  res += (!this->EvaluateBooleanExpression("proc a {} { return 1; }; a"));
  res += this->EvaluateBooleanExpression("proc a {} { return 0; }; a");

  return (res == 0);
}

//-----------------------------------------------------------------------------
void vtkPVApplication::PlayDemo(int fromDashboard)
{
  vtkPVWindow* window = this->GetMainWindow();
  window->SetInDemo(1);
  const char* demoDataPath;
  const char* demoScriptPath;
  
  window->Script("catch {unset pvDemoFromDashboard}");
  if (fromDashboard)
    {
    window->Script("update");
    window->Script("set pvDemoFromDashboard 1");
    }

  // Server path
  vtkPVProcessModule* pm = this->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetDemoPath"
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  if(!pm->GetLastResult(
       vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &demoDataPath))
    {
    demoDataPath = 0;
    }
  // Client path
  demoScriptPath = pm->GetDemoPath();

  if (demoDataPath && demoScriptPath)
    {
    char temp1[1024];
    sprintf(temp1, "%s/Demo1.pvs", 
            demoScriptPath);

    window->Script("set DemoDir {%s}", demoDataPath);
    window->LoadScript(temp1);
    }
  else
    {
    if (window->GetUseMessageDialog())
      {
      vtkKWMessageDialog::PopupMessage(
        this, window,
        "Warning", 
        "Could not find Demo1.pvs in the installation or\n"
        "build directory. Please make sure that ParaView\n"
        "is installed properly.",
        vtkKWMessageDialog::WarningIcon);
      }
    else
      {
      vtkWarningMacro("Could not find Demo1.pvs in the installation or "
                      "build directory. Please make sure that ParaView "
                      "is installed properly.");
      }
    }
  if(!fromDashboard)
    {
    window->SetInDemo(0);
    window->UpdateEnableState();
    }
}


//-----------------------------------------------------------------------------
char* vtkPVApplication::GetTextRepresentation(vtkPVSource* comp)
{
  char *buffer;
  if (!comp->GetLabel())
    {
    buffer = new char [strlen(comp->GetName()) + 1];
    sprintf(buffer, "%s", comp->GetName());
    }
  else
    {
    if (this->GetSourcesBrowserAlwaysShowName() && 
        comp->GetName() && *comp->GetName())
      {
      buffer = new char [strlen(comp->GetLabel())
                        + 2
                        + strlen(comp->GetName()) 
                        + 1
                        + 1];
      sprintf(buffer, "%s (%s)", comp->GetLabel(), comp->GetName());
      }
    else
      {
      buffer = new char [strlen(comp->GetLabel()) + 1];
      sprintf(buffer, "%s", comp->GetLabel());
      }
    }
  return buffer;
}

vtkKWLoadSaveDialog* vtkPVApplication::NewLoadSaveDialog()
{
  if(!this->Options->GetClientMode())
    {
      return vtkKWLoadSaveDialog::New();
    }
  vtkPVServerFileDialog* dialog = vtkPVServerFileDialog::New();
  dialog->SetMasterWindow(this->GetMainWindow());
  return dialog;
  
}

void vtkPVApplication::FindInstallationDirectory()
{
  this->Superclass::FindInstallationDirectory();
  
  if (!this->InstallationDirectory)
    {
    return;
    }

  // Paraview is installed in the bin/ directory. Strip it off if found.
  // This will only happen if the binary has been installed. No change is
  // made if the path is retrieved from the registry, or if the binary
  // is a 'build'.

  int length = strlen(this->InstallationDirectory);
  if (length >= 4 &&
      !strcmp(this->InstallationDirectory + length - 4, "/bin"))
    {
    this->InstallationDirectory[length - 4] = '\0';
    }
}

//----------------------------------------------------------------------------
int vtkPVApplication::SendStringToClient(const char* str)
{
  vtkClientServerStream stream;
  if (!stream.StreamFromString(str))
    {
    return 0;
    }
  this->ProcessModule->SendStream(vtkProcessModule::CLIENT, stream);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::SendStringToClientAndServer(const char* str)
{
  vtkClientServerStream stream;
  if (!stream.StreamFromString(str))
    {
    return 0;
    }
  this->ProcessModule->SendStream(vtkProcessModule::CLIENT |
                                  vtkProcessModule::DATA_SERVER,
                                  stream);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::SendStringToClientAndServerRoot(const char* str)
{
  vtkClientServerStream stream;
  if (!stream.StreamFromString(str))
    {
    return 0;
    }
  this->ProcessModule->SendStream(vtkProcessModule::CLIENT |
                                  vtkProcessModule::DATA_SERVER_ROOT,
                                  stream);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::SendStringToServer(const char* str)
{
  vtkClientServerStream stream;
  if (!stream.StreamFromString(str))
    {
    return 0;
    }
  this->ProcessModule->SendStream(vtkProcessModule::DATA_SERVER, stream);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::SendStringToServerRoot(const char* str)
{
  vtkClientServerStream stream;
  if (!stream.StreamFromString(str))
    {
    return 0;
    }
  this->ProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkPVApplication::GetStringFromServer()
{
  return this->ProcessModule->GetLastResult(
    vtkProcessModule::DATA_SERVER_ROOT).StreamToString();
}

//----------------------------------------------------------------------------
const char* vtkPVApplication::GetStringFromClient()
{
  return this->ProcessModule->GetLastResult(
    vtkProcessModule::CLIENT).StreamToString();
}

//----------------------------------------------------------------------------
vtkPVGUIClientOptions* vtkPVApplication::GetGUIClientOptions()
{
  return this->Options;
}

//----------------------------------------------------------------------------
vtkPVOptions* vtkPVApplication::GetOptions()
{
  return this->Options;
}

//----------------------------------------------------------------------------
void vtkPVApplication::SetOptions(vtkPVGUIClientOptions* op)
{
  this->Options = op;
}

//----------------------------------------------------------------------------
void vtkPVApplication::SetGarbageCollectionGlobalDebugFlag(int flag)
{
  vtkGarbageCollector::SetGlobalDebugFlag(flag);
}

//----------------------------------------------------------------------------
int vtkPVApplication::GetGarbageCollectionGlobalDebugFlag()
{
  return vtkGarbageCollector::GetGlobalDebugFlag();
}

//----------------------------------------------------------------------------
void vtkPVApplication::DeferredGarbageCollectionPush()
{
  vtkGarbageCollector::DeferredCollectionPush();
}

//----------------------------------------------------------------------------
void vtkPVApplication::DeferredGarbageCollectionPop()
{
  vtkGarbageCollector::DeferredCollectionPop();
}
