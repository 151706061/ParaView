// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogRecorder.h"

#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"

vtkStandardNewMacro(vtkLogRecorder);

//----------------------------------------------------------------------------
vtkLogRecorder::vtkLogRecorder()
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  this->MyRank = controller->GetLocalProcessId();
  this->Verbosity = vtkLogger::VERBOSITY_INVALID;
}

//----------------------------------------------------------------------------
vtkLogRecorder::~vtkLogRecorder()
{
  if (this->RankEnabled)
  {
    this->DisableLoggingCallback();
  }
}

//----------------------------------------------------------------------------
void vtkLogRecorder::SetVerbosity(int verbosity)
{
  if (verbosity == this->Verbosity)
  {
    return;
  }

  this->Verbosity = verbosity;

  // Tear down and rebuild the logging callback.
  if (!this->CallbackName.empty())
  {
    this->DisableLoggingCallback();
  }
  this->CallbackName.clear();
  if (this->RankEnabled)
  {
    this->EnableLoggingCallback();
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLogRecorder::EnableLoggingCallback()
{
  if (!this->CallbackName.empty())
  {
    vtkErrorMacro("Logging callback " << this->CallbackName << " already added.");
  }
  this->CallbackName = "log-grabber_" + std::to_string(RankEnabled) + "_" + std::to_string(rand());
  vtkLogger::AddCallback(
    this->CallbackName.c_str(),
    [](void* user_data, const vtkLogger::Message& message)
    {
      auto lines = reinterpret_cast<std::string*>(user_data);
      (*lines) += "\n";
      (*lines) += message.preamble;
      (*lines) += message.indentation;
      (*lines) += message.prefix;
      (*lines) += message.message;
    },
    &this->Logs, static_cast<vtkLogger::Verbosity>(this->Verbosity));
}

//----------------------------------------------------------------------------
void vtkLogRecorder::DisableLoggingCallback()
{
  if (this->CallbackName.empty())
  {
    vtkErrorMacro("Callback already disabled.");
  }
  vtkLogger::RemoveCallback(this->CallbackName.c_str());
  this->CallbackName.clear();
}

//----------------------------------------------------------------------------
void vtkLogRecorder::SetRankEnabled(int rank)
{
  if (this->MyRank == rank)
  {
    if (!this->RankEnabled)
    {
      this->EnableLoggingCallback();
    }
    this->RankEnabled++;
  }
}

//----------------------------------------------------------------------------
void vtkLogRecorder::SetRankDisabled(int rank)
{
  if (this->RankEnabled)
  {
    if (rank == this->MyRank)
    {
      this->RankEnabled--;
      if (!this->RankEnabled)
      {
        this->DisableLoggingCallback();
      }
    }
  }
}

//----------------------------------------------------------------------------
const std::string& vtkLogRecorder::GetLogs() const
{
  return this->Logs;
}

//----------------------------------------------------------------------------
void vtkLogRecorder::ClearLogs()
{
  this->Logs.clear();
}

//----------------------------------------------------------------------------
void vtkLogRecorder::SetCategoryVerbosity(int categoryIndex, int verbosity)
{
  vtkLogger::Verbosity v = static_cast<vtkLogger::Verbosity>(verbosity);
  switch (categoryIndex)
  {
    case 0:
      vtkPVLogger::SetDataMovementVerbosity(v);
      break;
    case 1:
      vtkPVLogger::SetRenderingVerbosity(v);
      break;
    case 2:
      vtkPVLogger::SetApplicationVerbosity(v);
      break;
    case 3:
      vtkPVLogger::SetPipelineVerbosity(v);
      break;
    case 4:
      vtkPVLogger::SetPluginVerbosity(v);
      break;
    case 5:
      vtkPVLogger::SetExecutionVerbosity(v);
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtkLogRecorder::ResetCategoryVerbosities()
{
  auto verbosity = vtkPVLogger::GetDefaultVerbosity();
  vtkPVLogger::SetDataMovementVerbosity(verbosity);
  vtkPVLogger::SetRenderingVerbosity(verbosity);
  vtkPVLogger::SetApplicationVerbosity(verbosity);
  vtkPVLogger::SetPipelineVerbosity(verbosity);
  vtkPVLogger::SetPluginVerbosity(verbosity);
  vtkPVLogger::SetExecutionVerbosity(verbosity);
}

//----------------------------------------------------------------------------
void vtkLogRecorder::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Rank Location: " << this->MyRank << endl;
  os << indent << "Activation Count: " << this->RankEnabled << endl;
  os << indent << "Callback Name: " << this->CallbackName << endl;
  os << indent << "Log Buffer: " << this->Logs << endl;
}
