/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplication.h
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
// .NAME vtkPVApplication
// .SECTION Description
// A subclass of vtkKWApplication specific to this application.

#ifndef __vtkPVApplication_h
#define __vtkPVApplication_h

#include "vtkKWApplication.h"

class vtkDataSet;
class vtkMapper;
class vtkMultiProcessController;
class vtkPVOutputWindow;
class vtkPVSource;
class vtkPVWindow;
class vtkPolyDataMapper;
class vtkProbeFilter;

#define VTK_PV_SLAVE_SCRIPT_RMI_TAG 1150
#define VTK_PV_SLAVE_SCRIPT_COMMAND_LENGTH_TAG 1100
#define VTK_PV_SLAVE_SCRIPT_COMMAND_TAG 1120
#define VTK_PV_SLAVE_SCRIPT_RESULT_LENGTH_TAG 1130
#define VTK_PV_SLAVE_SCRIPT_RESULT_TAG 1140

class VTK_EXPORT vtkPVApplication : public vtkKWApplication
{
public:
  static vtkPVApplication* New();
  vtkTypeRevisionMacro(vtkPVApplication,vtkKWApplication);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Start running the main application.
  virtual void Start(int argc, char *argv[]);
  virtual void Start()
    { this->vtkKWApplication::Start(); }
  virtual void Start(char* arg)
    { this->vtkKWApplication::Start(arg); }

  
//BTX
  // Description:
  // Script which is executed in the remot processes.
  // If a result string is passed in, the results are place in it. 
  void RemoteScript(int remoteId, char *EventString, ...);

  // Description:
  // Can only be called by process 0.  It executes a script on every other
  // process.
  void BroadcastScript(char *EventString, ...);
//ETX
  void RemoteSimpleScript(int remoteId, const char *str);
  void BroadcastSimpleScript(const char *str);
  
  // Description:
  // We need to keep the controller in a prominent spot because there is no
  // more "RegisterAndGetGlobalController" method.
  void SetController(vtkMultiProcessController *c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  
  // Description:
  // Make sure the user accepts the license before running.
  int AcceptLicense();
  int AcceptEvaluation();

  // Description:
  // We need to kill the slave processes
  virtual void Exit();
  
  // Description:
  // class static method to initialize Tcl/Tk
  static Tcl_Interp *InitializeTcl(int argc, char *argv[]);

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
  // This constructs a vtk object (type specified by class name) and
  // uses the tclName for the tcl instance command.  The user must
  // cast to the correct type, and is responsible for deleting the
  // object.
  vtkObject *MakeTclObject(const char *className,
                           const char *tclName);

  // Description:
  // This method returns pointer to the object specified as a tcl
  // name.
  vtkObject *TclToVTKObject(const char *tclName);

  // Description:
  // When ParaView needs to query data on other procs, it needs a way to
  // get the information back (only VTK object on satellite procs).
  // These methods send the requested data to proc 0 with a tag of 1966.
  // Note:  Process 0 returns without sending.
  // These should probably be consolidated into one GetDataInfo method.
  void SendDataBounds(vtkDataSet *data);
  void SendDataNumberOfCells(vtkDataSet *data);
  void SendDataNumberOfPoints(vtkDataSet *data);
  void SendMapperColorRange(vtkPolyDataMapper *mapper);
  void SendDataArrayRange(vtkDataSet *data, int pointDataFlag,
                          char *arrayName, int component);
  
  // Description:
  // A method that should probably be in the mapper.
  void GetMapperColorRange(float range[2], vtkPolyDataMapper *mapper);
  
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
  // Temporary fix because empty VTK objects do not have arrays.
  // This will create arrays if they exist on other processes.
  void CompleteArrays(vtkMapper *mapper, char *mapperTclName);
  void SendCompleteArrays(vtkMapper *mapper);

  // Description:
  // Since ParaView has only one window, we might as well provide access to it.
  vtkPVWindow *GetMainWindow();

  // Description:
  // ParaView version is always MajorVersion.MinorVersion.
  // Change these in the constructor.
  vtkGetMacro(MajorVersion, int);
  vtkGetMacro(MinorVersion, int);

  // Description:
  // Display the on-line help and about dialog for this application.
  // Over-writing vtkKWApplication defaults.
  void DisplayHelp(vtkKWWindow* master);
  virtual void DisplayAbout(vtkKWWindow *master);

  // For locating help (.chm) on Windows.
  virtual int GetApplicationKey() 
    {
      return 15;
    };

  // Description:
  // Need to put a global flag that indicates interactive rendering.  All
  // process must be consistent in choosing LODs because of the
  // vtkCollectPolydata filter.
  void SetGlobalLODFlag(int val);
  static int GetGlobalLODFlag();

  // Description:
  // For loggin from Tcl start and end execute events.  We do not have c
  // pointers to all filters.
  void LogStartEvent(char* str);
  void LogEndEvent(char* str);

  // Description:
  // Flag showing whether the commands are being executed from
  // a ParaView script.
  vtkSetMacro(RunningParaViewScript, int);
  vtkGetMacro(RunningParaViewScript, int);

  //BTX
  static const char* const LoadComponentProc;
  //ETX

  void DisplayTCLError(const char* message);

  // Description:
  // Get the process Id when running in MPI mode.
  vtkGetMacro(ProcessId, int);

  // Description:
  // This is used internally for specifying how many SGI pipes
  // to use for rendering which PV_USE_SGI_PIPES is defined.
  // All processes have this set to the same value.
  vtkSetMacro(NumberOfPipes, int);
  vtkGetMacro(NumberOfPipes, int);

  // Description:
  // A helper class to create a controller/communicator pair
  // the has a restricted set of processes as its world.
  vtkMultiProcessController *NewController(int minId, int maxId);

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

protected:
  vtkPVApplication();
  ~vtkPVApplication();

  void CreateButtonPhotos();
  void CreateSplashScreen();
  void CreatePhoto(char *name, unsigned char *data, int width, int height, int pixel_size, char *filename = 0);
  int CheckRegistration();
  int PromptRegistration(char *,char *);

  int MajorVersion;
  int MinorVersion;

  // For running with SGI pipes.
  int NumberOfPipes;

  int ProcessId;

  int Display3DWidgets;

  // Need to put a global flag that indicates interactive rendering.
  // All process must be consistent in choosing LODs because
  // of the vtkCollectPolydata filter.
  static int GlobalLODFlag;

  int RunningParaViewScript;

  vtkMultiProcessController *Controller;
  
  vtkPVOutputWindow *OutputWindow;

  static int CheckForArgument(int argc, char* argv[], const char* arg,
                              int& index);
  static int IsParaViewScriptFile(const char* arg);
  char* CreateHelpString();

  //BTX
  enum
  {
    NUM_ARGS=100
  };
  static const char ArgumentList[vtkPVApplication::NUM_ARGS][128];
  //ETX

  static vtkPVApplication* MainApplication;  

private:  
  vtkPVApplication(const vtkPVApplication&); // Not implemented
  void operator=(const vtkPVApplication&); // Not implemented
};

#endif


