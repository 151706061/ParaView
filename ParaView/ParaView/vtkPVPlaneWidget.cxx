/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlaneWidget.cxx
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
#include "vtkPVPlaneWidget.h"

#include "vtkCamera.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkPlaneWidget.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVPlaneWidget);
vtkCxxRevisionMacro(vtkPVPlaneWidget, "1.28");

int vtkPVPlaneWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPlaneWidget::vtkPVPlaneWidget()
{
  if ( this->Widget3D )
    {
    this->Widget3D->Delete();
    }
  vtkPlaneWidget *plane = vtkPlaneWidget::New();
  plane->SetPlaceFactor(1.0);
  this->Widget3D = plane;
}

//----------------------------------------------------------------------------
vtkPVPlaneWidget::~vtkPVPlaneWidget()
{
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::Reset()
{
  this->Superclass::Reset();
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::ActualPlaceWidget()
{
  this->Superclass::ActualPlaceWidget();
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::Accept()
{
  this->Superclass::Accept();
}


//----------------------------------------------------------------------------
void vtkPVPlaneWidget::SaveInTclScript(ofstream *file)
{
  *file << "vtkPlane " << this->PlaneTclName << endl;
  *file << "\t" << this->PlaneTclName << " SetOrigin ";
  this->Script("%s GetOrigin", this->PlaneTclName);
  *file << this->Application->GetMainInterp()->result << endl;
  *file << "\t" << this->PlaneTclName << " SetNormal ";
  this->Script("%s GetNormal", this->PlaneTclName);
  *file << this->Application->GetMainInterp()->result << endl;
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

vtkPVPlaneWidget* vtkPVPlaneWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVPlaneWidget::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::ChildCreate(vtkPVApplication* pvApp)
{
  this->Superclass::ChildCreate(pvApp);
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  vtkPlaneWidget *widget = vtkPlaneWidget::SafeDownCast(wdg);
  if ( widget )
    {
    float val[3];
    int cc;
    widget->GetCenter(val); 
    for (cc=0; cc < 3; cc ++ )
      {
      this->CenterEntry[cc]->SetValue(val[cc], 5);
      }
    widget->GetNormal(val); 
    for (cc=0; cc < 3; cc ++ )
      {
      this->NormalEntry[cc]->SetValue(val[cc], 5);
      }
    }
  this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
int vtkPVPlaneWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::SetCenter(float x, float y, float z)
{
  this->CenterEntry[0]->SetValue(x, 3);
  this->CenterEntry[1]->SetValue(y, 3);
  this->CenterEntry[2]->SetValue(z, 3); 
  this->ModifiedFlag = 1;
  if ( this->Widget3D )
    {
    vtkPlaneWidget *plane = static_cast<vtkPlaneWidget*>(this->Widget3D);
    plane->SetCenter(x, y, z); 
    }
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::SetNormal(float x, float y, float z)
{
  this->NormalEntry[0]->SetValue(x, 3);
  this->NormalEntry[1]->SetValue(y, 3);
  this->NormalEntry[2]->SetValue(z, 3); 
  this->ModifiedFlag = 1;
  if ( this->Widget3D )
    {
    vtkPlaneWidget *plane = static_cast<vtkPlaneWidget*>(this->Widget3D);
    plane->SetNormal(x, y, z); 
    }
}

