/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplication.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVApplication
// .SECTION Description
// A subclass of vtkKWApplication specific to this application.

#ifndef __vtkPVApplication_h
#define __vtkPVApplication_h

#include "vtkKWApplication.h"
class vtkPVProcessModule;
class vtkPVRenderModule;

class vtkDataSet;
class vtkKWMessageDialog;
class vtkMapper;
class vtkMultiProcessController;
class vtkSocketController;
class vtkPVOutputWindow;
class vtkPVSource;
class vtkPVWindow;
class vtkPVRenderView;
class vtkPolyDataMapper;
class vtkProbeFilter;
class vtkProcessObject;
class vtkPVApplicationObserver;
class vtkPVProgressHandler;

class VTK_EXPORT vtkPVApplication : public vtkKWApplication
{
public:
  static vtkPVApplication* New();
  vtkTypeRevisionMacro(vtkPVApplication,vtkKWApplication);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Parses the command line arguments and modifies the applications
  // ivars appropriately.
  // Return error (1) if the arguments are not formed properly.
  // Returns 0 if all went well.
  int ParseCommandLineArguments(int argc, char*argv[]);

  //BTX
  // Description:
  // Process module contains all methods for managing 
  // processes and communication.
  void SetProcessModule(vtkPVProcessModule *module);
  vtkPVProcessModule* GetProcessModule() { return this->ProcessModule;}
  
  // Description:
  // RenderingModule has the rendering abstraction.  
  // It creates the render window and any composit manager.  
  // It also creates part displays which handle level of details.
  void SetRenderModule(vtkPVRenderModule *module);
  vtkPVRenderModule* GetRenderModule() { return this->RenderModule;}
  //ETX

  // Description:
  // Start running the main application.
  virtual void Start(int argc, char *argv[]);
  virtual void Start()
    { this->vtkKWApplication::Start(); }
  virtual void Start(char* arg)
    { this->vtkKWApplication::Start(arg); }

  // Description:
  // We need to keep the controller in a prominent spot because there is no
  // more "RegisterAndGetGlobalController" method.
//BTX
  vtkMultiProcessController *GetController();
  
  // Description:
  // If ParaView is running in client server mode, then this returns
  // the socket controller used for client server communication.
  // It will only be set on the client and process 0 of the server.
  vtkSocketController *GetSocketController();
//ETX

  // Description:
  // No licence required.
  int AcceptLicense();
  int AcceptEvaluation();

  // Description:
  // This method is invoked when a window closes
  virtual void Close(vtkKWWindow *);

  // Description:
  // We need to kill the slave processes
  virtual void Exit();
  
  // Description:
  // Initialize Tcl/Tk
  // Return NULL on error (eventually provides an ostream where detailed
  // error messages will be stored).
  //BTX
  static Tcl_Interp *InitializeTcl(int argc, char *argv[], ostream *err = 0);
  //ETX
  
  // Description:
  // Perform internal PV Application initialization.
  void Initialize();

//BTX
#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
  // Description:
  // Setup traps for signals that may kill ParaView.
  void SetupTrapsForSignals(int nodeid);
  static void TrapsForSignals(int signal);

  // Description:
  // When error happens, try to exit as gracefully as you can.
  static void ErrorExit();
#endif // PV_HAVE_TRAPS_FOR_SIGNALS
//ETX

  // Description:
  // A start at recording macros in ParaView.  Create a custom trace file
  // that can be loaded back into paraview.  Window variables get
  // initialized when the file is opened.
  // Note: The trace entries get diverted to this file.  This is only used
  // for testing at the moment.  It is restricted to using sources created
  // after the recording is started.  The macro also cannot use the glyph
  // sources.  To make mocro recording available to the user, then there
  // must be a way of setting arguments (existing sources) to the macro,
  // and a way of prompting the user to set the arguments when the
  // macro/script is loaded.
  void StartRecordingScript(char *filename);
  void StopRecordingScript();

  // Description:
  // Since ParaView has only one window, we might as well provide access to it.
  vtkPVWindow *GetMainWindow();
  vtkPVRenderView *GetMainView();

  // Description:
  // Display the on-line help and about dialog for this application.
  // Over-writing vtkKWApplication defaults.
  void DisplayHelp(vtkKWWindow* master);

  // For locating help (.chm) on Windows.
  virtual int GetApplicationKey() 
    {
      return 15;
    };

  // Description:
  // Need to put a global flag that indicates interactive rendering.  All
  // process must be consistent in choosing LODs because of the
  // vtkCollectPolydata filter.  This has to be in vtkPVApplication
  // because we do not create a render module on remote processes.
  void SetGlobalLODFlag(int val);
  static int GetGlobalLODFlag();
  void SetGlobalLODFlagInternal(int val);

  // Description:
  // For loggin from Tcl start and end execute events.  We do not have c
  // pointers to all filters.
  void LogStartEvent(char* str);
  void LogEndEvent(char* str);
  void RegisterProgressEvent(vtkProcessObject* po, int id);

  // Description:
  // More timer log access methods.  Static methods are not accessible 
  // from tcl.  We need a timer object on all procs.
  void SetLogBufferLength(int length);
  void ResetLog();
  void SetEnableLog(int flag);

  // Description:
  // Time threshold for event (start-end) when getting the log with indents.
  // We do not have a timer object on all procs.  Statics do not work with Tcl.
  vtkSetMacro(LogThreshold, float);
  vtkGetMacro(LogThreshold, float);

  // Description:
  // Flag showing whether the commands are being executed from
  // a ParaView script.
  vtkSetMacro(RunningParaViewScript, int);
  vtkGetMacro(RunningParaViewScript, int);

  // Description:
  // Tells the process modules whether to start the main
  // event loop. Mainly used by command line argument parsing code
  // when an argument requires not starting the GUI
  vtkSetMacro(StartGUI, int);
  vtkGetMacro(StartGUI, int);

  //BTX
  static const char* const ExitProc;
  //ETX

  void DisplayTCLError(const char* message);

  // Description:
  // A method used to set environment variables in the satellite
  // processes. This method leaks memory and for now should be called
  // only once.
  void SetEnvironmentVariable(const char* string);

  // Description: 
  // Set or get the display 3D widgets flag.  When this flag is set,
  // the 3D widgets will be displayed when they are created. Otherwise
  // user has to enable them. User will still be able to disable the
  // 3D widget.
  vtkSetClampMacro(Display3DWidgets, int, 0, 1);
  vtkBooleanMacro(Display3DWidgets, int);
  vtkGetMacro(Display3DWidgets, int);

  // Description: 
  // Are we using a subset to render?
  vtkGetMacro(UseRenderingGroup, int);

  // Description:
  // Varibles (command line argurments) set to render to a tiled display.
  vtkGetMacro(UseTiledDisplay, int);
  vtkGetVector2Macro(TileDimensions, int);

  // Description:
  // Variable set by command line arguments --client or -c
  // Client mode tries to connect to a server through a socket.
  // The client does not have any local partitioned data.
  vtkGetMacro(ClientMode,int);

  // Description:
  // Variable set by command line arguments --server or -v
  // Server can have many processes, but has no UI.
  vtkGetMacro(ServerMode,int);

  // Description:
  // Variable set if the separate render server and data server
  // are being used.
  vtkGetMacro(RenderServerMode,int);

  // Description:
  // Get the host command line option. (--host=localhost).
  vtkGetStringMacro(RenderServerHostName);
  vtkGetStringMacro(HostName);
  vtkGetStringMacro(Username);

  // Description:
  // The the port for the client/server socket connection.
  vtkGetMacro(Port,int);

  // Description:
  // The the port for the client/render server socket connection.
  vtkGetMacro(RenderServerPort,int);
  // Description:
  // The the port for the client/render server socket connection.
  vtkGetMacro(RenderNodePort,int);

  // Description:
  // The default behavior is for the server to wait and for the client 
  // to connect to the server.  When this flag is set by the command line 
  // arguments.  The server tries to connect to the client instead.
  vtkGetMacro(ReverseConnection,int);

  // Description:
  // Variable set by command line arguments --client or -c
  // Client mode tries to connect to a server through a socket.
  // The client does not have any local partitioned data.
  vtkGetMacro(UseStereoRendering,int);

  // Description:
  // Set by the command line arguments --use-software-rendering or -r
  // Requires ParaView is linked with mangled mesa.
  // Supports off screen rendering.
  vtkGetMacro(UseSoftwareRendering,int);

  // Description:
  // Set by the command line arguments --use-satellite-software or -s
  // Requires ParaView is linked with mangled mesa.
  // Satellite processes use mesa (supports offscreen) while root
  // can still use hardware acceleration.
  vtkGetMacro(UseSatelliteSoftware,int);

  // Description:
  // Set by the command line arguments --use-offscreen-rendering or -os
  // Requires that ParaView is linked with mangled mesa or that sofware
  // rendering is enabled on Unix.
  // Satellite processes render offscreen.
  vtkGetMacro(UseOffscreenRendering,int);

  // Description:
  // Set by the command line arguments --start-empty or -e
  // This flag is set when ParaView was started without the default modules.
  vtkGetMacro(StartEmpty,int);

  // Description:
  // Set by the command line arguments --disable-composite or -dc.
  // You can use this option when redering resources are not available on
  // the server.
  vtkGetMacro(DisableComposite,int);

  // Description:
  // This is used internally for specifying how many pipes
  // to use for rendering when UseRenderingGroup is defined.
  // All processes have this set to the same value.
  vtkSetMacro(NumberOfPipes, int);
  vtkGetMacro(NumberOfPipes, int);

  // Description:
  // This root class name will eventually be replaced
  // with an XML specification of rendering module classes.
  vtkGetStringMacro(RenderModuleName);

  // Description:
  // I have ParaView.cxx set the proper default render module.
  vtkSetStringMacro(RenderModuleName);  

  // Description:
  // This is used (Unix only) to obtain the path of the executable.
  // This path is used to locate demos etc.
  vtkGetStringMacro(Argv0);

  // Description:
  // This is used by the render server only.
  vtkGetStringMacro(MachinesFileName);

  // Description:
  // This is used by the cave render module only.
  vtkGetStringMacro(CaveConfigurationFileName);

  // Description:
  // The name of the trace file.
  vtkGetStringMacro(TraceFileName);

  vtkSetClampMacro(AlwaysSSH, int, 0, 1);
  vtkBooleanMacro(AlwaysSSH, int);
  vtkGetMacro(AlwaysSSH, int);

  // Descrition:
  // Show/Hide the sources long help.
  virtual void SetShowSourcesLongHelp(int);
  vtkGetMacro(ShowSourcesLongHelp, int);
  vtkBooleanMacro(ShowSourcesLongHelp, int);

  // Descrition:
  // Show/Hide the sources long help.
  virtual void SetSourcesBrowserAlwaysShowName(int);
  vtkGetMacro(SourcesBrowserAlwaysShowName, int);
  vtkBooleanMacro(SourcesBrowserAlwaysShowName, int);

  // Descrition:
  // Get those application settings that are stored in the registery
  // Should be called once the application name is known (and the registery
  // level set).
  virtual void GetApplicationSettingsFromRegistery();

  // Description:
  // We need to get the data path for the demo on the server.
  char* GetDemoPath();

  // Description:
  // Enable or disable test errors. This refers to wether errors make test fail
  // or not.
  void EnableTestErrors();
  void DisableTestErrors();

  // Description:
  // This is a debug feature of ParaView. If this is set, ParaView will crash
  // on errors.
  vtkSetClampMacro(CrashOnErrors, int, 0, 1);
  vtkBooleanMacro(CrashOnErrors, int);
  vtkGetMacro(CrashOnErrors, int);

  // Description:
  // Abort execution and display errors.
  static void Abort();

  // Description:
  // Execute event on callback
  void ExecuteEvent(vtkObject *o, unsigned long event, void* calldata);


  // Description:
  void SendPrepareProgress();
  void SendCleanupPendingProgress();

  // Description:
  // This method is called before progress reports start comming.
  void PrepareProgress();

  // Description:
  // This method is called after force update to clenaup all the pending
  // progresses.
  void CleanupPendingProgress();

  // Description:
  // Get number of partitions.
  int GetNumberOfPartitions();
  
  // Description:
  // Send string to client and server. This is here so that tcl scripts can
  // access it.
  void SendStringToClientAndServer(const char*);

  // Description:
  // Play the demo
  void PlayDemo(int fromDashboard);
  
  // Description:
  // Return the textual representation of the composite (i.e. its name and/or
  // its description. Memory is allocated, a pointer is return, it's up to
  // the caller to delete it.
  char* GetTextRepresentation(vtkPVSource* comp);

protected:
  vtkPVApplication();
  ~vtkPVApplication();

  virtual void CreateSplashScreen();
  virtual void AddAboutText(ostream &);

  void ProgressEvent(vtkObject *o, int val, const char* filter);

  void CreateButtonPhotos();
  void CreatePhoto(const char *name, 
                   const unsigned char *data, 
                   int width, int height, int pixel_size, 
                   unsigned long buffer_length = 0,
                   const char *filename = 0);
  int CheckRegistration();
  int PromptRegistration(char *,char *);

  vtkPVProcessModule *ProcessModule;
  vtkPVRenderModule *RenderModule;
  char* RenderModuleName;

  // For running with SGI pipes.
  int NumberOfPipes;

  int Display3DWidgets;

  int StartGUI;

  int RunBatchScript;

  char* BatchScriptName;
  vtkSetStringMacro(BatchScriptName);
  vtkGetStringMacro(BatchScriptName);

  // Command line arguments.
  int ClientMode;  // true if this is the client
  int ServerMode;  // true if this is the server or data server
  int RenderServerMode;  // true if this uses the render server
  char* HostName;
  char* RenderServerHostName;
  vtkSetStringMacro(RenderServerHostName);
  vtkSetStringMacro(HostName);
  char* Username;
  vtkSetStringMacro(Username);
  int Port;
  int RenderServerPort;
  int RenderNodePort;
  int AlwaysSSH;
  int ReverseConnection;
  int UseSoftwareRendering;
  int UseSatelliteSoftware;
  int UseStereoRendering;
  int StartEmpty;
  int PlayDemoFlag;
  int UseRenderingGroup;
  int UseOffscreenRendering;
  char* GroupFileName;
  vtkSetStringMacro(GroupFileName);
  int UseTiledDisplay;
  int TileDimensions[2];
  int DisableComposite;

  // Need to put a global flag that indicates interactive rendering.
  // All process must be consistent in choosing LODs because
  // of the vtkCollectPolydata filter.
  static int GlobalLODFlag;

  int RunningParaViewScript;
  
  vtkPVOutputWindow *OutputWindow;

  static int CheckForExtension(const char* arg, const char* ext);
  char* CreateHelpString();
  int CheckForTraceFile(char* name, unsigned int len);
  void DeleteTraceFiles(char* name, int all);
  void SaveTraceFile(const char* fname);

  vtkSetStringMacro(MachinesFileName);
  char* MachinesFileName;
  vtkSetStringMacro(CaveConfigurationFileName);
  char* CaveConfigurationFileName;
  vtkSetStringMacro(TraceFileName);
  char* TraceFileName;
  char* Argv0;
  vtkSetStringMacro(Argv0);

  //BTX
  enum
  {
    NUM_ARGS=100
  };
  static const char ArgumentList[vtkPVApplication::NUM_ARGS][128];
  //ETX

  static vtkPVApplication* MainApplication;  

  int ShowSourcesLongHelp;
  int SourcesBrowserAlwaysShowName;

  char* DemoPath;
  vtkSetStringMacro(DemoPath);

  float LogThreshold;

  int CrashOnErrors;

  vtkPVApplicationObserver* Observer;
  vtkPVProgressHandler* ProgressHandler;
  int ProgressEnabled;
  int ProgressRequests;

  int ApplicationInitialized;

private:  
  vtkPVApplication(const vtkPVApplication&); // Not implemented
  void operator=(const vtkPVApplication&); // Not implemented
};

#endif


