/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAdvancedReaderModule.cxx
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
#include "vtkPVAdvancedReaderModule.h"

#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVPart.h"
#include "vtkPVFileEntry.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidgetCollection.h"
#include "vtkString.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAdvancedReaderModule);
vtkCxxRevisionMacro(vtkPVAdvancedReaderModule, "1.13");

int vtkPVAdvancedReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVAdvancedReaderModule::vtkPVAdvancedReaderModule()
{
  this->CommandFunction = vtkPVAdvancedReaderModuleCommand;
  this->AcceptAfterRead = 0;
}

//----------------------------------------------------------------------------
vtkPVAdvancedReaderModule::~vtkPVAdvancedReaderModule()
{
}

//----------------------------------------------------------------------------
// This method used to fix the output data type of clone.
// It does nothing now, so we should get rid of it........ !!!!!!!!
int vtkPVAdvancedReaderModule::Initialize(const char* fname, 
                                          vtkPVReaderModule*& clone)
{
  int retVal = this->Superclass::Initialize(fname, clone);

  if (retVal != VTK_OK)
    {
    return retVal;
    }
  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVAdvancedReaderModule::ReadFileInformation(const char* fname)
{
  int retVal =  this->Superclass::ReadFileInformation(fname);
  if (retVal != VTK_OK)
    {
    return retVal;
    }
  
  // Update the reader's information on node 0 so that the widgets can
  // get the correct initial values.
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->ServerScript("%s UpdateInformation", this->GetVTKSourceTclName());
  
  // We need to update the widgets.
  vtkPVWidget *pvw;
  vtkPVWidgetCollection* widgets = this->GetWidgets();
  if (widgets)
    {
    widgets->InitTraversal();
    for (int i = 0; i < widgets->GetNumberOfItems(); i++)
      {
      pvw = widgets->GetNextPVWidget();
      pvw->ModifiedCallback();
      }
    this->UpdateParameterWidgets();
    }

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVAdvancedReaderModule::Finalize(const char* fname)
{
  return this->FinalizeInternal(fname, 0);
}

//----------------------------------------------------------------------------
void vtkPVAdvancedReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
