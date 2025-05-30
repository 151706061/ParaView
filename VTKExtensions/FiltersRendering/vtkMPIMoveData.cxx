// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMPIMoveData.h"

#include "vtkAllToNRedistributeCompositePolyData.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTypes.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessControllerHelper.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPVLogger.h"
#include "vtkPVSession.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"

#include "vtk_zlib.h"
#include <sstream>
#include <vector>

bool vtkMPIMoveData::UseZLibCompression = false;

namespace
{
bool vtkMPIMoveDataMerge(std::vector<vtkSmartPointer<vtkDataObject>>& pieces, vtkDataObject* result)
{
  return vtkMultiProcessControllerHelper::MergePieces(pieces, result);
}

void unsetGlobalIdsAttribute(vtkDataObject* piece)
{
  vtkDataSet* ds = vtkDataSet::SafeDownCast(piece);
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(piece);
  if (ds)
  {
    ds->GetCellData()->SetActiveAttribute(-1, vtkDataSetAttributes::GLOBALIDS);
    ds->GetPointData()->SetActiveAttribute(-1, vtkDataSetAttributes::GLOBALIDS);
  }
  else if (mb)
  {
    vtkCompositeDataIterator* it = mb->NewIterator();
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      vtkDataSet* leaf = vtkDataSet::SafeDownCast(it->GetCurrentDataObject());
      if (leaf)
      {
        leaf->GetCellData()->SetActiveAttribute(-1, vtkDataSetAttributes::GLOBALIDS);
        leaf->GetPointData()->SetActiveAttribute(-1, vtkDataSetAttributes::GLOBALIDS);
      }
    }
    it->Delete();
  }
}
};

vtkStandardNewMacro(vtkMPIMoveData);

vtkCxxSetObjectMacro(vtkMPIMoveData, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkMPIMoveData, ClientDataServerSocketController, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkMPIMoveData, MPIMToNSocketConnection, vtkMPIMToNSocketConnection);
//-----------------------------------------------------------------------------
vtkMPIMoveData::vtkMPIMoveData()
{
  this->Controller = nullptr;
  this->ClientDataServerSocketController = nullptr;
  this->MPIMToNSocketConnection = nullptr;

  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->MoveMode = vtkMPIMoveData::PASS_THROUGH;
  // This tells which server/client this object is on.
  this->Server = -1;

  // This is set on the data server and render server when we are running
  // with a render server.
  this->MPIMToNSocketConnection = nullptr;

  this->NumberOfBuffers = 0;
  this->BufferLengths = nullptr;
  this->BufferOffsets = nullptr;
  this->Buffers = nullptr;
  this->BufferTotalLength = 9;

  this->OutputDataType = VTK_POLY_DATA;

  this->UpdateNumberOfPieces = 0;
  this->UpdatePiece = 0;

  this->SkipDataServerGatherToZero = false;
}

//-----------------------------------------------------------------------------
vtkMPIMoveData::~vtkMPIMoveData()
{
  this->SetController(nullptr);
  this->SetClientDataServerSocketController(nullptr);
  this->SetMPIMToNSocketConnection(nullptr);
  this->ClearBuffer();
}

//----------------------------------------------------------------------------
void vtkMPIMoveData::InitializeForCommunicationForParaView()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm == nullptr)
  {
    vtkWarningMacro("No process module found.");
    return;
  }

  vtkPVSession* session = vtkPVSession::SafeDownCast(pm->GetActiveSession());
  if (!session)
  {
    vtkWarningMacro("No active vtkPVSession found.");
    return;
  }

  int processRoles = session->GetProcessRoles();
  if (processRoles & vtkPVSession::RENDER_SERVER)
  {
    this->SetServerToRenderServer();
  }

  if (processRoles & vtkPVSession::DATA_SERVER)
  {
    this->SetServerToDataServer();
    this->SetClientDataServerSocketController(session->GetController(vtkPVSession::CLIENT));
  }

  if (processRoles & vtkPVSession::CLIENT)
  {
    this->SetServerToClient();
    this->SetClientDataServerSocketController(session->GetController(vtkPVSession::DATA_SERVER));
  }

  this->SetController(pm->GetGlobalController());
  this->SetMPIMToNSocketConnection(session->GetMPIMToNSocketConnection());
}

//----------------------------------------------------------------------------
void vtkMPIMoveData::SetUseZLibCompression(bool b)
{
  vtkMPIMoveData::UseZLibCompression = b;
}

//----------------------------------------------------------------------------
bool vtkMPIMoveData::GetUseZLibCompression()
{
  return vtkMPIMoveData::UseZLibCompression;
}

//----------------------------------------------------------------------------
int vtkMPIMoveData::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkMPIMoveData::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  auto output = vtkDataObject::GetData(outputVector, 0);
  if (output && output->GetDataObjectType() == this->OutputDataType)
  {
    return 1;
  }

  if (auto newoutput = vtkDataObjectTypes::NewDataObject(this->OutputDataType))
  {
    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newoutput);
    newoutput->FastDelete();
    return 1;
  }

  vtkErrorMacro("Unrecognized output type: " << this->OutputDataType << ". Cannot create output.");
  return 0;
}

//-----------------------------------------------------------------------------
bool vtkMPIMoveData::GetOutputGeneratedOnProcess()
{
  switch (this->Server)
  {
    case vtkMPIMoveData::RENDER_SERVER:
      // if this->Server is RENDER_SERVER, then we are in a true client-ds-rs
      // configuration. In that case, the data is valid only when movemode is
      // clone or pass-thru.
      return (this->MoveMode == PASS_THROUGH || this->MoveMode == CLONE ||
        this->MoveMode == COLLECT_AND_PASS_THROUGH);

    case vtkMPIMoveData::DATA_SERVER:
      // if this->Server is DATA_SERVER, we may be in cs or cdsrs modes.
      if (this->MPIMToNSocketConnection)
      {
        // definitely in render-server mode. This process never generates data.
        return false;
      }
      return (this->MoveMode == PASS_THROUGH || this->MoveMode == CLONE ||
        this->MoveMode == COLLECT_AND_PASS_THROUGH);

    case vtkMPIMoveData::CLIENT:
      if (this->ClientDataServerSocketController)
      {
        // client.
        return (this->MoveMode == COLLECT || this->MoveMode == CLONE ||
          this->MoveMode == COLLECT_AND_PASS_THROUGH);
      }
      // built-in mode; ofcourse we have data.
      return true;
  }

  vtkErrorMacro("Invalid setup. Is vtkMPIMoveData initialized yet?");
  return false;
}

//-----------------------------------------------------------------------------
// This filter  is going to replace the many variations of collection filters.
// It handles collection and duplication.
// It handles poly data and unstructured grid.
// It handles rendering on the data server and render server.

// Pass through, No render server. (Distributed rendering on data server).
// Data server copy input to output.

// Passthrough, Yes RenderServer (Distributed rendering on render server).
// Data server MtoN
// Move data from N data server processes to N render server processes.

// Duplicate, No render server. (Tile rendering on data server and client).
// GatherAll on data server.
// Data server process 0 sends data to client.

// Duplicate, Yes RenderServer (Tile rendering on rendering server and client).
// GatherToZero on data server.
// Data server process 0 sends to client
// Data server process 0 sends to render server 0
// Render server process 0 broad casts to all render server processes.

// Collect, render server: yes or no. (client rendering).
// GatherToZero on data server.
// Data server process 0 sends data to client.

//-----------------------------------------------------------------------------
// We should avoid marshalling more than once.
int vtkMPIMoveData::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = nullptr;
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    input = inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  }

  if (this->OutputDataType == VTK_IMAGE_DATA)
  {
    if (this->MoveMode == vtkMPIMoveData::PASS_THROUGH && this->MPIMToNSocketConnection)
    {
      vtkErrorMacro("Image data delivery to render server not supported.");
      return 0;
    }
  }

  this->UpdatePiece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  this->UpdateNumberOfPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // This case deals with everything running as one MPI group
  // Client, Data and render server are all the same program.
  // This covers single process mode too, although this filter
  // is unnecessary in that mode and really should not be put in.
  if (this->MPIMToNSocketConnection == nullptr && this->ClientDataServerSocketController == nullptr)
  {
    // Clone in this mode is used for plots and picking.
    if (this->MoveMode == vtkMPIMoveData::CLONE)
    {
      this->DataServerGatherAll(input, output);
      return 1;
    }
    // Collect mode for rendering on node 0.
    if (this->MoveMode == vtkMPIMoveData::COLLECT)
    {
      this->DataServerGatherToZero(input, output);
      return 1;
    }
    // PassThrough mode for compositing.
    if (this->MoveMode == vtkMPIMoveData::PASS_THROUGH)
    {
      vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "pass-through");
      output->ShallowCopy(input);
      return 1;
    }
    // Collect and PassThrough.
    if (this->MoveMode == vtkMPIMoveData::COLLECT_AND_PASS_THROUGH)
    {
      // Collect
      this->DataServerGatherToZero(input, output);
      // PassThrough
      vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "pass-through");
      output->ShallowCopy(input);
      return 1;
    }
    vtkErrorMacro("MoveMode not set.");
    return 0;
  }

  // PassThrough with no RenderServer. (Distributed rendering on data server).
  // Data server copy input to output.
  // Render server and client will not have an input.
  if (this->MoveMode == vtkMPIMoveData::PASS_THROUGH && this->MPIMToNSocketConnection == nullptr)
  {
    if (input)
    {
      vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "pass-through");
      output->ShallowCopy(input);
    }
    return 1;
  }

  // Passthrough and RenderServer (Distributed rendering on render server).
  // Data server MtoN
  // Move data from N data server processes to N render server processes.
  if (this->MoveMode == vtkMPIMoveData::PASS_THROUGH && this->MPIMToNSocketConnection)
  {
    if (this->Server == vtkMPIMoveData::DATA_SERVER)
    {
      this->DataServerAllToN(
        input, output, this->MPIMToNSocketConnection->GetNumberOfConnections());
      this->DataServerSendToRenderServer(output);
      output->Initialize();
      return 1;
    }
    if (this->Server == vtkMPIMoveData::RENDER_SERVER)
    {
      this->RenderServerReceiveFromDataServer(output);
      return 1;
    }
    // Client does nothing.
    return 1;
  }

  // Duplicate with no RenderServer.(Tile rendering on data server and client).
  // GatherAll on data server.
  // Data server process 0 sends data to client.
  if (this->MoveMode == vtkMPIMoveData::CLONE && this->MPIMToNSocketConnection == nullptr)
  {
    if (this->Server == vtkMPIMoveData::DATA_SERVER)
    {
      this->DataServerGatherAll(input, output);
      this->DataServerSendToClient(output);
      return 1;
    }
    if (this->Server == vtkMPIMoveData::CLIENT)
    {
      this->ClientReceiveFromDataServer(output);
      return 1;
    }
  }

  // Duplicate and RenderServer(Tile rendering on rendering server and client).
  // GatherToZero on data server.
  // Data server process 0 sends to client
  // Data server process 0 sends to render server 0
  // Render server process 0 broad casts to all render server processes.
  if (this->MoveMode == vtkMPIMoveData::CLONE && this->MPIMToNSocketConnection)
  {
    if (this->Server == vtkMPIMoveData::DATA_SERVER)
    {
      this->DataServerGatherToZero(input, output);
      this->DataServerSendToClient(output);
      this->DataServerZeroSendToRenderServerZero(output);
      return 1;
    }
    if (this->Server == vtkMPIMoveData::CLIENT)
    {
      this->ClientReceiveFromDataServer(output);
      return 1;
    }
    if (this->Server == vtkMPIMoveData::RENDER_SERVER)
    {
      this->RenderServerZeroReceiveFromDataServerZero(output);
      this->RenderServerZeroBroadcast(output);
    }
  }

  // Collect and data server or render server (client rendering).
  // GatherToZero on data server.
  // Data server process 0 sends data to client.
  if (this->MoveMode == vtkMPIMoveData::COLLECT)
  {
    if (this->Server == vtkMPIMoveData::DATA_SERVER)
    {
      this->DataServerGatherToZero(input, output);
      this->DataServerSendToClient(output);
      return 1;
    }
    if (this->Server == vtkMPIMoveData::CLIENT)
    {
      this->ClientReceiveFromDataServer(output);
      return 1;
    }
    // Render server does nothing
    return 1;
  }

  if (this->MoveMode == vtkMPIMoveData::COLLECT_AND_PASS_THROUGH)
  {
    if (this->MPIMToNSocketConnection == nullptr)
    {
      // In client-server mode without render server.
      if (this->Server == vtkMPIMoveData::DATA_SERVER)
      {
        vtkDataObject* tmp = input->NewInstance();
        this->DataServerGatherToZero(input, tmp);
        this->DataServerSendToClient(tmp);
        tmp->Delete();
        tmp = nullptr;
        vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "pass-through");
        output->ShallowCopy(input);
        return 1;
      }
      if (this->Server == vtkMPIMoveData::CLIENT)
      {
        this->ClientReceiveFromDataServer(output);
        return 1;
      }
    }
    else
    {
      // in client-dataserver-renderserver mode.
      if (this->Server == vtkMPIMoveData::DATA_SERVER)
      {
        // Pass Through
        this->DataServerAllToN(
          input, output, this->MPIMToNSocketConnection->GetNumberOfConnections());
        this->DataServerSendToRenderServer(output);
        output->Initialize();

        // Collect to client.
        this->DataServerGatherToZero(input, output);
        this->DataServerSendToClient(output);
        output->Initialize();
        return 1;
      }
      if (this->Server == vtkMPIMoveData::RENDER_SERVER)
      {
        this->RenderServerReceiveFromDataServer(output);
        return 1;
      }
      if (this->Server == vtkMPIMoveData::CLIENT)
      {
        this->ClientReceiveFromDataServer(output);
        return 1;
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
// Use LANL filter to redistribute the data.
// We will marshal more than once, but that is OK.
void vtkMPIMoveData::DataServerAllToN(vtkDataObject* input, vtkDataObject* output, int n)
{
  vtkMultiProcessController* controller = this->Controller;
  int m;

  if (controller == nullptr)
  {
    vtkErrorMacro("Missing controller.");
    return;
  }

  m = this->Controller->GetNumberOfProcesses();
  if (n > m)
  {
    vtkWarningMacro("Too many render servers.");
    n = m;
  }
  if (input == nullptr || output == nullptr)
  {
    vtkErrorMacro("All to N only works for poly data currently.");
    return;
  }

  if (n == m)
  {
    vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "pass-through");
    output->ShallowCopy(input);
    return;
  }

  // Perform the M to N operation.
  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "redistribute MxN (M=%d, N=%d)", m, n);
  vtkAllToNRedistributeCompositePolyData* AllToN = nullptr;
  AllToN = vtkAllToNRedistributeCompositePolyData::New();
  AllToN->SetController(controller);
  AllToN->SetNumberOfProcesses(n);
  AllToN->SetInputData(input);
  AllToN->Update();
  output->ShallowCopy(AllToN->GetOutputDataObject(0));
  AllToN->Delete();
  AllToN = nullptr;
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerGatherAll(vtkDataObject* input, vtkDataObject* output)
{
  int numProcs = this->Controller->GetNumberOfProcesses();

  if (numProcs <= 1)
  {
    if (input)
    {
      vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "pass-through");
      output->ShallowCopy(input);
    }
    return;
  }

  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "gather-all");

  int idx;
  auto com = this->Controller->GetCommunicator();
  if (com == nullptr)
  {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
  }
  this->ClearBuffer();
  this->MarshalDataToBuffer(input);

  // Save a copy of the buffer so we can receive into the buffer.
  // We will be responsiblefor deleting the buffer.
  // This assumes one buffer. MashalData will produce only one buffer
  // One data set, one buffer.
  vtkIdType inBufferLength = this->BufferTotalLength;
  char* inBuffer = this->Buffers;
  this->Buffers = nullptr;
  this->ClearBuffer();

  // Allocate arrays used by the AllGatherV call.
  this->BufferLengths = new vtkIdType[numProcs];
  this->BufferOffsets = new vtkIdType[numProcs];

  // Compute the degenerate input offsets and lengths.
  // Broadcast our size to all other processes.
  com->AllGather(&inBufferLength, this->BufferLengths, 1);

  // Compute the displacements.
  this->BufferTotalLength = 0;
  for (idx = 0; idx < numProcs; ++idx)
  {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
  }
  // Gather the marshaled data sets from all procs.
  this->NumberOfBuffers = numProcs;
  this->Buffers = new char[this->BufferTotalLength];
  com->AllGatherV(
    inBuffer, this->Buffers, inBufferLength, this->BufferLengths, this->BufferOffsets);

  this->ReconstructDataFromBuffer(output);

  // int fixme; // Do not clear buffers here
  this->ClearBuffer();
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerGatherToZero(vtkDataObject* input, vtkDataObject* output)
{
  int numProcs = this->Controller->GetNumberOfProcesses();
  if (numProcs == 1)
  {
    if (input)
    {
      vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "pass-through");
      output->ShallowCopy(input);
    }
    return;
  }
  if (this->SkipDataServerGatherToZero)
  {
    if (this->Controller->GetLocalProcessId() == 0 && input)
    {
      vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "pass-through");
      output->ShallowCopy(input);
    }
    return;
  }

  vtkTimerLog::MarkStartEvent("Dataserver gathering to 0");

  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "gather-to-0");
  int idx;
  int myId = this->Controller->GetLocalProcessId();
  auto com = this->Controller->GetCommunicator();
  if (com == nullptr)
  {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
  }
  this->ClearBuffer();
  this->MarshalDataToBuffer(input);

  // Save a copy of the buffer so we can receive into the buffer.
  // We will be responsiblefor deleting the buffer.
  // This assumes one buffer. MashalData will produce only one buffer
  // One data set, one buffer.
  vtkIdType inBufferLength = this->BufferTotalLength;
  char* inBuffer = this->Buffers;
  this->Buffers = nullptr;
  this->ClearBuffer();

  // Allocate arrays used by the AllGatherV call.
  this->BufferLengths = new vtkIdType[numProcs];
  this->BufferOffsets = new vtkIdType[numProcs];

  // Compute the degenerate input offsets and lengths.
  // Broadcast our size to all processes.
  // Note: this has to be done as the GatherV function
  // needs to know the offsets and lengths for all processes.
  com->AllGather(&inBufferLength, this->BufferLengths, 1);

  // Compute the displacements.
  this->BufferTotalLength = 0;
  for (idx = 0; idx < numProcs; ++idx)
  {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
  }
  // Gather the marshaled data sets to 0.
  this->Buffers = new char[this->BufferTotalLength];
  com->GatherV(
    inBuffer, this->Buffers, inBufferLength, this->BufferLengths, this->BufferOffsets, 0);
  this->NumberOfBuffers = numProcs;

  if (myId == 0)
  {
    this->ReconstructDataFromBuffer(output);
  }

  // int fixme; // Do not clear buffers here
  this->ClearBuffer();

  delete[] inBuffer;
  inBuffer = nullptr;

  vtkTimerLog::MarkEndEvent("Dataserver gathering to 0");
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerSendToRenderServer(vtkDataObject* output)
{
  vtkSocketCommunicator* com = this->MPIMToNSocketConnection->GetSocketCommunicator();

  if (com == nullptr)
  {
    // Some data server may not have sockets because there are more data
    // processes than render server processes.
    return;
  }

  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "send-to-renderserver");

  // int fixme;
  // We might be able to eliminate this marshal.
  this->ClearBuffer();
  this->MarshalDataToBuffer(output);

  com->Send(&(this->NumberOfBuffers), 1, 1, 23480);
  com->Send(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
  com->Send(this->Buffers, this->BufferTotalLength, 1, 23482);
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::RenderServerReceiveFromDataServer(vtkDataObject* output)
{
  vtkSocketCommunicator* com = this->MPIMToNSocketConnection->GetSocketCommunicator();

  if (com == nullptr)
  {
    vtkErrorMacro("All render server processes should have sockets.");
    return;
  }

  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "receive-from-dataserver");

  this->ClearBuffer();
  com->Receive(&(this->NumberOfBuffers), 1, 1, 23480);
  this->BufferLengths = new vtkIdType[this->NumberOfBuffers];
  com->Receive(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
  // Compute additional buffer information.
  this->BufferOffsets = new vtkIdType[this->NumberOfBuffers];
  this->BufferTotalLength = 0;
  for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
  {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
  }
  this->Buffers = new char[this->BufferTotalLength];
  com->Receive(this->Buffers, this->BufferTotalLength, 1, 23482);

  // int fixme;  // Can we avoid this?
  this->ReconstructDataFromBuffer(output);
  this->ClearBuffer();
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerZeroSendToRenderServerZero(vtkDataObject* data)
{
  int myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
  {
    vtkSocketCommunicator* com = this->MPIMToNSocketConnection->GetSocketCommunicator();

    if (com == nullptr)
    {
      // Proc 0 (at least) should have a communicator.
      vtkErrorMacro("Missing socket connection.");
      return;
    }

    vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "send-to-renderserver-root");

    // int fixme;
    // We might be able to eliminate this marshal.
    this->ClearBuffer();
    this->MarshalDataToBuffer(data);
    com->Send(&(this->NumberOfBuffers), 1, 1, 23480);
    com->Send(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
    com->Send(this->Buffers, this->BufferTotalLength, 1, 23482);
    this->ClearBuffer();
  }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::RenderServerZeroReceiveFromDataServerZero(vtkDataObject* data)
{
  int myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
  {
    vtkSocketCommunicator* com = this->MPIMToNSocketConnection->GetSocketCommunicator();

    if (com == nullptr)
    {
      vtkErrorMacro("All render server processes should have sockets.");
      return;
    }

    vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "receive-from-dataserver-root");

    this->ClearBuffer();
    com->Receive(&(this->NumberOfBuffers), 1, 1, 23480);
    this->BufferLengths = new vtkIdType[this->NumberOfBuffers];
    com->Receive(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
    // Compute additional buffer information.
    this->BufferOffsets = new vtkIdType[this->NumberOfBuffers];
    this->BufferTotalLength = 0;
    for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
    {
      this->BufferOffsets[idx] = this->BufferTotalLength;
      this->BufferTotalLength += this->BufferLengths[idx];
    }
    this->Buffers = new char[this->BufferTotalLength];
    com->Receive(this->Buffers, this->BufferTotalLength, 1, 23482);

    // int fixme;  // Can we avoid this?
    this->ReconstructDataFromBuffer(data);
    this->ClearBuffer();
  }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerSendToClient(vtkDataObject* output)
{
  if (this->ClientDataServerSocketController == nullptr)
  {
    return;
  }

  int myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
  {
    vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "send-to-client");
    vtkTimerLog::MarkStartEvent("Dataserver sending to client");
    this->ClearBuffer();
    this->MarshalDataToBuffer(output);
    this->ClientDataServerSocketController->Send(&(this->NumberOfBuffers), 1, 1, 23490);
    this->ClientDataServerSocketController->Send(
      this->BufferLengths, this->NumberOfBuffers, 1, 23491);
    this->ClientDataServerSocketController->Send(this->Buffers, this->BufferTotalLength, 1, 23492);
    this->ClearBuffer();
    vtkTimerLog::MarkEndEvent("Dataserver sending to client");
  }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::ClientReceiveFromDataServer(vtkDataObject* output)
{
  vtkCommunicator* com = nullptr;
  com = this->ClientDataServerSocketController->GetCommunicator();
  if (com == nullptr)
  {
    vtkErrorMacro("Missing socket controller on client.");
    return;
  }

  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "receive-from-dataserver");

  this->ClearBuffer();
  com->Receive(&(this->NumberOfBuffers), 1, 1, 23490);
  this->BufferLengths = new vtkIdType[this->NumberOfBuffers];
  com->Receive(this->BufferLengths, this->NumberOfBuffers, 1, 23491);
  // Compute additional buffer information.
  this->BufferOffsets = new vtkIdType[this->NumberOfBuffers];
  this->BufferTotalLength = 0;
  for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
  {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
  }
  this->Buffers = new char[this->BufferTotalLength];
  com->Receive(this->Buffers, this->BufferTotalLength, 1, 23492);
  this->ReconstructDataFromBuffer(output);
  this->ClearBuffer();
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::RenderServerZeroBroadcast(vtkDataObject* data)
{
  (void)data; // shut up warning
  int numProcs = this->Controller->GetNumberOfProcesses();
  if (numProcs <= 1)
  {
    return;
  }

  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "broadcast");
  int myId = this->Controller->GetLocalProcessId();

  auto com = this->Controller->GetCommunicator();
  if (com == nullptr)
  {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
  }

  int bufferLength = 0;
  if (myId == 0)
  {
    this->ClearBuffer();
    this->MarshalDataToBuffer(data);
    bufferLength = this->BufferLengths[0];
  }

  // Broadcast the size of the buffer.
  com->Broadcast(&bufferLength, 1, 0);

  // Allocate buffers for all receiving nodes.
  if (myId != 0)
  {
    this->NumberOfBuffers = 1;
    this->BufferLengths = new vtkIdType[1];
    this->BufferLengths[0] = bufferLength;
    this->BufferOffsets = new vtkIdType[1];
    this->BufferOffsets[0] = 0;
    this->BufferTotalLength = this->BufferLengths[0];
    this->Buffers = new char[bufferLength];
  }

  // Broadcast the buffer.
  com->Broadcast(this->Buffers, bufferLength, 0);

  // Reconstruct the output on nodes other than 0.
  if (myId != 0)
  {
    this->ReconstructDataFromBuffer(data);
  }

  this->ClearBuffer();
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::ClearBuffer()
{
  this->NumberOfBuffers = 0;
  if (this->BufferLengths)
  {
    delete[] this->BufferLengths;
    this->BufferLengths = nullptr;
  }
  if (this->BufferOffsets)
  {
    delete[] this->BufferOffsets;
    this->BufferOffsets = nullptr;
  }
  if (this->Buffers)
  {
    delete[] this->Buffers;
    this->Buffers = nullptr;
  }
  this->BufferTotalLength = 0;
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::MarshalDataToBuffer(vtkDataObject* data)
{
  vtkImageData* imageData = vtkImageData::SafeDownCast(data);

  // Protect from empty data.
  if (data->GetNumberOfElements(vtkDataObject::POINT) == 0 &&
    data->GetNumberOfElements(vtkDataObject::VERTEX) == 0)
  {
    this->NumberOfBuffers = 0;
  }

  // Copy input to isolate reader from the pipeline.
  vtkDataWriter* writer = vtkGenericDataObjectWriter::New();
  writer->SetInputData(data);
  if (imageData)
  {
    // We add the image extents to the header, since the writer doesn't preserve
    // the extents.
    int* extent = imageData->GetExtent();
    double* origin = imageData->GetOrigin();
    std::ostringstream stream;
    stream << "EXTENT " << extent[0] << " " << extent[1] << " " << extent[2] << " " << extent[3]
           << " " << extent[4] << " " << extent[5];
    stream << " ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2];
    writer->SetHeader(stream.str().c_str());
  }

  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();
  writer->Write();

  char* buffer = nullptr;
  vtkIdType buffer_length = 0;

  if (vtkMPIMoveData::UseZLibCompression)
  {
    vtkTimerLog::MarkStartEvent("Zlib compress");
    // Use z-lib compression.
    uLongf out_size = compressBound(writer->GetOutputStringLength());
    buffer = new char[out_size + 8];
    memcpy(buffer, "zlib0000", 8);

    compress2(reinterpret_cast<Bytef*>(buffer + 8), &out_size,
      reinterpret_cast<const Bytef*>(writer->GetOutputString()), writer->GetOutputStringLength(),
      /* compression_level */ Z_DEFAULT_COMPRESSION);
    vtkTimerLog::MarkEndEvent("Zlib compress");
    int in_size = static_cast<int>(writer->GetOutputStringLength());
    for (int cc = 0; cc < 4; cc++)
    {
      // the first 4 bytes in the header are "zlib" which helps the receiver
      // identify that zlib compression has been used.
      // the next 4 bytes are the original length since zlib doesn't provide
      // that to the receiver.
      buffer[4 + cc] = (in_size & 0x0ff);
      in_size = in_size >> 8;
    }
    buffer_length = out_size + 8;
  }
  else
  {
    buffer_length = writer->GetOutputStringLength();
    buffer = writer->RegisterAndGetOutputString();
  }

  // Get string.
  this->NumberOfBuffers = 1;
  this->BufferLengths = new vtkIdType[1];
  this->BufferLengths[0] = buffer_length;
  this->BufferOffsets = new vtkIdType[1];
  this->BufferOffsets[0] = 0;
  this->Buffers = buffer;
  this->BufferTotalLength = this->BufferLengths[0];

  writer->Delete();
  writer = nullptr;
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::ReconstructDataFromBuffer(vtkDataObject* data)
{
  if (this->NumberOfBuffers == 0 || this->Buffers == nullptr)
  {
    data->Initialize();
    return;
  }

  bool is_image_data = data->IsA("vtkImageData") != 0;
  std::vector<vtkSmartPointer<vtkDataObject>> pieces;

  for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
  {
    char* bufferArray = this->Buffers + this->BufferOffsets[idx];
    vtkIdType bufferLength = this->BufferLengths[idx];

    char* realBuffer = nullptr;
    if (bufferLength > 4 && strncmp(bufferArray, "zlib", 4) == 0)
    {
      // sender used zlib compression. Decompress it.
      vtkIdType compressed_length = bufferLength - 8; // remove the zlib header.
      vtkIdType uncompressed_length = 0;
      for (int cc = 0; cc < 4; cc++)
      {
        uncompressed_length = uncompressed_length | ((0xff & (bufferArray[4 + cc])) << 8 * cc);
      }

      // using zlib compression.
      realBuffer = new char[uncompressed_length];
      uLongf destLen = uncompressed_length;
      vtkTimerLog::MarkStartEvent("Zlib uncompress");
      uncompress(reinterpret_cast<Bytef*>(realBuffer), &destLen,
        reinterpret_cast<const Bytef*>(bufferArray + 8), compressed_length);
      vtkTimerLog::MarkEndEvent("Zlib uncompress");

      bufferArray = realBuffer;
      bufferLength = uncompressed_length;
    }

    // Setup a reader.
    vtkDataReader* reader = vtkGenericDataObjectReader::New();
    reader->ReadFromInputStringOn();

    vtkCharArray* mystring = vtkCharArray::New();
    mystring->SetArray(bufferArray, bufferLength, 1);
    reader->SetInputArray(mystring);
    reader->Modified(); // For append loop
    reader->Update();

    if (is_image_data)
    {
      // FIXME: EXTENT and ORIGIN in vtkImageData are lost by reader/writer.
      // The header hack we used isn't going to work for composite datasets. We
      // need a more intrusive fix in the reader/writer itself.
      int extent[6] = { 0, 0, 0, 0, 0, 0 };
      float origin[3] = { 0, 0, 0 };
      int values_read = sscanf(reader->GetHeader(), "EXTENT %d %d %d %d %d %d ORIGIN %f %f %f",
        &extent[0], &extent[1], &extent[2], &extent[3], &extent[4], &extent[5], &origin[0],
        &origin[1], &origin[2]);
      if (values_read != 9)
      {
        vtkWarningMacro("EXTENT and ORIGIN may not have been read correctly.");
      }
      vtkImageData* clone =
        vtkImageData::SafeDownCast(reader->GetOutputDataObject(0)->NewInstance());
      clone->ShallowCopy(reader->GetOutputDataObject(0));
      clone->SetOrigin(origin[0], origin[1], origin[2]);
      clone->SetExtent(extent);
      // reconstructing data distributted on MPI node, so global ids are valid
      // global ids attributes are removed when appending data so we set
      // the active global ids attribute to nullptr which keeps the global ids array.
      unsetGlobalIdsAttribute(clone);
      pieces.push_back(clone);
      clone->Delete();
    }
    else
    {
      vtkDataObject* output = reader->GetOutputDataObject(0);
      // reconstructing data distributted on MPI node, so global ids are valid
      unsetGlobalIdsAttribute(output);
      pieces.push_back(output);
    }
    mystring->Delete();
    mystring = nullptr;
    reader->Delete();
    reader = nullptr;
    delete[] realBuffer;
    realBuffer = nullptr;
  }

  vtkMPIMoveDataMerge(pieces, data);
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfBuffers: " << this->NumberOfBuffers << endl;
  os << indent << "Server: " << this->Server << endl;
  os << indent << "MoveMode: " << this->MoveMode << endl;
  os << indent << "SkipDataServerGatherToZero: " << this->SkipDataServerGatherToZero << endl;
  os << indent << "OutputDataType: ";
  if (this->OutputDataType == VTK_POLY_DATA)
  {
    os << "VTK_POLY_DATA";
  }
  else if (this->OutputDataType == VTK_UNSTRUCTURED_GRID)
  {
    os << "VTK_UNSTRUCTURED_GRID";
  }
  else if (this->OutputDataType == VTK_IMAGE_DATA)
  {
    os << "VTK_IMAGE_DATA";
  }
  else if (this->OutputDataType == VTK_DIRECTED_GRAPH)
  {
    os << "VTK_DIRECTED_GRAPH";
  }
  else if (this->OutputDataType == VTK_UNDIRECTED_GRAPH)
  {
    os << "VTK_UNDIRECTED_GRAPH";
  }
  else
  {
    os << "Unrecognized output type " << this->OutputDataType;
  }
  os << endl;
  // os << indent << "MToN
}
