/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyph3D.cxx
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
#include "vtkPVGlyph3D.h"

#include "vtkKWCompositeCollection.h"
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVInputMenu.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVSelectionList.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVArrayMenu.h"
#include "vtkDataSetAttributes.h"
#include "vtkPVWindow.h"
#include "vtkStringList.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGlyph3D);
vtkCxxRevisionMacro(vtkPVGlyph3D, "1.73.2.2");

int vtkPVGlyph3DCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVGlyph3D::vtkPVGlyph3D()
{
  this->CommandFunction = vtkPVGlyph3DCommand;
  
  this->GlyphSource = NULL;

  // Glyph adds too its input, so sould not replace it.
  this->ReplaceInput = 0;
}

//----------------------------------------------------------------------------
vtkPVGlyph3D::~vtkPVGlyph3D()
{
  if (this->GlyphSource)
    {
    this->GlyphSource->UnRegister(this);
    this->GlyphSource = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::SetGlyphSource(vtkPVData *source)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->SetNthPVInput(1, source);

  if (source)
    {
    source->Register(this);
    pvApp->BroadcastScript("%s SetSource %s", this->GetVTKSourceTclName(),
                          source->GetVTKDataTclName());
    }
  else
    {
    pvApp->BroadcastScript("%s SetSource {}", this->GetVTKSourceTclName());
    }
  if (this->GlyphSource)
    {
    this->GlyphSource->UnRegister(this);
    }
  this->GlyphSource = source;
}

//----------------------------------------------------------------------------
vtkPVData* vtkPVGlyph3D::GetGlyphSource()
{
  return this->GlyphSource;
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::CreateProperties()
{
  this->vtkPVSource::CreateProperties();
}

void vtkPVGlyph3D::InitializePrototype()
{
  vtkPVInputMenu *sourceMenu;
  
  this->Superclass::InitializePrototype();

  /*
  inputMenu = this->AddInputMenu(
    "Input", "PVInput", "vtkDataSet",
    "Select the input for the filter.",
    this->GetPVWindow()->GetSourceList("Sources"));
  inputMenu->SetVTKInputName("Input");
  inputMenu->SetModifiedCommand(this->GetTclName(), 
                                "SetAcceptButtonColorToRed");
  */

  sourceMenu = this->AddInputMenu(
    "Glyph", "GlyphSource", "vtkPolyData",
    "Select the data set to use as the glyph geometry.",
    this->GetPVWindow()->GetSourceList("GlyphSources"));
  sourceMenu->SetVTKInputName("Source");
  sourceMenu->SetModifiedCommand(this->GetTclName(), 
                                "SetAcceptButtonColorToRed");
    
  /* Not necessary now that we have input arraySelection.
  sl = vtkPVSelectionList::New();  
  sl->SetParent(this->ParameterFrame->GetFrame());
  sl->SetLabel("Vector Mode");
  sl->SetObjectVariable(this->GetVTKSourceTclName(), "VectorMode");
  sl->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  sl->Create(this->Application);
  sl->SetBalloonHelpString("Select what to use as vectors for "
                           "scaling/rotation");
  sl->AddItem("Vector", 0);
  //sl->AddItem("Normal", 1);
  sl->AddItem("Vector RotationOff", 2);
  this->AddPVWidget(sl);
  sl->Delete();
  */

  /*
  vtkPVLabeledToggle *toggle = vtkPVLabeledToggle::New();
  toggle->SetParent(this->ParameterFrame->GetFrame());
  toggle->SetObjectVariable(this->GetVTKSourceTclName(), "Orient");
  toggle->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  toggle->SetLabel("Orient");
  toggle->Create(this->Application);
  toggle->SetBalloonHelpString("Select whether to orient the glyphs");
  this->AddPVWidget(toggle);
  toggle->Delete();

  vtkPVArrayMenu *am = vtkPVArrayMenu::New();
  am->SetLabel("Vectors");
  am->SetNumberOfComponents(3);
  am->SetInputName("Input");
  am->SetAttributeType(vtkDataSetAttributes::VECTORS);
  am->SetObjectTclName(this->GetVTKSourceTclName());
  am->SetInputMenu(inputMenu);
  this->AddPVWidget(am);
  am->Delete();
  am = NULL;
  */

  /*
  vtkPVSelectionList *sl = vtkPVSelectionList::New();  
  sl->SetParent(this->ParameterFrame->GetFrame());
  sl->SetLabel("Scale Mode");
  sl->SetObjectVariable(this->GetVTKSourceTclName(), "ScaleMode");
  sl->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  sl->Create(this->Application);
  sl->SetBalloonHelpString("Select whether/how to scale the glyphs");
  sl->AddItem("Scalar", 0);
  sl->AddItem("Vector", 1);
  sl->AddItem("Vector Components", 2);
  sl->AddItem("Data Scalaing Off", 3);
  this->AddPVWidget(sl);
  sl->Delete();
  */

  /* Scaling off can be achieved with DataScaling Off and scale factor of 1.
  toggle = vtkPVLabeledToggle::New();
  toggle->SetParent(this->ParameterFrame->GetFrame());
  toggle->SetObjectVariable(this->GetVTKSourceTclName(), "Scaling");
  toggle->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  toggle->SetLabel("Scaling");
  toggle->Create(this->Application);
  toggle->SetBalloonHelpString("Select whether to scale the glyphs");
  this->AddPVWidget(toggle);
  toggle->Delete();
  */

  /*
  vtkPVVectorEntry *entry = vtkPVVectorEntry::New();
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), "ScaleFactor");
  entry->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  entry->SetLabel("Scale Factor");
  entry->SetBalloonHelpString("Select the amount to scale the glyphs by");
  entry->SetDataType(VTK_FLOAT);
  entry->Create(this->Application);
  this->AddPVWidget(entry);
  entry->Delete();
  */
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
