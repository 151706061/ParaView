/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScale.cxx
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
#include "vtkPVScale.h"

#include "vtkArrayMap.txx"
#include "vtkKWEvent.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVScale);
vtkCxxRevisionMacro(vtkPVScale, "1.16.2.3");

//----------------------------------------------------------------------------
vtkPVScale::vtkPVScale()
{
  this->EntryLabel = 0;
  this->LabelWidget = vtkKWLabel::New();
  this->Scale = vtkKWScale::New();
  this->Round = 0;
}

//----------------------------------------------------------------------------
vtkPVScale::~vtkPVScale()
{
  this->SetEntryLabel(0);
  this->Scale->Delete();
  this->Scale = NULL;
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
}

void vtkPVScale::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetLabel(label);
}

void vtkPVScale::SetBalloonHelpString(const char *str)
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
    this->LabelWidget->SetBalloonHelpString(this->BalloonHelpString);
    this->Scale->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVScale::SetResolution(float res)
{
  this->Scale->SetResolution(res);
}

//----------------------------------------------------------------------------
void vtkPVScale::SetRange(float min, float max)
{
  this->Scale->SetRange(min, max);
}

//----------------------------------------------------------------------------
void vtkPVScale::DisplayEntry()
{
  this->Scale->DisplayEntry();
}

//----------------------------------------------------------------------------
void vtkPVScale::SetDisplayEntryAndLabelOnTop(int value)
{
  this->Scale->SetDisplayEntryAndLabelOnTop(value);
}

//----------------------------------------------------------------------------
void vtkPVScale::CheckModifiedCallback()
{
  this->ModifiedCallback();
  this->AcceptedCallback();
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkPVScale::Create(vtkKWApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("PVScale already created");
    return;
    }

  // For getting the widget in a script.
  if (this->EntryLabel && this->EntryLabel[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(this->EntryLabel);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
  
  this->SetApplication(pvApp);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  // Now a label
  this->LabelWidget->SetParent(this);
  this->LabelWidget->Create(pvApp, "-width 18 -justify right");
  this->LabelWidget->SetLabel(this->EntryLabel);
  this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());

  this->Scale->SetParent(this);
  this->Scale->Create(this->Application, "-showvalue 1");
  this->Scale->SetCommand(this, "CheckModifiedCallback");

  this->SetBalloonHelpString(this->BalloonHelpString);
  this->Script("pack %s -side left -fill x -expand t", 
               this->Scale->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVScale::SetValue(float val)
{
  float newVal;
  float oldVal;
  
  if(this->Round)
    {
    newVal = this->RoundValue(val);
    }
  else
    {
    newVal = val;
    }
  
  oldVal = this->Scale->GetValue();
  if (newVal == oldVal)
    {
    return;
    }

  this->Scale->SetValue(newVal); 
  this->ModifiedCallback();
}



//----------------------------------------------------------------------------
void vtkPVScale::AcceptInternal(const char* sourceTclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (sourceTclName && this->VariableName)
    {
    if(this->Round)
      {
      pvApp->BroadcastScript("%s Set%s %d", 
                             sourceTclName,
                             this->VariableName, 
                             this->RoundValue(this->GetValue()));
      }
    else
      {
      pvApp->BroadcastScript("%s Set%s %g", 
                             sourceTclName,
                             this->VariableName, 
                             this->GetValue());
      }
    }

  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVScale::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetValue "
        << this->Scale->GetValue() << endl;
}


//----------------------------------------------------------------------------
void vtkPVScale::ResetInternal(const char* sourceTclName)
{
  if (sourceTclName && this->VariableName)
    {
    this->Script("%s SetValue [%s Get%s]", this->Scale->GetTclName(),
                  this->ObjectTclName, this->VariableName);
    }

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
vtkPVScale* vtkPVScale::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVScale::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVScale::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVScale* pvs = vtkPVScale::SafeDownCast(clone);
  if (pvs)
    {
    float min, max;
    this->Scale->GetRange(min, max);
    pvs->SetRange(min, max);
    pvs->SetResolution(this->Scale->GetResolution());
    pvs->SetLabel(this->EntryLabel);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVScale.");
    }
}

//----------------------------------------------------------------------------
float vtkPVScale::GetValue() 
{ 
  return this->Scale->GetValue(); 
}

//----------------------------------------------------------------------------
int vtkPVScale::ReadXMLAttributes(vtkPVXMLElement* element,
                                  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(!label)
    {
    label = element->GetAttribute("variable");
    if (!label )
      {
      vtkErrorMacro("No label attribute.");
      return 0;
      }
    }
  this->SetLabel(label);

  // Setup the Resolution.
  float resolution;
  if(!element->GetScalarAttribute("resolution",&resolution))
    {
    resolution = 1;
    }
  this->SetResolution(resolution);

  float range[2];
  if(!element->GetVectorAttribute("range",2,range))
    {
    range[0] = 0;
    range[1] = 100;
    }
  this->SetRange(range[0], range[1]);

  return 1;
  
}

//----------------------------------------------------------------------------
void vtkPVScale::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPVScale::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                           vtkPVAnimationInterfaceEntry *ai)
{
  char methodAndArgs[500];
  
  sprintf(methodAndArgs, "AnimationMenuCallback %s", ai->GetTclName()); 
  menu->AddCommand(this->LabelWidget->GetLabel(), this, methodAndArgs, 0,"");
}

//----------------------------------------------------------------------------
void vtkPVScale::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  char script[500];
  
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s)", 
                        this->GetTclName(), ai->GetTclName());
    }
  
  // I do not like setting the label like this but ...
  sprintf(script, "%s Set%s $pvTime", 
          this->ObjectTclName, this->VariableName);
  ai->SetLabelAndScript(this->LabelWidget->GetLabel(), script);
  ai->SetTimeStart(this->Scale->GetRangeMin());
  ai->SetTimeEnd(this->Scale->GetRangeMax());
  ai->SetTypeToInt();
  ai->Update();
}

//----------------------------------------------------------------------------
int vtkPVScale::RoundValue(float val)
{
  if(val >= 0)
    {
    return static_cast<int>(val+0.5);
    }
  else
    {
    return -static_cast<int>((-val)+0.5);
    }
}
