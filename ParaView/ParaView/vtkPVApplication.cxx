/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplication.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVApplication.h"

#include "vtkToolkits.h"
#include "vtkPVConfig.h"
#ifdef VTK_USE_MPI
# include <mpi.h>
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIGroup.h"
#endif

#include "vtkCallbackCommand.h"
#include "vtkCharArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkKWDialog.h"
#include "vtkKWEvent.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWindowCollection.h"
#include "vtkLongArray.h"
#include "vtkMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkProbeFilter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkShortArray.h"
#include "vtkString.h"
#include "vtkTclUtil.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkPolyData.h"
#include "vtkPVSourceInterfaceDirectories.h"

#include <sys/stat.h>
#include <stdarg.h>
#include <signal.h>

#ifdef _WIN32
#include "vtkKWRegisteryUtilities.h"

#include "htmlhelp.h"
#include "direct.h"
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVApplication);
vtkCxxRevisionMacro(vtkPVApplication, "1.141");

int vtkPVApplicationCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

//----------------------------------------------------------------------------
extern "C" int Vtktkrenderwidget_Init(Tcl_Interp *interp);
extern "C" int Vtkkwparaviewtcl_Init(Tcl_Interp *interp);

vtkPVApplication* vtkPVApplication::MainApplication = 0;

static void vtkPVAppProcessMessage(vtkObject* vtkNotUsed(object),
                                   unsigned long vtkNotUsed(event), 
                                   void *clientdata, void *calldata)
{
  vtkPVApplication *self = static_cast<vtkPVApplication*>( clientdata );
  const char* message = static_cast<char*>( calldata );
  cout << "# Error or warning: " << message << endl;
  self->AddTraceEntry("# Error or warning:");
  int cc;
  ostrstream str;
  for ( cc= 0; cc < vtkString::Length(message); cc ++ )
    {
    str << message[cc];
    if ( message[cc] == '\n' )
      {
      str << "# ";
      }
    }
  str << ends;
  self->AddTraceEntry("# %s\n#", str.str());
  //cout << "# " << str.str() << endl;
  str.rdbuf()->freeze(0);
}

// initialze the class variables
int vtkPVApplication::GlobalLODFlag = 0;

// Output window which prints out the process id
// with the error or warning messages
class VTK_EXPORT vtkPVOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkPVOutputWindow,vtkOutputWindow);
  
  static vtkPVOutputWindow* New();

  void DisplayText(const char* t)
  {
    if ( this->Windows && this->Windows->GetNumberOfItems() &&
         this->Windows->GetLastKWWindow() )
      {
      vtkKWWindow *win = this->Windows->GetLastKWWindow();
      char buffer[1024];      
      const char *message = strstr(t, "): ");
      char type[1024], file[1024];
      int line;
      sscanf(t, "%[^:]: In %[^,], line %d", type, file, &line);
      if ( message )
        {
        int error = 0;
        if ( !strncmp(t, "ERROR", 5) )
          {
          error = 1;
          }
        message += 3;
        char *rmessage = vtkString::Duplicate(message);
        int last = vtkString::Length(rmessage)-1;
        while ( last > 0 && 
                (rmessage[last] == ' ' || rmessage[last] == '\n' || 
                 rmessage[last] == '\r' || rmessage[last] == '\t') )
          {
          rmessage[last] = 0;
          last--;
          }
        sprintf(buffer, "There was a VTK %s in file: %s (%d)\n %s", 
                (error ? "Error" : "Warning"),
                file, line,
                rmessage);
        if ( error )
          {
          win->ErrorMessage(buffer);
          }
        else 
          {
          win->WarningMessage(buffer);
          }
        delete [] rmessage;
        }
      }
  }

  vtkPVOutputWindow()
  {
    this->Windows = 0;
  }
  
  void SetWindowCollection(vtkKWWindowCollection *windows)
  {
    this->Windows = windows;
  }

protected:
  vtkKWWindowCollection *Windows;

private:
  vtkPVOutputWindow(const vtkPVOutputWindow&);
  void operator=(const vtkPVOutputWindow&);
};

vtkStandardNewMacro(vtkPVOutputWindow);

Tcl_Interp *vtkPVApplication::InitializeTcl(int argc, char *argv[])
{

  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc,argv);
  
  //  if (Vtkparalleltcl_Init(interp) == TCL_ERROR) 
  //  {
   // cerr << "Init Parallel error\n";
   // }

  // Why is this here?  Doesn't the superclass initialize this?
  if (vtkKWApplication::GetWidgetVisibility())
    {
    Vtktkrenderwidget_Init(interp);
    }
   
  Vtkkwparaviewtcl_Init(interp);
  
  // Create the component loader procedure in Tcl.
  char* script = vtkString::Duplicate(vtkPVApplication::LoadComponentProc);  
  if (Tcl_GlobalEval(interp, script) != TCL_OK)
    {
    // ????
    }  
  delete [] script;
  
  return interp;
}

//----------------------------------------------------------------------------
vtkPVApplication::vtkPVApplication()
{
  this->Display3DWidgets = 0;
  this->ProcessId = 0;
  this->RunningParaViewScript = 0;

  char name[128];
  this->CommandFunction = vtkPVApplicationCommand;
  this->MajorVersion = 0;
  this->MinorVersion = 5;
  this->SetApplicationName("ParaView");
  sprintf(name, "ParaView%d.%d", this->MajorVersion, this->MinorVersion);
  this->SetApplicationVersionName(name);
  this->SetApplicationReleaseName("development");

  this->Controller = NULL;

  struct stat fs;

  if (stat("ParaViewTrace1.pvs", &fs) == 0) 
    {
    rename("ParaViewTrace1.pvs", "ParaViewTrace2.pvs");
    }

  if (stat("ParaViewTrace.pvs", &fs) == 0) 
    {
    rename("ParaViewTrace.pvs", "ParaViewTrace1.pvs");
    }

  this->TraceFile = new ofstream("ParaViewTrace.pvs", ios::out);
  if (this->TraceFile && this->TraceFile->fail())
    {
    delete this->TraceFile;
    this->TraceFile = NULL;
    }

  vtkKWLabeledFrame::AllowShowHideOn();
  
  // The following is necessary to make sure that the tcl object
  // created has the right command function. Without this,
  // the tcl object has the vtkKWApplication's command function
  // since it is first created in vtkKWApplication's constructor
  // (in vtkKWApplication's constructor GetClassName() returns
  // the wrong value because the virtual table is not setup yet)
  char* tclname = vtkString::Duplicate(this->GetTclName());
  vtkTclUpdateCommand(this->MainInterp, tclname, this);
  delete[] tclname;

  this->HasSplashScreen = 1;
  if (this->HasRegisteryValue(
    2, "RunTime", VTK_KW_SPLASH_SCREEN_REG_KEY))
    {
    this->ShowSplashScreen = this->GetIntRegisteryValue(
      2, "RunTime", VTK_KW_SPLASH_SCREEN_REG_KEY);
    }
  else
    {
    this->ShowSplashScreen = 1;
    }
}


//----------------------------------------------------------------------------
vtkPVWindow *vtkPVApplication::GetMainWindow()
{
  this->Windows->InitTraversal();
  return (vtkPVWindow*)(this->Windows->GetNextItemAsObject());
}


//----------------------------------------------------------------------------
void vtkPVApplication::SetController(vtkMultiProcessController *c)
{
  if (this->Controller == c)
    {
    return;
    }

  if (c)
    {
    c->Register(this);
    this->ProcessId = c->GetLocalProcessId();
    }
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    }
  

  this->Controller = c;

  this->NumberOfPipes = 1;
}

//----------------------------------------------------------------------------
vtkPVApplication::~vtkPVApplication()
{
  this->SetController(NULL);
  if ( this->TraceFile )
    {
    delete this->TraceFile;
    this->TraceFile = 0;
    }
}


//----------------------------------------------------------------------------
void vtkPVApplication::RemoteScript(int id, char *format, ...)
{
  char event[1600];
  char* buffer = event;
  
  va_list ap;
  va_start(ap, format);
  int length = this->EstimateFormatLength(format, ap);
  va_end(ap);
  
  if(length > 1599)
    {
    buffer = new char[length+1];
    }
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(buffer, format, var_args);
  va_end(var_args);  
  
  this->RemoteSimpleScript(id, buffer);
  
  if(buffer != event)
    {
    delete [] buffer;
    }
}
//----------------------------------------------------------------------------
void vtkPVApplication::RemoteSimpleScript(int remoteId, const char *str)
{
  int length;

  // send string to evaluate.
  length = vtkString::Length(str) + 1;
  if (length <= 1)
    {
    return;
    }

  if (this->Controller->GetLocalProcessId() == remoteId)
    {
    this->SimpleScript(str);
    return;
    }
  
  this->Controller->TriggerRMI(remoteId, const_cast<char*>(str), 
                               VTK_PV_SLAVE_SCRIPT_RMI_TAG);
}

//----------------------------------------------------------------------------
void vtkPVApplication::BroadcastScript(char *format, ...)
{
  char event[1600];
  char* buffer = event;
  
  va_list ap;
  va_start(ap, format);
  int length = this->EstimateFormatLength(format, ap);
  va_end(ap);
  
  if(length > 1599)
    {
    buffer = new char[length+1];
    }
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(buffer, format, var_args);
  va_end(var_args);
  
  this->BroadcastSimpleScript(buffer);
  
  if(buffer != event)
    {
    delete [] buffer;
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::BroadcastSimpleScript(const char *str)
{
  int id, num;
  
  num = this->Controller->GetNumberOfProcesses();

  int len = vtkString::Length(str);
  if (!str || (len < 1))
    {
    return;
    }

  for (id = 1; id < num; ++id)
    {
    this->RemoteSimpleScript(id, str);
    }
  
  // Do reverse order, because 0 will block.
  this->SimpleScript(str);
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

//----------------------------------------------------------------------------
int vtkPVApplication::CheckForArgument(int argc, char* argv[], 
                                       const char* arg, int& index)
{
  if (!arg)
    {
    return VTK_ERROR;
    }

  int i;
  for (i=0; i < argc; i++)
    {
    if (argv[i] && strcmp(arg, argv[i]) == 0)
      {
      index = i;
      return VTK_OK;
      }
    }
  return VTK_ERROR;
}

const char vtkPVApplication::ArgumentList[vtkPVApplication::NUM_ARGS][128] = 
{ "--start-empty" , "-e", 
  "Start ParaView without any default modules.", 
  "--disable-registry", "-g", 
  "Do not use registry when running ParaView (for testing).", 
#ifdef VTK_MANGLE_MESA
  "--use-software-rendering", "-r", 
  "Use software (Mesa) rendering (supports off-screen rendering).", 
  "--use-satellite-software", "-s", 
  "Use software (Mesa) rendering (supports off-screen rendering) only on satellite processes.", 
#endif
  "--help", "",
  "Displays available command line arguments.",
  "" 
};

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
    error << argument1;
    if (argument2[0])
      {
      error << ", " << argument2;
      }
    error << " : " << help << endl;
    j += 3;
    argument1 = vtkPVApplication::ArgumentList[j];
    if (argument1 && argument1[0]) 
      {
      argument2 = vtkPVApplication::ArgumentList[j+1];
      help = vtkPVApplication::ArgumentList[j+2];
      }
    }
  error << ends;
  return error.str();
  
}

int vtkPVApplication::IsParaViewScriptFile(const char* arg)
{
  if (!arg || strlen(arg) < 4)
    {
    return 0;
    }
  if (strcmp(arg + strlen(arg) - 4,".pvs") == 0)
    {
    return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVApplication::SetEnvironmentVariable(const char* str)
{
  char* envstr = vtkString::Duplicate(str);
  putenv(envstr);
}

//----------------------------------------------------------------------------
void vtkPVApplication::Start(int argc, char*argv[])
{
  // Splash screen ?

  if (this->ShowSplashScreen)
    {
    this->CreateSplashScreen();
    this->SplashScreen->SetProgressMessage("Initializing application...");
    }

  vtkOutputWindow::GetInstance()->PromptUserOn();

  // set the font size to be small
#ifdef _WIN32
  this->Script("option add *font {{MS Sans Serif} 8}");
#else
  this->Script("option add *font -adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1");
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


  int i;
  for (i=1; i < argc; i++)
    {
    if ( vtkPVApplication::IsParaViewScriptFile(argv[i]) )
      {
      this->RunningParaViewScript = 1;
      break;
      }
    }

  if (!this->RunningParaViewScript)
    {
    for (i=1; i < argc; i++)
      {
      int valid=0;
      if (argv[i])
        {
        int  j=0;
        const char* argument1 = vtkPVApplication::ArgumentList[j];
        const char* argument2 = vtkPVApplication::ArgumentList[j+1];
        while (argument1 && argument1[0])
          {
          if ( strcmp(argv[i], argument1) == 0 || 
               strcmp(argv[i], argument2) == 0)
            {
            valid = 1;
            }
          j += 3;
          argument1 = vtkPVApplication::ArgumentList[j];
          if (argument1 && argument1[0]) 
            {
            argument2 = vtkPVApplication::ArgumentList[j+1];
            }
          }
        }
      if (!valid)
        {
        char* error = this->CreateHelpString();
        vtkErrorMacro("Unrecognized argument " << argv[i] << "." << endl
                      << error);
        delete[] error;
        this->Exit();
        return;
        }
      }
    }

  int index=-1;

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--help",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-h",
                                          index) == VTK_OK )
    {
    char* error = this->CreateHelpString();
    vtkWarningMacro(<<error);
    delete[] error;
    this->Exit();
    return;
    }

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--disable-registry",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-d",
                                          index) == VTK_OK )
    {
    this->RegisteryLevel = 0;
    }

#ifdef VTK_MANGLE_MESA
  
  if ( vtkPVApplication::CheckForArgument(argc, argv, "--use-software-rendering",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-r",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "--use-satellite-software",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-s",
                                          index) == VTK_OK ||
       getenv("PV_SOFTWARE_RENDERING") )
    {
    this->BroadcastScript("vtkGraphicsFactory _graphics_fact\n"
                          "_graphics_fact SetUseMesaClasses 1\n"
                          "_graphics_fact Delete");
    this->BroadcastScript("vtkImagingFactory _imaging_fact\n"
                          "_imaging_fact SetUseMesaClasses 1\n"
                          "_imaging_fact Delete");
    if ( getenv("PV_SOFTWARE_RENDERING") ||
         vtkPVApplication::CheckForArgument(
           argc, argv, "--use-satellite-software", index) == VTK_OK ||
         vtkPVApplication::CheckForArgument(argc, argv, "-s",
                                            index) == VTK_OK)
      {
      this->Script("vtkGraphicsFactory _graphics_fact\n"
                   "_graphics_fact SetUseMesaClasses 0\n"
                   "_graphics_fact Delete");
      this->Script("vtkImagingFactory _imaging_fact\n"
                   "_imaging_fact SetUseMesaClasses 0\n"
                   "_imaging_fact Delete");
      }
    }
#endif


  // Handle setting up the SGI pipes.
#ifdef PV_USE_SGI_PIPES
  int numPipes = 1;
  int numProcs = this->Controller->GetNumberOfProcesses();
  int id;
  // Until I add a user interface to set the number of pipes,
  // just read it from a file.
  ifstream ifs("pipes.inp",ios::in);
  if (ifs.fail())
    {
    vtkErrorMacro("Could not find the file pipes.inp");
    numPipes = numProcs;
    }
  else
    {
    ifs >> numPipes;
    if (numPipes > numProcs) numPipes = numProcs;
    if (numPipes < 1) numPipes = 1;
    }
  this->BroadcastScript("$Application SetNumberOfPipes %d", numPipes);

  // assuming that the number of pipes is the same as the number of procs
  char *displayString;
  // Get display root
  char displayCommand[80];
  char displayStringRoot[80];
  displayString = getenv("DISPLAY");
  if (displayString)
    {
    // Extract the position of the display from the string.
    int len = -1;
    int j, i = 0;
    while (i < 80)
      {
      if (displayString[i] == ':')
        {
        j = i+1;
        while (j < 80)
          {
          if (displayString[j] == '.')
            {
            len = j+1;
            break;
            }
          j++;
          }
        break;
        }
      i++;
      }
    for (id = 0; id < numPipes; ++id)
      {
      // Format a new display string based on process.
      strncpy(displayStringRoot, displayString, len);
      displayStringRoot[len] = '\0';
      //    cerr << "display string root = " << displayStringRoot << endl;
      sprintf(displayCommand, "DISPLAY=%s%d", displayStringRoot, id);
      //    cerr << "display command = " << displayCommand << endl;
      this->RemoteScript(id, "$Application SetEnvironmentVariable {%s}", 
                         displayCommand);
      }
    }
#endif

  vtkPVWindow *ui = vtkPVWindow::New();
  this->Windows->AddItem(ui);

  vtkCallbackCommand *ccm = vtkCallbackCommand::New();
  ccm->SetClientData(this);
  ccm->SetCallback(::vtkPVAppProcessMessage);  
  ui->AddObserver(vtkKWEvent::WarningMessageEvent, ccm);
  ui->AddObserver(vtkKWEvent::ErrorMessageEvent, ccm);
  ccm->Delete();

  if (this->ShowSplashScreen)
    {
    this->SplashScreen->SetProgressMessage("Creating icons...");
    }

  this->CreateButtonPhotos();

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--start-empty", index) 
       == VTK_OK || 
       vtkPVApplication::CheckForArgument(argc, argv, "-e", index) 
       == VTK_OK)
    {
    ui->InitializeDefaultInterfacesOff();
    }

  if (this->ShowSplashScreen)
    {
    this->SplashScreen->SetProgressMessage("Creating UI...");
    }

  ui->Create(this,"");

  // ui has ref. count of at least 1 because of AddItem() above
  ui->Delete();

  this->Script("proc bgerror { m } { global Application; $Application DisplayTCLError $m }");
  vtkPVOutputWindow *window = vtkPVOutputWindow::New();
  window->SetWindowCollection( this->Windows );
  this->OutputWindow = window;
  vtkOutputWindow::SetInstance(this->OutputWindow);

  if (this->ShowSplashScreen)
    {
    this->SplashScreen->Hide();
    }

  // If any of the argumens has a .pvs extension, load it as a script.
  for (i=1; i < argc; i++)
    {
    if (vtkPVApplication::IsParaViewScriptFile(argv[i]))
      {
      this->RunningParaViewScript = 1;
      ui->LoadScript(argv[i]);
      this->RunningParaViewScript = 0;
      }
    }

  this->vtkKWApplication::Start(argc,argv);
  vtkOutputWindow::SetInstance(0);
  this->OutputWindow->Delete();
}


//----------------------------------------------------------------------------
vtkMultiProcessController *vtkPVApplication::NewController(int minId, int maxId)
{
#ifdef VTK_USE_MPI

  vtkMPICommunicator* localComm = vtkMPICommunicator::New();
  vtkMPIGroup* localGroup= vtkMPIGroup::New();
  vtkMPIController* localController = vtkMPIController::New();
  vtkMPICommunicator* worldComm = vtkMPICommunicator::GetWorldCommunicator();

  // I might want to pass the reference controller as a parameter.
  localGroup->Initialize( static_cast<vtkMPIController*>(this->Controller) );
  for(int i=minId; i<=maxId; i++)
    {
    localGroup->AddProcessId(i);
    }
  localComm->Initialize(worldComm, localGroup);
  localGroup->UnRegister(0);

  // Create a local controller (for the sub-group)
  localController->SetCommunicator(localComm);
  localComm->UnRegister(0);

  return localController;

#else
  return NULL;
#endif

}


//----------------------------------------------------------------------------
void vtkPVApplication::Exit()
{
  int id, myId, num;
  
  // Send a break RMI to each of the slaves.
  num = this->Controller->GetNumberOfProcesses();
  myId = this->Controller->GetLocalProcessId();
  
  this->vtkKWApplication::Exit();

  for (id = 0; id < num; ++id)
    {
    if (id != myId)
      {
      this->Controller->TriggerRMI(id, 
                                   vtkMultiProcessController::BREAK_RMI_TAG);
      }
    }

}


//----------------------------------------------------------------------------
void vtkPVApplication::SendDataBounds(vtkDataSet *data)
{
  float *bounds;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  bounds = data->GetBounds();
  this->Controller->Send(bounds, 6, 0, 1967);
}

//----------------------------------------------------------------------------
void vtkPVApplication::SendDataNumberOfCells(vtkDataSet *data)
{
  int num;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  num = data->GetNumberOfCells();
  this->Controller->Send(&num, 1, 0, 1968);
}

//----------------------------------------------------------------------------
void vtkPVApplication::SendDataNumberOfPoints(vtkDataSet *data)
{
  int num;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  num = data->GetNumberOfPoints();
  this->Controller->Send(&num, 1, 0, 1969);
}

//----------------------------------------------------------------------------
void vtkPVApplication::GetMapperColorRange(float range[2],
                                           vtkPolyDataMapper *mapper)
{
  vtkDataSetAttributes *attr = NULL;
  vtkDataArray *array;
  
  if (mapper == NULL || mapper->GetInput() == NULL)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    return;
    }

  // Determine and get the array used to color the model.
  if (mapper->GetScalarMode() == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
    {
    attr = mapper->GetInput()->GetPointData();
    }
  if (mapper->GetScalarMode() == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
    attr = mapper->GetInput()->GetCellData();
    }

  // Sanity check.
  if (attr == NULL)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    return;
    }

  array = attr->GetArray(mapper->GetArrayName());
  if (array == NULL)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    return;
    }

  array->GetRange( range, mapper->GetArrayComponent());
}


//----------------------------------------------------------------------------
void vtkPVApplication::SendMapperColorRange(vtkPolyDataMapper *mapper)
{
  float range[2];
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }

  this->GetMapperColorRange(range, mapper);
  this->Controller->Send(range, 2, 0, 1969);
}

//----------------------------------------------------------------------------
void vtkPVApplication::SendDataArrayRange(vtkDataSet *data, 
                                          int pointDataFlag, 
                                          char *arrayName,
                                          int component)
{
  float range[2];
  vtkDataArray *array;

  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  
  if (pointDataFlag)
    {
    array = data->GetPointData()->GetArray(arrayName);
    }
  else
    {
    array = data->GetCellData()->GetArray(arrayName);
    }

  if (array && component >= 0 && component < array->GetNumberOfComponents())
    {
    array->GetRange(range, component);
    }
  else
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    }

  this->Controller->Send(range, 2, 0, 1976);
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
  this->AddTraceEntry("set kw(%s) [$Application GetMainWindow]",
                      this->GetMainWindow()->GetTclName());
  this->GetMainWindow()->SetTraceInitialized(1);
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


//----------------------------------------------------------------------------
void vtkPVApplication::CompleteArrays(vtkMapper *mapper, char *mapperTclName)
{
  int i, j;
  int numProcs;
  int nonEmptyFlag = 0;
  int activeAttributes[5];

  if (mapper->GetInput() == NULL || this->Controller == NULL ||
      mapper->GetInput()->GetNumberOfPoints() > 0 ||
      mapper->GetInput()->GetNumberOfCells() > 0)
    {
    return;
    }

  // Find the first non empty data object on another processes.
  numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
    {
    this->RemoteScript(i, "$Application SendCompleteArrays %s", mapperTclName);
    this->Controller->Receive(&nonEmptyFlag, 1, i, 987243);
    if (nonEmptyFlag)
      { // This process has data.  Receive all the arrays, type and component.
      int num = 0;
      vtkDataArray *array = 0;
      char *name;
      int nameLength = 0;
      int type = 0;
      int numComps = 0;
      
      // First Point data.
      this->Controller->Receive(&num, 1, i, 987244);
      for (j = 0; j < num; ++j)
        {
        this->Controller->Receive(&type, 1, i, 987245);
        switch (type)
          {
          case VTK_INT:
            array = vtkIntArray::New();
            break;
          case VTK_FLOAT:
            array = vtkFloatArray::New();
            break;
          case VTK_DOUBLE:
            array = vtkDoubleArray::New();
            break;
          case VTK_CHAR:
            array = vtkCharArray::New();
            break;
          case VTK_LONG:
            array = vtkLongArray::New();
            break;
          case VTK_SHORT:
            array = vtkShortArray::New();
            break;
          case VTK_UNSIGNED_CHAR:
            array = vtkUnsignedCharArray::New();
            break;
          case VTK_UNSIGNED_INT:
            array = vtkUnsignedIntArray::New();
            break;
          case VTK_UNSIGNED_LONG:
            array = vtkUnsignedLongArray::New();
            break;
          case VTK_UNSIGNED_SHORT:
            array = vtkUnsignedShortArray::New();
            break;
          }
        this->Controller->Receive(&numComps, 1, i, 987246);
        array->SetNumberOfComponents(numComps);
        this->Controller->Receive(&nameLength, 1, i, 987247);
        name = new char[nameLength];
        this->Controller->Receive(name, nameLength, i, 987248);
        array->SetName(name);
        delete [] name;
        mapper->GetInput()->GetPointData()->AddArray(array);
        array->Delete();
        } // end of loop over point arrays.
      // Which scalars, ... are active?
      this->Controller->Receive(activeAttributes, 5, i, 987258);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[0],0);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[1],1);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[2],2);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[3],3);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[4],4);
 
      // Next Cell data.
      this->Controller->Receive(&num, 1, i, 987244);
      for (j = 0; j < num; ++j)
        {
        this->Controller->Receive(&type, 1, i, 987245);
        switch (type)
          {
          case VTK_INT:
            array = vtkIntArray::New();
            break;
          case VTK_FLOAT:
            array = vtkFloatArray::New();
            break;
          case VTK_DOUBLE:
            array = vtkDoubleArray::New();
            break;
          case VTK_CHAR:
            array = vtkCharArray::New();
            break;
          case VTK_LONG:
            array = vtkLongArray::New();
            break;
          case VTK_SHORT:
            array = vtkShortArray::New();
            break;
          case VTK_UNSIGNED_CHAR:
            array = vtkUnsignedCharArray::New();
            break;
          case VTK_UNSIGNED_INT:
            array = vtkUnsignedIntArray::New();
            break;
          case VTK_UNSIGNED_LONG:
            array = vtkUnsignedLongArray::New();
            break;
          case VTK_UNSIGNED_SHORT:
            array = vtkUnsignedShortArray::New();
            break;
          }
        this->Controller->Receive(&numComps, 1, i, 987246);
        array->SetNumberOfComponents(numComps);
        this->Controller->Receive(&nameLength, 1, i, 987247);
        name = new char[nameLength];
        this->Controller->Receive(name, nameLength, i, 987248);
        array->SetName(name);
        delete [] name;
        mapper->GetInput()->GetCellData()->AddArray(array);
        array->Delete();
        } // end of loop over cell arrays.
      // Which scalars, ... are active?
      this->Controller->Receive(activeAttributes, 5, i, 987258);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[0],0);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[1],1);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[2],2);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[3],3);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[4],4);
      
      // We only need information from one.
      return;
      } // End of if-non-empty check.
    }// End of loop over processes.
}



//----------------------------------------------------------------------------
void vtkPVApplication::SendCompleteArrays(vtkMapper *mapper)
{
  int nonEmptyFlag;
  int num;
  int i;
  int type;
  int numComps;
  int nameLength;
  const char *name;
  vtkDataArray *array;
  int activeAttributes[5];

  if (mapper->GetInput() == NULL ||
      (mapper->GetInput()->GetNumberOfPoints() == 0 &&
       mapper->GetInput()->GetNumberOfCells() == 0))
    {
    nonEmptyFlag = 0;
    this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);
    return;
    }
  nonEmptyFlag = 1;
  this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);

  // First point data.
  num = mapper->GetInput()->GetPointData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, 0, 987244);
  for (i = 0; i < num; ++i)
    {
    array = mapper->GetInput()->GetPointData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, 0, 987245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, 0, 987246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = vtkString::Length(name)+1;
    this->Controller->Send(&nameLength, 1, 0, 987247);
    // I am pretty sure that Send does not modify the string.
    this->Controller->Send(const_cast<char*>(name), nameLength, 0, 987248);
    }
  mapper->GetInput()->GetPointData()->GetAttributeIndices(activeAttributes);
  this->Controller->Send(activeAttributes, 5, 0, 987258);

  // Next cell data.
  num = mapper->GetInput()->GetCellData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, 0, 987244);
  for (i = 0; i < num; ++i)
    {
    array = mapper->GetInput()->GetCellData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, 0, 987245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, 0, 987246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = vtkString::Length(name+1);
    this->Controller->Send(&nameLength, 1, 0, 987247);
    this->Controller->Send(const_cast<char*>(name), nameLength, 0, 987248);
    }
  mapper->GetInput()->GetCellData()->GetAttributeIndices(activeAttributes);
  this->Controller->Send(activeAttributes, 5, 0, 987258);
}


void vtkPVApplication::SetGlobalLODFlag(int val)
{
  vtkPVApplication::GlobalLODFlag = val;

  if (this->Controller->GetLocalProcessId() == 0)
    {
    int idx, num;
    num = this->Controller->GetNumberOfProcesses();
    for (idx = 1; idx < num; ++idx)
      {
      this->RemoteScript(idx, "$Application SetGlobalLODFlag %d", val);
      }
    }
}

int vtkPVApplication::GetGlobalLODFlag()
{
  return vtkPVApplication::GlobalLODFlag;
}


//============================================================================
// Make instances of sources.
//============================================================================

//----------------------------------------------------------------------------
vtkObject *vtkPVApplication::MakeTclObject(const char *className,
                                           const char *tclName)
{
  this->BroadcastScript("%s %s", className, tclName);
  return this->TclToVTKObject(tclName);
}

//----------------------------------------------------------------------------
vtkObject *vtkPVApplication::TclToVTKObject(const char *tclName)
{
  vtkObject *o;
  int error;

  o = (vtkObject *)(vtkTclGetPointerFromObject(
                      tclName, "vtkObject", this->GetMainInterp(), error));
  
  if (o == NULL)
    {
    vtkErrorMacro("Could not get object from pointer.");
    }
  
  return o;
}

//----------------------------------------------------------------------------
void vtkPVApplication::DisplayAbout(vtkKWWindow *master)
{
//    ostrstream str;
//    str << this->GetApplicationName() << " was developed by Kitware Inc." << endl
//        << "http://www.paraview.org" << endl
//        << "http://www.kitware.com" << endl
//        << "This is version " << this->MajorVersion << "." << this->MinorVersion
//        << ", release " << this->GetApplicationReleaseName() << ends;

//    char* msg = str.str();
//    vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
//    dlg->SetTitle("About ParaView");
//    dlg->SetMasterWindow(master);
//    dlg->Create(this,"");
//    dlg->SetText(msg);
//    dlg->Invoke();  
//    dlg->Delete(); 
//    delete[] msg;

  this->SplashScreen->ShowWithBind();
}

void vtkPVApplication::DisplayHelp(vtkKWWindow* master)
{
#ifdef _WIN32
  char temp[1024];
  char loc[1024];
  vtkKWRegisteryUtilities *reg = this->GetRegistery();
  sprintf(temp, "%i", this->GetApplicationKey());
  reg->SetTopLevel(temp);
  if (reg->ReadValue("Inst", "Loc", loc))
    {
    sprintf(temp,"%s/%s.chm::/UsersGuide/index.html",
            loc,this->ApplicationName);
    }
  else
    {
    sprintf(temp,"%s.chm::/UsersGuide/index.html",this->ApplicationName);
    }
  if ( HtmlHelp(NULL, temp, HH_DISPLAY_TOPIC, 0) )
    {
    return;
    }
#endif
  vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
  dlg->SetTitle("ParaView Help");
  dlg->SetMasterWindow(master);
  dlg->Create(this,"");
  dlg->SetText(
    "HTML help is included in the Documentation/HTML subdirectory of\n"
    "this application. You can view this help using a standard web browser.");
  dlg->Invoke();  
  dlg->Delete();
}

//----------------------------------------------------------------------------
void vtkPVApplication::LogStartEvent(char* str)
{
  vtkTimerLog::MarkStartEvent(str);
}

//----------------------------------------------------------------------------
void vtkPVApplication::LogEndEvent(char* str)
{
  vtkTimerLog::MarkEndEvent(str);
}

#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
//----------------------------------------------------------------------------
void vtkPVApplication::SetupTrapsForSignals(int nodeid)
{
  vtkPVApplication::MainApplication = this;
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
    cout << "Call exit on application" << endl;
    vtkPVApplication::MainApplication->Exit();
    }
  cout << "Call exit on window" << endl;
  win->Exit();
}

//----------------------------------------------------------------------------
void vtkPVApplication::ErrorExit()
{
  // This { is here because compiler is smart enough to know that exit
  // exits the code without calling destructors. By adding this,
  // destructors are called before the exit.
  {
  cout << "There was a major error! Trying to exit..." << endl;
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
  os << indent << "Controller: " << this->Controller << endl;;
  os << indent << "MajorVersion: " << this->MajorVersion << endl;
  os << indent << "MinorVersion: " << this->MinorVersion << endl;
  os << indent << "RunningParaViewScript: " 
     << ( this->RunningParaViewScript ? "on" : " off" ) << endl;
  os << indent << "Current Process Id: " << this->ProcessId << endl;
  os << indent << "NumberOfPipes: " << this->NumberOfPipes << endl;
}

void vtkPVApplication::DisplayTCLError(const char* message)
{
  vtkErrorMacro("TclTk error: "<<message);
}

//----------------------------------------------------------------------------
const char* const vtkPVApplication::LoadComponentProc =
"namespace eval ::paraview {\n"
"    proc load_component {name {optional_paths {}}} {\n"
"        \n"
"        global tcl_platform auto_path env\n"
"        \n"
"        # First dir is empty, to let Tcl try in the current dir\n"
"        \n"
"        set dirs $optional_paths\n"
"        set dirs [concat $dirs {\"\"}]\n"
"        set ext [info sharedlibextension]\n"
"        if {$tcl_platform(platform) == \"unix\"} {\n"
"            set prefix \"lib\"\n"
"            # Help Unix a bit by browsing into $auto_path and /usr/lib...\n"
"            set dirs [concat $dirs /usr/local/lib /usr/local/lib/vtk $auto_path]\n"
"            if {[info exists env(LD_LIBRARY_PATH)]} {\n"
"                set dirs [concat $dirs [split $env(LD_LIBRARY_PATH) \":\"]]\n"
"            }\n"
"            if {[info exists env(PATH)]} {\n"
"                set dirs [concat $dirs [split $env(PATH) \":\"]]\n"
"            }\n"
"        } else {\n"
"            set prefix \"\"\n"
"            if {$tcl_platform(platform) == \"windows\"} {\n"
"                if {[info exists env(PATH)]} {\n"
"                    set dirs [concat $dirs [split $env(PATH) \";\"]]\n"
"                }\n"
"            }\n"
"        }\n"
"        \n"
"        foreach dir $dirs {\n"
"            set libname [file join $dir ${prefix}${name}${ext}]\n"
"            if {[file exists $libname]} {\n"
"                if {![catch {load $libname} errormsg]} {\n"
"                    # WARNING: it HAS to be \"\" so that pkg_mkIndex work (since\n"
"                    # while evaluating a package ::paraview::load_component won't\n"
"                    # exist and will default to the unknown() proc that \n"
"                    # returns \"\"\n"
"                    return \"\"\n"
"                } else {\n"
"                    # If not loaded but file was found, oops\n"
"                    puts stderr $errormsg\n"
"                }\n"
"            }\n"
"        }\n"
"        \n"
"        puts stderr \"::paraview::load_component: $name could not be found.\"\n"
"        \n"
"        return 1\n"
"    }\n"
"    namespace export load_component\n"
"}\n";
