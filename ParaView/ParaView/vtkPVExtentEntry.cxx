/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtentEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVMinMax.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVExtentWidgetProperty.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtentEntry);
vtkCxxRevisionMacro(vtkPVExtentEntry, "1.31");

vtkCxxSetObjectMacro(vtkPVExtentEntry, InputMenu, vtkPVInputMenu);

//-----------------------------------------------------------------------------
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

  this->Range[0] = this->Range[2] = this->Range[4] = -VTK_LARGE_INTEGER;
  this->Range[1] = this->Range[3] = this->Range[5] = VTK_LARGE_INTEGER;

  this->Property = NULL;

  this->AnimationAxis = 0;
  this->UseCellExtent = 0;
}

//-----------------------------------------------------------------------------
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
  
  this->SetProperty(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::Update()
{
  this->Superclass::Update();

  vtkPVSource *input = this->InputMenu->GetCurrentValue();
  if (input == NULL)
    {
    this->SetRange(0, 0, 0, 0, 0, 0);
    this->SetValue(0, 0, 0, 0, 0, 0);
    }
  else
    {
    int *ext = input->GetDataInformation()->GetExtent();
    if (!this->UseCellExtent)
      {
      this->SetRange(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      this->SetValue(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else
      {
      this->SetRange(ext[0], ext[1]-1, ext[2], ext[3]-1, ext[4], ext[5]-1);
      this->SetValue(ext[0], ext[1]-1, ext[2], ext[3]-1, ext[4], ext[5]-1);
      }
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

//-----------------------------------------------------------------------------
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

  this->LabeledFrame->Create(pvApp, 0);
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
    this->MinMax[i]->SetRange(this->Range[i*2], this->Range[i*2+1]);
    this->MinMax[i]->SetMinimumLabel(labels[i]);
    this->MinMax[i]->GetMinScale()->SetEndCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMinScale()->SetEntryCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMinScale()->SetDisplayEntryAndLabelOnTop(1);
    this->MinMax[i]->GetMinScale()->DisplayEntry();
    this->MinMax[i]->GetMinScale()->DisplayLabel(" Min:");
    this->MinMax[i]->GetMaxScale()->SetEndCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMaxScale()->SetEntryCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMaxScale()->SetDisplayEntryAndLabelOnTop(1);
    this->MinMax[i]->GetMaxScale()->DisplayEntry();
    this->MinMax[i]->GetMaxScale()->DisplayLabel(" Max:");
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


//-----------------------------------------------------------------------------
void vtkPVExtentEntry::AcceptInternal(vtkClientServerID sourceID)
{
  float values[6];
  values[0] = this->MinMax[0]->GetMinValue();
  values[1] = this->MinMax[0]->GetMaxValue();
  values[2] = this->MinMax[1]->GetMinValue();
  values[3] = this->MinMax[1]->GetMaxValue();
  values[4] = this->MinMax[2]->GetMinValue();
  values[5] = this->MinMax[2]->GetMaxValue();

  this->Property->SetVTKSourceID(sourceID);
  this->Property->SetScalars(6, values);
  this->Property->AcceptInternal();
  
  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetValue ";
  for(int i=0; i<3; i++)
    {
    *file << this->MinMax[i]->GetMinValue() << " "
          << this->MinMax[i]->GetMaxValue() << " ";
    }
  *file << endl;
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  float *values = this->Property->GetScalars();
  this->SetValue(static_cast<int>(values[0]), static_cast<int>(values[1]),
                 static_cast<int>(values[2]), static_cast<int>(values[3]),
                 static_cast<int>(values[4]), static_cast<int>(values[5]));
  
  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::SetRange(int v0, int v1, int v2, 
                                int v3, int v4, int v5)
{
  this->Range[0] = v0;
  this->Range[1] = v1;
  this->Range[2] = v2;
  this->Range[3] = v3;
  this->Range[4] = v4;
  this->Range[5] = v5;

  if (this->Application)
    {
    this->MinMax[0]->SetRange(v0, v1);
    this->MinMax[1]->SetRange(v2, v3);
    this->MinMax[2]->SetRange(v4, v5);
    }

  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::SetValue(int v0, int v1, int v2, 
                                int v3, int v4, int v5)
{
  float range[2];

  // First, restrict value to current range.
  this->MinMax[0]->GetRange(range);
  if (v0 < range[0]) {v0 = static_cast<int>(range[0]);}
  if (v0 > range[1]) {v0 = static_cast<int>(range[1]);}
  if (v1 < range[0]) {v1 = static_cast<int>(range[0]);}
  if (v1 > range[1]) {v1 = static_cast<int>(range[1]);}
  this->MinMax[1]->GetRange(range);
  if (v2 < range[0]) {v2 = static_cast<int>(range[0]);}
  if (v2 > range[1]) {v2 = static_cast<int>(range[1]);}
  if (v3 < range[0]) {v3 = static_cast<int>(range[0]);}
  if (v3 > range[1]) {v3 = static_cast<int>(range[1]);}
  this->MinMax[2]->GetRange(range);
  if (v4 < range[0]) {v4 = static_cast<int>(range[0]);}
  if (v4 > range[1]) {v4 = static_cast<int>(range[1]);}
  if (v5 < range[0]) {v5 = static_cast<int>(range[0]);}
  if (v5 > range[1]) {v5 = static_cast<int>(range[1]);}

  float values[6];
  
  if ( v1 >= v0 )
    {
    if ( (float)v1 < this->MinMax[0]->GetMinValue() )
      {
      this->MinMax[0]->SetMinValue(v0);
      this->MinMax[0]->SetMaxValue(v1);
      values[0] = v0;
      values[1] = v1;
      }
    else
      {
      this->MinMax[0]->SetMaxValue(v1);
      this->MinMax[0]->SetMinValue(v0);
      values[0] = v1;
      values[1] = v0;
      }
    }

  if ( v3 >= v2 )
    {
    if ( (float)v3 < this->MinMax[1]->GetMinValue() )
      {
      this->MinMax[1]->SetMinValue(v2);
      this->MinMax[1]->SetMaxValue(v3);
      values[2] = v2;
      values[3] = v3;
      }
    else
      {
      this->MinMax[1]->SetMaxValue(v3);
      this->MinMax[1]->SetMinValue(v2);
      values[2] = v3;
      values[3] = v2;
      }
    }

  if ( v5 >= v4 )
    {
    if ( (float)v5 < this->MinMax[2]->GetMinValue() )
      {
      this->MinMax[2]->SetMinValue(v4);
      this->MinMax[2]->SetMaxValue(v5);
      values[4] = v4;
      values[5] = v5;
      }
    else
      {
      this->MinMax[2]->SetMaxValue(v5);
      this->MinMax[2]->SetMinValue(v4);
      values[4] = v5;
      values[5] = v4;
      }
    }
  
  if (!this->AcceptCalled)
    {
    this->Property->SetScalars(6, values);
    }
  
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                                 vtkPVAnimationInterfaceEntry *ai)
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
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai,
                                             int mode)
{
  int ext[6];

  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s) %d", 
      this->GetTclName(), ai->GetTclName(), mode);
    }


  // Get the whole extent to set up defaults.
  // Now I can imagine that we will need a more flexible way of getting 
  // the whole extent from sources (in the future.
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->ObjectID << "GetInput"
                  << vtkClientServerStream::End
                  << vtkClientServerStream::Invoke
                  << vtkClientServerStream::LastResult << "GetWholeExtent"
                  << vtkClientServerStream::End;
  pm->SendStreamToServerRoot();
  if(!pm->GetLastServerResult().GetArgument(0, 0, ext, 6))
    {
    vtkErrorMacro("Bad return value from GetWholeExtent");
    }

  if (mode == 0)
    {
    ai->SetLabelAndScript("X Axis", NULL);
    ai->SetTimeStart(ext[0]);
    ai->SetTimeEnd(ext[1]);
    }
  else if (mode == 1)
    {
    ai->SetLabelAndScript("Y Axis", NULL);
    ai->SetTimeStart(ext[2]);
    ai->SetTimeEnd(ext[3]);
    }
  else if (mode == 2)
    {
    ai->SetLabelAndScript("Z Axis", NULL);
    ai->SetTimeStart(ext[4]);
    ai->SetTimeEnd(ext[5]);
    }
  else
    {
    vtkErrorMacro("Bad extent animation mode.");
    }

  this->SetAnimationAxis(mode);
  ai->SetCurrentProperty(this->Property);
  ai->Update();
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
    pvee->UseCellExtent = this->UseCellExtent;

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

//-----------------------------------------------------------------------------
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

  // Setup the cell_extent.
  if(!element->GetScalarAttribute("use_cell_extent",
                                  &this->UseCellExtent))
    {
    this->UseCellExtent = 0;
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

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVExtentWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    char *cmd = new char[strlen(this->VariableName)+4];
    sprintf(cmd, "Set%s", this->VariableName);
    int numScalars = 6;
    this->Property->SetVTKCommands(1, &cmd, &numScalars);
    delete [] cmd;
    }
}

//-----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVExtentEntry::GetProperty()
{
  return this->Property;
}

//-----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVExtentEntry::CreateAppropriateProperty()
{
  return vtkPVExtentWidgetProperty::New();
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
  os << indent << "Label: " << (this->Label ? this->Label : "(none)") << endl;
  os << indent << "Range: " << this->Range[0] << " " << this->Range[1] << endl;
  os << indent << "AnimationAxis: " << this->AnimationAxis << endl;
}
