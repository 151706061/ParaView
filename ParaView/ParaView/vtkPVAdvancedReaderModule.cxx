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
#include "vtkPVData.h"
#include "vtkPVFileEntry.h"
#include "vtkPVWidgetCollection.h"
#include "vtkString.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAdvancedReaderModule);
vtkCxxRevisionMacro(vtkPVAdvancedReaderModule, "1.5");

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
int vtkPVAdvancedReaderModule::ReadFile(const char* fname, 
                                        vtkPVReaderModule*& clone)
{
  // If the reader has as an output vtkDataSet or vtkPointSet, then we
  // should fix it, but just for the clone process. After the clone
  // process, we should change it back.
  int isdatasetreader = vtkString::Equals(this->OutputClassName, "vtkDataSet");
  int ispointsetreader = vtkString::Equals(this->OutputClassName, "vtkPointSet");
  if ( ispointsetreader || isdatasetreader )
    {
    char* res = vtkString::Duplicate(
      this->Script("%s vtkPVAdvancedReaderModuleTemporaryVariable", 
                   this->SourceClassName));

    // Change the hardcoded "FileName" to something more elaborated
    this->Script("%s Set%s %s", res, "FileName", fname);
    this->Script("%s UpdateInformation", res);
    this->SetOutputClassName(this->Script("[ %s GetOutput ] GetClassName", res));
    this->Script("%s Delete", res);
    delete[] res;
    }

  int retVal = this->Superclass::ReadFile(fname, clone);

  // If the reader was dataset or pointset reader, then modify the
  // default output class name back to whatever it was before.
  if ( isdatasetreader )
    {
    this->SetOutputClassName("vtkDataSet");
    }
  if ( ispointsetreader )
    {
    this->SetOutputClassName("vtkPointSet");
    }

  if (retVal != VTK_OK)
    {
    return retVal;
    }

  this->Script("%s UpdateInformation", 
               clone->GetPVOutput()->GetVTKDataTclName());

  // We called UpdateInformation, we need to update the widgets.
  vtkPVWidget *pvw;
  vtkPVWidgetCollection* widgets = clone->GetWidgets();
  if (widgets)
    {
    widgets->InitTraversal();
    for (int i = 0; i < widgets->GetNumberOfItems(); i++)
      {
      pvw = widgets->GetNextPVWidget();
      pvw->ModifiedCallback();
      }
    clone->UpdateParameterWidgets();
    }

  return VTK_OK;
}


//----------------------------------------------------------------------------
void vtkPVAdvancedReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
