/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtractGeometryByScalar.cxx
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
#include "vtkPVExtractGeometryByScalar.h"

#include "vtkDataSet.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVComponentSelection.h"
#include "vtkPVData.h"
#include "vtkPVWindow.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtractGeometryByScalar);
vtkCxxRevisionMacro(vtkPVExtractGeometryByScalar, "1.6.2.1");

int vtkPVExtractGeometryByScalarCommand(ClientData cd, Tcl_Interp *interp,
                                        int argc, char *argv[]);

//---------------------------------------------------------------------------
vtkPVExtractGeometryByScalar::vtkPVExtractGeometryByScalar()
{
  this->CommandFunction = vtkPVExtractGeometryByScalarCommand;
}

//---------------------------------------------------------------------------
vtkPVExtractGeometryByScalar::~vtkPVExtractGeometryByScalar()
{
}

//---------------------------------------------------------------------------
void vtkPVExtractGeometryByScalar::CreateProperties()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();
  
  this->AddInputMenu("Input", "PVInput", "vtkUnstructuredGrid",
                     "Select the input for the filter.",
                     this->GetPVWindow()->GetSourceList("Sources"));
  
  vtkPVComponentSelection *select = vtkPVComponentSelection::New();
  select->SetPVSource(this);
  select->SetParent(this->GetParameterFrame()->GetFrame());
  select->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  select->SetObjectVariable(this->VTKSourceTclName, "Value");
  
  float range[2];
  this->GetPVInput()->GetVTKData()->GetPointData()->GetScalars()->
    GetRange(range);
  select->SetNumberOfComponents((int)range[1]);
  select->Create(pvApp);
  select->SetTraceName("ComponentSelect");
  this->AddPVWidget(select);
  
  this->Script("pack %s", select->GetWidgetName());
  select->Delete();
  select = NULL;
}

//---------------------------------------------------------------------------
void vtkPVExtractGeometryByScalar::SaveInTclScript(ofstream* vtkNotUsed(file),
                                                   int vtkNotUsed(interactiveFlag), 
                                                   int vtkNotUsed(vtkFlag))
{
}

//----------------------------------------------------------------------------
void vtkPVExtractGeometryByScalar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
