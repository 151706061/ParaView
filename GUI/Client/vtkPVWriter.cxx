/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVWriter.h"

#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVWriter);
vtkCxxRevisionMacro(vtkPVWriter, "1.16");

//----------------------------------------------------------------------------
vtkPVWriter::vtkPVWriter()
{
  this->InputClassName = 0;
  this->WriterClassName = 0;
  this->Description = 0;
  this->Extension = 0;
  this->Parallel = 0;
  this->DataModeMethod = 0;
}

//----------------------------------------------------------------------------
vtkPVWriter::~vtkPVWriter()
{
  this->SetInputClassName(0);
  this->SetWriterClassName(0);
  this->SetDescription(0);
  this->SetExtension(0);
  this->SetDataModeMethod(0);
}

//----------------------------------------------------------------------------
void vtkPVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputClassName: " 
     << (this->InputClassName?this->InputClassName:"(none)") << endl;
  os << indent << "WriterClassName: " 
     << (this->WriterClassName?this->WriterClassName:"(none)") << endl;
  os << indent << "Description: " 
     << (this->Description?this->Description:"(none)") << endl;
  os << indent << "Extension: " 
     << (this->Extension?this->Extension:"(none)") << endl;
  os << indent << "Parallel: " << this->Parallel << endl;
  os << indent << "DataModeMethod: " 
     << (this->DataModeMethod?this->DataModeMethod:"(none)") << endl;
}

//----------------------------------------------------------------------------
int vtkPVWriter::CanWriteData(vtkDataSet* data, int parallel, int numParts)
{
  if (data == NULL)
    {
    return 0;
    }
  return ((numParts == 1) &&
          (parallel == this->Parallel) &&
          data->IsA(this->InputClassName));
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVWriter::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
void vtkPVWriter::Write(const char* fileName, vtkPVSource* pvs,
                        int numProcs, int ghostLevel, int timeSeries)
{
  vtkPVReaderModule* rm = vtkPVReaderModule::SafeDownCast(pvs);
  if(rm && timeSeries)
    {
    vtkstd::string name = fileName;
    vtkstd::string::size_type pos = name.find_last_of(".");
    vtkstd::string base = name.substr(0, pos);
    vtkstd::string ext = name.substr(pos);
    int n = rm->GetNumberOfTimeSteps();
    char buf[100];
    for(int i=0; i < n; ++i)
      {
      sprintf(buf, "T%03d", i);
      name = base;
      name += buf;
      name += ext;
      rm->SetRequestedTimeStep(i);
      if (!this->WriteOneFile(name.c_str(), pvs, numProcs, ghostLevel))
        {
        return;
        }
      }
    }
  else
    {
    this->WriteOneFile(fileName, pvs, numProcs, ghostLevel);
    }
}

//----------------------------------------------------------------------------
int vtkPVWriter::WriteOneFile(const char* fileName, vtkPVSource* pvs,
                              int numProcs, int ghostLevel)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerID dataID = pvs->GetPart()->GetVTKDataID();
  int success = 1;

  // Create the writer and configure it.
  vtkClientServerID writerID = pm->NewStreamObject(this->WriterClassName);
  pm->GetStream() << vtkClientServerStream::Invoke
                  << writerID << "SetFileName" << fileName
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << writerID << "SetInput" << dataID
                  << vtkClientServerStream::End;
  if (this->DataModeMethod)
    {
    pm->GetStream() << vtkClientServerStream::Invoke
                    << writerID << this->DataModeMethod
                    << vtkClientServerStream::End;
    }

  if(this->Parallel)
    {
    pm->GetStream() << vtkClientServerStream::Invoke
                    << writerID << "SetNumberOfPieces" << numProcs
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke
                    << writerID << "SetGhostLevel" << ghostLevel
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke
                    << pm->GetProcessModuleID() << "GetPartitionId"
                    << vtkClientServerStream::End
                    << vtkClientServerStream::Invoke
                    << writerID << "SetStartPiece"
                    << vtkClientServerStream::LastResult
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke
                    << pm->GetProcessModuleID() << "GetPartitionId"
                    << vtkClientServerStream::End
                    << vtkClientServerStream::Invoke
                    << writerID << "SetEndPiece"
                    << vtkClientServerStream::LastResult
                    << vtkClientServerStream::End;

    // Tell each process's writer whether it should write the summary
    // file.  This assumes that the writer is a vtkXMLWriter.  When we
    // add more writers, we will need a separate writer module.
    vtkClientServerID helperID = pm->NewStreamObject("vtkPVSummaryHelper");
    pm->GetStream() << vtkClientServerStream::Invoke
                    << helperID << "SetWriter" << writerID
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke
                    << pm->GetProcessModuleID() << "GetController"
                    << vtkClientServerStream::End
                    << vtkClientServerStream::Invoke
                    << helperID << "SetController"
                    << vtkClientServerStream::LastResult
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke
                    << helperID << "SynchronizeSummaryFiles"
                    << vtkClientServerStream::End;
    pm->DeleteStreamObject(helperID);
    }

  // Write the data.
  pm->GetStream() << vtkClientServerStream::Invoke
                  << writerID << "Write"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << writerID << "GetErrorCode"
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER);
  int retVal;
  if(pm->GetLastServerResult().GetArgument(0, 0, &retVal) &&
     retVal == vtkErrorCode::OutOfDiskSpaceError)
    {
    vtkKWMessageDialog::PopupMessage(
      pvApp, pvApp->GetMainWindow(),
      "Write Error", "There is insufficient disk space to save this data. "
      "The file(s) already written will be deleted.");
    success = 0;
    }

  // Cleanup.
  pm->DeleteStreamObject(writerID);
  pm->SendStream(vtkProcessModule::DATA_SERVER);
  return success;
}
