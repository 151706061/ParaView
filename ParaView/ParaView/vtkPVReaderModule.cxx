/*=========================================================================

  Program:   ParaView
  Module:    vtkPVReaderModule.cxx
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
#include "vtkPVReaderModule.h"
#include "vtkPVApplication.h"
#include "vtkObjectFactory.h"
#include "vtkPVFileEntry.h"
#include "vtkKWFrame.h"
#include "vtkPVWindow.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVReaderModule);
vtkCxxRevisionMacro(vtkPVReaderModule, "1.13.2.1");

int vtkPVReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVReaderModule::vtkPVReaderModule()
{
  this->CommandFunction = vtkPVReaderModuleCommand;
  this->FileEntry = 0;
  this->AcceptAfterRead = 1;
  this->Extensions = vtkVector<const char*>::New();
  this->Iterator = this->Extensions->NewIterator();
  this->PackFileEntry = 1;
  this->AddFileEntry = 1;
}

//----------------------------------------------------------------------------
vtkPVReaderModule::~vtkPVReaderModule()
{
  if (this->FileEntry)
    {
    this->FileEntry->Delete();
    }
  this->Extensions->Delete();
  this->Iterator->Delete();
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::CreateProperties()
{
  this->Superclass::CreateProperties();

  this->FileEntry = vtkPVFileEntry::New();
  this->FileEntry->SetLabel("Filename");
  this->FileEntry->SetPVSource(this);
  this->FileEntry->SetParent(this->GetParameterFrame()->GetFrame());
  this->FileEntry->SetModifiedCommand(this->GetTclName(), 
                                      "SetAcceptButtonColorToRed");
  this->FileEntry->SetObjectVariable(this->VTKSourceTclName, "FileName");
  this->FileEntry->Create(this->GetPVApplication());

  if (this->AddFileEntry)
    {
    this->AddPVWidget(this->FileEntry);
    }
  
  if (this->PackFileEntry)
    {
    this->Script("pack %s", this->FileEntry->GetWidgetName());
    }

}

//----------------------------------------------------------------------------
int vtkPVReaderModule::ClonePrototype(int makeCurrent, 
                                      vtkPVReaderModule*& clone )
{
  clone = 0;

  vtkPVSource* pvs = 0;
  int retVal = this->ClonePrototypeInternal(makeCurrent, pvs);
  if (retVal == VTK_OK)
    {
    clone = vtkPVReaderModule::SafeDownCast(pvs);
    }
  return retVal;
}

//----------------------------------------------------------------------------
int vtkPVReaderModule::CanReadFile(const char* fname)
{
  const char* ext = this->ExtractExtension(fname);
  const char* val = 0;
  this->Iterator->GoToFirstItem();
  while(!this->Iterator->IsDoneWithTraversal())
    {
    this->Iterator->GetData(val);
    if (ext && strcmp(ext, val) == 0)
      {
//        // The extension matches, see if the reader can read the file.
//        this->Script("%s vtkPVReaderModuleCanReadFileTemp",
//                     this->SourceClassName);
//        this->Script("vtkPVReaderModuleCanReadFileTemp CanReadFile {%s}",
//                     fname);
//        int result = this->GetIntegerResult(this->Application);
//        this->Script("vtkPVReaderModuleCanReadFileTemp Delete");
//        if(result)
//          {
      return 1;
//        }
      }
    this->Iterator->GoToNextItem();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::AddExtension(const char* ext)
{
  this->Extensions->AppendItem(ext);
}

//----------------------------------------------------------------------------
const char* vtkPVReaderModule::RemovePath(const char* fname)
{
  char* ptr = strrchr(fname, '/');
  if ( ptr )
    {
    if ( ptr[1] != '\0' )
      {
      return ptr+1;
      }
    else
      {
      return ptr;
      }
    }
  return  0;
}

//----------------------------------------------------------------------------
const char* vtkPVReaderModule::ExtractExtension(const char* fname)
{
  return strrchr(fname, '.');
}

//----------------------------------------------------------------------------
int vtkPVReaderModule::ReadFile(const char* fname, vtkPVReaderModule*& clone)
{
  clone = 0;
  if (this->ClonePrototype(1, clone) != VTK_OK)
    {
    vtkErrorMacro("Error creating reader " << this->GetClassName()
                  << endl);
    clone = 0;
    return VTK_ERROR;
    }
  this->Script("%s Set%s %s", clone->GetVTKSourceTclName(), 
               clone->FileEntry->GetVariableName(), fname);

  const char* ext = this->ExtractExtension(fname);
  if (ext)
    {
    clone->FileEntry->SetExtension(ext+1);
    }
  clone->UpdateParameterWidgets();

  if (clone)
    {
    const char* desc = this->RemovePath(fname);
    if (desc)
      {
      clone->SetDescriptionNoTrace(desc);
      }
    if (clone->GetTraceInitialized() == 0)
      { 
      vtkPVApplication* pvApp=this->GetPVApplication();
      pvApp->AddTraceEntry("set kw(%s) [$kw(%s) GetCurrentPVSource]", 
                           clone->GetTclName(), 
                           pvApp->GetMainWindow()->GetTclName());
      clone->SetTraceInitialized(1);
      }
    }

  return VTK_OK;
}

//----------------------------------------------------------------------------
vtkIdType vtkPVReaderModule::GetNumberOfExtensions()
{
  return this->Extensions->GetNumberOfItems();
}

//----------------------------------------------------------------------------
const char* vtkPVReaderModule::GetExtension(vtkIdType i)
{
  const char* result = 0;
  if(this->Extensions->GetItem(i, result) != VTK_OK) { result = 0; }
  return result;
}


//----------------------------------------------------------------------------
void vtkPVReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "AcceptAfterRead: " << this->AcceptAfterRead << endl;
  os << "PackFileEntry: " << this->PackFileEntry << endl;
}
