/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIProcessModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMPIProcessModule
// .SECTION Description
// A class to encapaulate all of the process initialization,
// distributed data model and duplication  of the pipeline.
// Filters and compositers will still need a controller, 
// but every thing else should be handled here.  This class 
// sets up the default MPI processes with the user interface
// running on process 0.  I plan to make an alternative module
// for client server mode, where the client running the UI 
// is not in the MPI group but links to the MPI group through 
// a socket connection.

#ifndef __vtkPVMPIProcessModule_h
#define __vtkPVMPIProcessModule_h

#include "vtkPVProcessModule.h"
class vtkPVPart;
class vtkPVApplication;
class vtkMultiProcessController;
class vtkMapper;
class vtkDataSet;

class VTK_EXPORT vtkPVMPIProcessModule : public vtkPVProcessModule
{
public:
  static vtkPVMPIProcessModule* New();
  vtkTypeRevisionMacro(vtkPVMPIProcessModule,vtkPVProcessModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ParaView.cxx (main) calls this method to setup the processes.
  // It currently creates the application, but I will try to pass
  // the application as an argument.  Start calls Initialize
  // which calls vtkPVApplication::Start();
  virtual int Start(int argc, char **argv);
  void Initialize();
  
  // Description:
  // This breaks rmi loops and cleans up processes.`                
  virtual void Exit();

  // Description:
  // Send the current ClientServerStream data to different places and 
  // combinations of places.  Possible places are the Client, the 
  // Server (data server), or the RenderServer.  Also the stream
  // can be sent to the root of the render and data servers.
  // Most combinations are possible.
  virtual void SendStreamToClient();
  virtual void SendStreamToServer();
  virtual void SendStreamToRenderServer();
  virtual void SendStreamToServerRoot();
  virtual void SendStreamToRenderServerRoot(); 
  virtual void SendStreamToClientAndServerRoot();
  virtual void SendStreamToRenderServerAndServerRoot();
  virtual void SendStreamToClientAndRenderServerRoot(); 
  virtual void SendStreamToClientAndServer();
  virtual void SendStreamToClientAndRenderServer();
  virtual void SendStreamToRenderServerAndServer();
  virtual void SendStreamToRenderServerClientAndServer();
//BTX
  virtual void SendStreamToServerRootTemp(vtkClientServerStream* stream);
  virtual void SendStreamToServerTemp(vtkClientServerStream* stream);
//ETX
  
  // Description:
  // A method for getting generic information from the server.
  virtual void GatherInformationInternal(const char* infoClassName,
                                         vtkObject* object);
    
  // Description:
  // Get the partition number. -1 means no assigned partition.
  virtual int GetPartitionId();

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions();

  // Description:
  // Used internally.  Do not call.  Use LoadModule instead.
  virtual int LoadModuleInternal(const char* name, const char* directory);
protected:
  vtkPVMPIProcessModule();
  ~vtkPVMPIProcessModule();

  virtual void SendStreamToServerNodeInternal(int remoteId);
  virtual void SendStreamToServerInternal();

  virtual void SendStreamToServerNodeInternal(
    int remoteId, vtkClientServerStream* stream);
  virtual void SendStreamToServerInternal(vtkClientServerStream* stream);

  // To pass arguments through controller single method.
  int    ArgumentCount;
  char** Arguments;
  int    ReturnValue;

private:  
  vtkPVMPIProcessModule(const vtkPVMPIProcessModule&); // Not implemented
  void operator=(const vtkPVMPIProcessModule&); // Not implemented
};

#endif
