/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtentEntry.cxx
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
#include "vtkPVExtentEntry.h"

#include "vtkArrayMap.txx"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterface.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVInputMenu.h"
#include "vtkPVMinMax.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtentEntry);
vtkCxxRevisionMacro(vtkPVExtentEntry, "1.10");

vtkCxxSetObjectMacro(vtkPVExtentEntry, InputMenu, vtkPVInputMenu);

//---------------------------------------------------------------------------
vtkPVExtentEntry::vtkPVExtentEntry()
{
  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->LabeledFrame->SetParent(this);

  this->Label = 0;

  for (int i=0; i<3; i++)
    {
    this->MinMax[i] = vtkPVMinMax::New();
    }

  this->InputMenu = 0;
}

//---------------------------------------------------------------------------
vtkPVExtentEntry::~vtkPVExtentEntry()
{
  this->LabeledFrame->Delete();
  this->LabeledFrame = 0;

  this->SetLabel(0);

  if (this->InputMenu)
    {
    this->InputMenu->Delete();
    }

  for(int i=0; i<3; i++)
    {
    this->MinMax[i]->Delete();
    this->MinMax[i] = 0;
    }
}

//---------------------------------------------------------------------------
void vtkPVExtentEntry::Update()
{
  this->Superclass::Update();

  vtkPVData *input = this->InputMenu->GetCurrentValue()->GetPVOutput();
  if (input == NULL)
    {
    this->Script("eval %s SetRange %d %d %d %d %d %d",
                 this->GetTclName(), 0, 0, 0, 0, 0, 0);
    }
  else
    {
    this->Script("eval %s SetRange [%s GetWholeExtent]",
                 this->GetTclName(),
                 input->GetVTKDataTclName());
    }

}

void vtkPVExtentEntry::SetBalloonHelpString( const char *str )
{
  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->Application && !this->BalloonHelpInitialized )
    {
    this->LabeledFrame->SetBalloonHelpString(this->BalloonHelpString);
    for (int i=0; i<3; i++)
      {
      this->MinMax[i]->SetBalloonHelpString(this->BalloonHelpString);
      }

    this->BalloonHelpInitialized = 1;
    }
}

//---------------------------------------------------------------------------
void vtkPVExtentEntry::Create(vtkKWApplication *pvApp)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("VectorEntry already created");
    return;
    }
  
  // For getting the widget in a script.
  if (this->Label && this->Label[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(this->Label);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  this->SetApplication(pvApp);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);

  this->LabeledFrame->Create(pvApp);
  // Now a label
  if (this->Label && this->Label[0] != '\0')
    {
    this->LabeledFrame->SetLabel(this->Label);
    }
  else
    {
    this->LabeledFrame->SetLabel("Extent");
    }
   
  char labels[3][4] = { "I: ", "J: ", "K: "};
  int i;
  for(i=0; i<3; i++)
    {
    this->MinMax[i]->SetParent(this->LabeledFrame->GetFrame());
    this->MinMax[i]->PackVerticallyOff();
    this->MinMax[i]->ShowMaxLabelOff();
    this->MinMax[i]->SetMinLabelWidth(2);
    this->MinMax[i]->Create(pvApp);
    this->MinMax[i]->SetMinimumLabel(labels[i]);
    this->MinMax[i]->GetMinScale()->SetEndCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMinScale()->DisplayEntry();
    this->MinMax[i]->GetMinScale()->DisplayLabel(" Min:");
    this->MinMax[i]->GetMaxScale()->SetEndCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMaxScale()->DisplayEntry();
    this->MinMax[i]->GetMaxScale()->DisplayLabel(" Max:");
    }

  // Initialize the extent of the VTK source. Normally, it
  // is set to -VTK_LARGE_FLOAT, VTK_LARGE_FLOAT...
  if (this->PVSource && this->PVSource->GetPVInput() && this->VariableName )
    {
    this->Script("eval %s Set%s [%s GetWholeExtent]",
                 this->PVSource->GetVTKSourceTclName(), 
                 this->GetVariableName(),
                 this->PVSource->GetPVInput()->GetVTKDataTclName());

    this->Script("eval %s SetRange [%s GetWholeExtent]",
                 this->GetTclName(),
                 this->PVSource->GetPVInput()->GetVTKDataTclName());
    }

  for(i=0; i<3; i++)
    {
    this->Script("pack %s -side top -fill x -expand t -pady 5", 
                 this->MinMax[i]->GetWidgetName());
    }

  this->Script("pack %s -side left -fill x -expand t", 
               this->LabeledFrame->GetWidgetName());

  this->SetBalloonHelpString(this->BalloonHelpString);
}


//---------------------------------------------------------------------------
void vtkPVExtentEntry::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  ofstream *traceFile = pvApp->GetTraceFile();
  int traceFlag = 0;

  if ( ! this->ModifiedFlag)
    {
    return;
    }

  // Start the trace entry and the accept command.
  if (traceFile && this->InitializeTrace())
    {
    traceFlag = 1;
    }

  if (traceFlag)
    {
    *traceFile << "$kw(" << this->GetTclName() << ") SetValue ";
    for(int i=0; i<3; i++)
      {
      *traceFile << this->MinMax[i]->GetMinValue() << " "
                 << this->MinMax[i]->GetMaxValue() << " ";
      }
    *traceFile << endl;
    }

  pvApp->BroadcastScript("%s Set%s %d %d %d %d %d %d", 
                         this->ObjectTclName, this->VariableName,
                         static_cast<int>(this->MinMax[0]->GetMinValue()), 
                         static_cast<int>(this->MinMax[0]->GetMaxValue()),
                         static_cast<int>(this->MinMax[1]->GetMinValue()), 
                         static_cast<int>(this->MinMax[1]->GetMaxValue()),
                         static_cast<int>(this->MinMax[2]->GetMinValue()), 
                         static_cast<int>(this->MinMax[2]->GetMaxValue()));

  this->ModifiedFlag = 0;  
}

//---------------------------------------------------------------------------
void vtkPVExtentEntry::Reset()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  this->Script("eval %s SetValue [%s Get%s]",
               this->GetTclName(), this->ObjectTclName,  this->VariableName);

  this->ModifiedFlag = 0;
}


//---------------------------------------------------------------------------
void vtkPVExtentEntry::SetRange(int v0, int v1, int v2, 
                                int v3, int v4, int v5)
{
  this->MinMax[0]->SetRange(v0, v1);
  this->MinMax[1]->SetRange(v2, v3);
  this->MinMax[2]->SetRange(v4, v5);

  this->ModifiedCallback();
}

//---------------------------------------------------------------------------
void vtkPVExtentEntry::SetValue(int v0, int v1, int v2, 
                                int v3, int v4, int v5)
{
  this->MinMax[0]->SetMaxValue(v1);
  this->MinMax[0]->SetMinValue(v0);

  this->MinMax[1]->SetMaxValue(v3);
  this->MinMax[1]->SetMinValue(v2);

  this->MinMax[2]->SetMaxValue(v5);
  this->MinMax[2]->SetMinValue(v4);

  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVExtentEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                                 vtkPVAnimationInterface *ai)
{
  vtkKWMenu *cascadeMenu;
  char methodAndArgs[200];

  // Lets create a cascade menu to keep things neat.
  cascadeMenu = vtkKWMenu::New();
  cascadeMenu->SetParent(menu);
  cascadeMenu->Create(this->Application, "-tearoff 0");
  menu->AddCascade(this->GetTraceName(), cascadeMenu, 0,
                             "Choose a plane of the extent to animate.");  
  // X
  sprintf(methodAndArgs, "AnimationMenuCallback %s 0", ai->GetTclName());
  cascadeMenu->AddCommand("X Axis", this, methodAndArgs, 0, "");
  // Y
  sprintf(methodAndArgs, "AnimationMenuCallback %s 1", ai->GetTclName());
  cascadeMenu->AddCommand("Y Axis", this, methodAndArgs, 0, "");
  // Z
  sprintf(methodAndArgs, "AnimationMenuCallback %s 2", ai->GetTclName());
  cascadeMenu->AddCommand("Z Axis", this, methodAndArgs, 0, "");

  cascadeMenu->Delete();
  cascadeMenu = NULL;

  return;
}

//----------------------------------------------------------------------------
void vtkPVExtentEntry::AnimationMenuCallback(vtkPVAnimationInterface *ai,
                                             int mode)
{
  char script[500];
  int ext[6];

  // Get the whole extent to set up defaults.
  // Now I can imagine that we will need a more flexible way of getting 
  // the whole extent from sources (in the future.
  this->Script("[%s GetInput] GetWholeExtent", this->ObjectTclName);
  sscanf(this->Application->GetMainInterp()->result, "%d %d %d %d %d %d",
         ext, ext+1, ext+2, ext+3, ext+4, ext+5);

  if (mode == 0)
    {
    sprintf(script, 
            "%s Set%s [expr int($pvTime)] [expr int($pvTime)] %d %d %d %d", 
            this->ObjectTclName,this->VariableName,ext[2],ext[3],ext[4],ext[5]);
    ai->SetLabelAndScript("X Axis", script);
    ai->SetTimeStart(ext[0]);
    ai->SetCurrentTime(ext[0]);
    ai->SetTimeEnd(ext[1]);
    ai->SetTimeStep(1.0);
    }
  else if (mode == 1)
    {
    sprintf(script, 
            "%s Set%s %d %d [expr int($pvTime)] [expr int($pvTime)] %d %d", 
            this->ObjectTclName,this->VariableName,ext[0],ext[1],ext[4],ext[5]);
    ai->SetLabelAndScript("Y Axis", script);
    ai->SetTimeStart(ext[2]);
    ai->SetCurrentTime(ext[2]);
    ai->SetTimeEnd(ext[3]);
    ai->SetTimeStep(1.0);
    }
  else if (mode == 2)
    {
    sprintf(script, 
            "%s Set%s %d %d %d %d [expr int($pvTime)] [expr int($pvTime)]", 
            this->ObjectTclName,this->VariableName,ext[0],ext[1],ext[2],ext[3]);
    ai->SetLabelAndScript("Z Axis", script);
    ai->SetTimeStart(ext[4]);
    ai->SetCurrentTime(ext[4]);
    ai->SetTimeEnd(ext[5]);
    ai->SetTimeStep(1.0);
    }
  else
    {
    vtkErrorMacro("Bad extent animation mode.");
    }
  ai->SetControlledWidget(this);
}

vtkPVExtentEntry* vtkPVExtentEntry::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVExtentEntry::SafeDownCast(clone);
}

void vtkPVExtentEntry::CopyProperties(vtkPVWidget* clone, 
                                      vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVExtentEntry* pvee = vtkPVExtentEntry::SafeDownCast(clone);
  if (pvee)
    {
    pvee->SetLabel(this->Label);

    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvee->SetInputMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVExtentEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVExtentEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetLabel(label);
    }
  else
    {
    this->SetLabel(this->VariableName);
    }
  
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if(!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }
  
  vtkPVXMLElement* ime = element->LookupElement(input_menu);
  vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
  vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
  if(!imw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
    return 0;
    }
  imw->AddDependent(this);
  this->SetInputMenu(imw);
  imw->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVExtentEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "InputMenu: " << this->InputMenu << endl;
  os << "Label: " << (this->Label ? this->Label : "(none)") << endl;
}
