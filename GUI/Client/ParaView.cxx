/*=========================================================================

Program:   ParaView
Module:    ParaView.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h"
#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

#include "vtkKWRemoteExecute.h"
#include "vtkMultiProcessController.h"
#include "vtkOutputWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVConfig.h"
#include "vtkPVMPIProcessModule.h"

#include "vtkObject.h"
#include "vtkTclUtil.h"
#include "vtkTimerLog.h"

/*
 * Make sure all the kits register their classes with vtkInstantiator.
 * Since ParaView uses Tcl wrapping, all of VTK is already compiled in
 * anyway.  The instantiators will add no more code for the linker to
 * collect.
 */
#include "vtkCommonInstantiator.h"
#include "vtkFilteringInstantiator.h"
#include "vtkIOInstantiator.h"
#include "vtkImagingInstantiator.h"
#include "vtkGraphicsInstantiator.h"

#ifdef VTK_USE_RENDERING
#include "vtkRenderingInstantiator.h"
#endif

#ifdef VTK_USE_PATENTED
#include "vtkPatentedInstantiator.h"
#endif

#ifdef VTK_USE_HYBRID
#include "vtkHybridInstantiator.h"
#endif

#ifdef VTK_USE_PARALLEL
#include "vtkParallelInstantiator.h"
#endif

#include "vtkParaViewInstantiator.h"

static void ParaViewEnableMSVCDebugHook();
static void ParaViewEnableWindowsExceptionFilter();
static void ParaViewInitializeInterpreter(vtkPVProcessModule* pm);

//----------------------------------------------------------------------------
int MyMain(int argc, char *argv[])
{
  int retVal = 0;
  int startVal = 0;
  int myId = 0;
  vtkPVProcessModule *pm;
  vtkPVApplication *app;
  ParaViewEnableMSVCDebugHook();
  ParaViewEnableWindowsExceptionFilter();

#ifdef VTK_USE_MPI
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);
  // Might as well get our process ID here.  I use it to determine
  // Whether to initialize tk.  Once again, splitting Tk and Tcl 
  // initialization would clean things up.
  MPI_Comm_rank(MPI_COMM_WORLD,&myId); 
#endif

  // Don't prompt the user with startup errors on unix.
#if defined(_WIN32) && !defined(__CYGWIN__)
  vtkOutputWindow::GetInstance()->PromptUserOn();
#else
  vtkOutputWindow::GetInstance()->PromptUserOff();
#endif

  // The server is a special case.  We do not initialize Tk for process 0.
  // I would rather have application find this command line option, but
  // I cannot create an application before I initialize Tcl.
  // I could clean this up if I separate the initialization of Tk and Tcl.
  // I do not do this because it would affect other applications.
  int serverMode = 0;
  int idx;
  for (idx = 0; idx < argc; ++idx)
    {
    if (strcmp(argv[idx],"--server") == 0 || strcmp(argv[idx],"-v") == 0)
      {
      serverMode = 1;
      }
    }

  // Initialize Tcl/Tk.
  Tcl_Interp *interp;
  if (serverMode || myId > 0)
    { // DO not initialize Tk.
    vtkKWApplication::SetWidgetVisibility(0);
    }

  ostrstream err;
  interp = vtkPVApplication::InitializeTcl(argc, argv, &err);
  err << ends;
  if (!interp)
    {
#ifdef _WIN32
    ::MessageBox(0, err.str(), 
                 "ParaView error: InitializeTcl failed", MB_ICONERROR|MB_OK);
#else
    cerr << "ParaView error: InitializeTcl failed" << endl 
         << err.str() << endl;
#endif
    err.rdbuf()->freeze(0);
    return 1;
    }
  err.rdbuf()->freeze(0);

  // Create the application to parse the command line arguments.
  app = vtkPVApplication::New();

  if (myId == 0 && app->ParseCommandLineArguments(argc, argv))
    {
    retVal = 1;
    app->SetStartGUI(0);
    app->Exit();
    }
  else
    {
    // Get the application settings from the registery
    // It has to be called now, after ParseCommandLineArguments, which can 
    // change the registery level (also, it can not be called in the application
    // constructor or even the KWApplication constructor since we need the
    // application name to be set)

    app->GetApplicationSettingsFromRegistery();

    // Create the proper default render module.
    // Only the root server processes args.
    if (app->GetUseTiledDisplay())
      {
      if (app->GetRenderModuleName() == NULL)
        { // I do not like this initialization here.
        // Think about moving it.
#ifdef PARAVIEW_USE_ICE_T
        app->SetRenderModuleName("IceTRenderModule");
#else
        app->SetRenderModuleName("MultiDisplayRenderModule");
#endif
        }
      }
    else
      {
#ifdef VTK_USE_MPI
      if (app->GetRenderModuleName() == NULL)
        { // I do not like this initialization here.
        // Think about moving it.
        app->SetRenderModuleName("MPIRenderModule");
        }
#else
      if (app->GetRenderModuleName() == NULL)
        { // I do not like this initialization here.
        // Think about moving it.
        if (app->GetClientMode())
          {
          app->SetRenderModuleName("MPIRenderModule");
          }
        else
          {
          app->SetRenderModuleName("LODRenderModule");
          }
        }
#endif
      }

    // Create the process module for initializing the processes.
    // Only the root server processes args.
    if (app->GetClientMode() || serverMode) 
      {
      vtkPVClientServerModule *processModule = vtkPVClientServerModule::New();
      pm = processModule;
      }
    else
      {
#ifdef VTK_USE_MPI
      vtkPVMPIProcessModule *processModule = vtkPVMPIProcessModule::New();
#else 
      vtkPVProcessModule *processModule = vtkPVProcessModule::New();
#endif
      pm = processModule;
      }

    pm->SetApplication(app);
    app->SetProcessModule(pm);
    pm->InitializeInterpreter();
    ParaViewInitializeInterpreter(pm);

    // Start the application's event loop.  This will enable
    // vtkOutputWindow's user prompting for any further errors now that
    // startup is completed.
    if ( retVal )
      {
      app->Exit();
      }
    else
      {
      startVal = pm->Start(argc, argv);
      }

    // Clean up for exit.
    pm->FinalizeInterpreter();
    pm->Delete();
    pm = NULL;
    }

  // free some memory
  vtkTimerLog::CleanupLog();
  app->Delete();
  Tcl_DeleteInterp(interp);
  Tcl_Finalize();

  return (retVal?retVal:startVal);
}

#ifdef _WIN32
#include <windows.h>

int __stdcall WinMain(HINSTANCE vtkNotUsed(hInstance), 
                      HINSTANCE vtkNotUsed(hPrevInstance),
                      LPSTR lpCmdLine, int vtkNotUsed(nShowCmd))
{
  int          argc;
  int          retVal;
  char**       argv;
  unsigned int i;
  int          j;

  // enable floating point exceptions on MSVC
//  short m = 0x372;
//  __asm
//    {
//    fldcw m;
//    }

  // parse a few of the command line arguments
  // a space delimites an argument except when it is inside a quote

  argc = 1;
  int pos = 0;
  for (i = 0; i < strlen(lpCmdLine); i++)
    {
    while (lpCmdLine[i] == ' ' && i < strlen(lpCmdLine))
      {
      i++;
      }
    if (lpCmdLine[i] == '\"')
      {
      i++;
      while (lpCmdLine[i] != '\"' && i < strlen(lpCmdLine))
        {
        i++;
        pos++;
        }
      argc++;
      pos = 0;
      }
    else
      {
      while (lpCmdLine[i] != ' ' && i < strlen(lpCmdLine))
        {
        i++;
        pos++;
        }
      argc++;
      pos = 0;
      }
    }

  argv = (char**)malloc(sizeof(char*)* (argc+1));

  argv[0] = (char*)malloc(1024);
  ::GetModuleFileName(0, argv[0],1024);

  for(j=1; j<argc; j++)
    {
    argv[j] = (char*)malloc(strlen(lpCmdLine)+10);
    }
  argv[argc] = 0;

  argc = 1;
  pos = 0;
  for (i = 0; i < strlen(lpCmdLine); i++)
    {
    while (lpCmdLine[i] == ' ' && i < strlen(lpCmdLine))
      {
      i++;
      }
    if (lpCmdLine[i] == '\"')
      {
      i++;
      while (lpCmdLine[i] != '\"' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    else
      {
      while (lpCmdLine[i] != ' ' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    }
  argv[argc] = 0;

  // Initialize the processes and start the application.
  retVal = MyMain(argc, argv);

  // Delete arguments
  for(j=0; j<argc; j++)
    {
    free(argv[j]);
    }
  free(argv);

  return retVal;;
}
#else
int main(int argc, char *argv[])
{
  return MyMain(argc, argv);
}
#endif

// For a DEBUG build on MSVC, add a hook to prevent error dialogs when
// being run from DART.
#if defined(_MSC_VER) && defined(_DEBUG)
# include <crtdbg.h>
static int ParaViewDebugReport(int, char* message, int*)
{
  fprintf(stderr, message);
  exit(1);
  return 0;
}
void ParaViewEnableMSVCDebugHook()
{
  if(getenv("DART_TEST_FROM_DART"))
    {
    _CrtSetReportHook(ParaViewDebugReport);
    }
}
#else
void ParaViewEnableMSVCDebugHook()
{
}
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
# include <windows.h>
static LONG __stdcall
ParaViewUnhandledExceptionFilter(EXCEPTION_POINTERS* e)
{
  ExitProcess(e->ExceptionRecord->ExceptionCode);
  return 0;
}
static void ParaViewEnableWindowsExceptionFilter()
{
  if(getenv("DART_TEST_FROM_DART"))
    {
    SetUnhandledExceptionFilter(&ParaViewUnhandledExceptionFilter);    
    }
}
#else
static void ParaViewEnableWindowsExceptionFilter()
{
}
#endif

//----------------------------------------------------------------------------
// ClientServer wrapper initialization functions.
extern "C" void vtkCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkImagingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGraphicsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkIOCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkHybridCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkParallelCS_Initialize(vtkClientServerInterpreter*);
#ifdef VTK_USE_PATENTED
extern "C" void vtkPatentedCS_Initialize(vtkClientServerInterpreter*);
#endif
extern "C" void vtkPVFiltersCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkParaViewServerCS_Initialize(vtkClientServerInterpreter*);
#ifdef PARAVIEW_LINK_XDMF
extern "C" void vtkXdmfCS_Initialize(vtkClientServerInterpreter *);
#endif
#ifdef PARAVIEW_BUILD_DEVELOPMENT
extern "C" void vtkPVDevelopmentCS_Initialize(vtkClientServerInterpreter *);
#endif

//----------------------------------------------------------------------------
void ParaViewInitializeInterpreter(vtkPVProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkCommonCS_Initialize(pm->GetInterpreter());
  vtkFilteringCS_Initialize(pm->GetInterpreter());
  vtkImagingCS_Initialize(pm->GetInterpreter());
  vtkGraphicsCS_Initialize(pm->GetInterpreter());
  vtkIOCS_Initialize(pm->GetInterpreter());
  vtkRenderingCS_Initialize(pm->GetInterpreter());
  vtkHybridCS_Initialize(pm->GetInterpreter());
  vtkParallelCS_Initialize(pm->GetInterpreter());
#ifdef VTK_USE_PATENTED
  vtkPatentedCS_Initialize(pm->GetInterpreter());
#endif
  vtkPVFiltersCS_Initialize(pm->GetInterpreter());
  vtkParaViewServerCS_Initialize(pm->GetInterpreter());
#ifdef PARAVIEW_LINK_XDMF
  vtkXdmfCS_Initialize(pm->GetInterpreter());
#endif
#ifdef PARAVIEW_BUILD_DEVELOPMENT
  vtkPVDevelopmentCS_Initialize(pm->GetInterpreter());
#endif
}
