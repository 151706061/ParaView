/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightReaderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVEnSightReaderModule.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVFileEntry.h"
#include "vtkPVProcessModule.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnSightReaderModule);
vtkCxxRevisionMacro(vtkPVEnSightReaderModule, "1.51");

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::vtkPVEnSightReaderModule()
{
  this->AddFileEntry = 1;
  this->PackFileEntry = 0;
  this->UpdateSourceInBatch = 1;
}

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::~vtkPVEnSightReaderModule()
{
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::CreateProperties()
{
  this->Superclass::CreateProperties();
  this->FileEntry->SetVariableName("CaseFileName");
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::InitializeData()
{
  int numSources = this->GetNumberOfVTKSources();
  int i;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  for(i = 0; i < numSources; ++i)
    {
    pm->GetStream() << vtkClientServerStream::Invoke <<  this->GetVTKSourceID(i)
                    << "Update" 
                    << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER);
  return this->Superclass::InitializeData();
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::SaveInBatchScript(ofstream *file)
{
  if (this->VisitedFlag)
    {
    return;
    }

  this->SaveFilterInBatchScript(file);
  *file << "  $pvTemp" <<  this->GetVTKSourceID(0)
        << " Update" 
        << endl;
  // Add the mapper, actor, scalar bar actor ...
  this->GetPVOutput()->SaveInBatchScript(file);
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::ReadFileInformation(const char* fname)
{
  // If this is a vtkPVEnSightMasterServerReader, set the controller.
  if(strcmp(this->SourceClassName, "vtkPVEnSightMasterServerReader") == 0)
    {
    int i;
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    int numSources = this->GetNumberOfVTKSources();
    for(i=0; i < numSources; ++i)
      {
      pm->GetStream() << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
                      << "GetController"
                      << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke << this->GetVTKSourceID(i) 
                      << "SetController"
                      << vtkClientServerStream::LastResult
                      << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER);
      }
    }
  return this->Superclass::ReadFileInformation(fname);
}
