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
#include "vtkKWFrameWithLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVMinMax.h"
#include "vtkSMPart.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMExtentDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVTraceHelper.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtentEntry);
vtkCxxRevisionMacro(vtkPVExtentEntry, "1.64");

vtkCxxSetObjectMacro(vtkPVExtentEntry, InputMenu, vtkPVInputMenu);

//-----------------------------------------------------------------------------
vtkPVExtentEntry::vtkPVExtentEntry()
{
  this->LabeledFrame = vtkKWFrameWithLabel::New();
  this->LabeledFrame->SetParent(this);

  this->Label = 0;

  for (int i=0; i<3; i++)
    {
    this->MinMax[i] = vtkPVMinMax::New();
    }

  this->InputMenu = 0;

  this->Range[0] = this->Range[2] = this->Range[4] = -VTK_LARGE_INTEGER;
  this->Range[1] = this->Range[3] = this->Range[5] = VTK_LARGE_INTEGER;
}

//-----------------------------------------------------------------------------
vtkPVExtentEntry::~vtkPVExtentEntry()
{
  this->LabeledFrame->Delete();
  this->LabeledFrame = 0;

  this->SetLabel(0);
  this->SetInputMenu(0);

  for(int i=0; i<3; i++)
    {
    this->MinMax[i]->Delete();
    this->MinMax[i] = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::Update()
{
  this->Superclass::Update();

  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMExtentDomain *dom = 0;
  
  if (prop)
    {
    dom = vtkSMExtentDomain::SafeDownCast(prop->GetDomain("extent"));
    }
  
  if (!prop || !dom)
    {
    vtkErrorMacro("Property or domain (extent) could not be found.");
    this->SetRange(0, 0, 0, 0, 0, 0);
    this->SetValue(0, 0, 0, 0, 0, 0);
    }
  else
    {
    int ext[6], i, exists;
    for (i = 0; i < 3; i++)
      {
      ext[2*i] = dom->GetMinimum(i, exists);
      if (!exists)
        {
        ext[2*i] = 0;
        }
      ext[2*i+1] = dom->GetMaximum(i, exists);
      if (!exists)
        {
        ext[2*i+1] = 0;
        }
      }
    
    this->SetRange(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
    this->SetValue(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
    }
}

void vtkPVExtentEntry::SetBalloonHelpString( const char *str )
{
  this->Superclass::SetBalloonHelpString(str);

  if (this->LabeledFrame)
    {
    this->LabeledFrame->SetBalloonHelpString(str);
    }

  for (int i=0; i<3; i++)
    {
    if (this->MinMax[i])
      {
      this->MinMax[i]->SetBalloonHelpString(str);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  // For getting the widget in a script.

  if (this->Label && this->Label[0] &&
      (this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateUninitialized ||
       this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateDefault) )
    {
    this->GetTraceHelper()->SetObjectName(this->Label);
    this->GetTraceHelper()->SetObjectNameState(
      vtkPVTraceHelper::ObjectNameStateSelfInitialized);
    }

  this->LabeledFrame->Create();

  // Now a label

  if (this->Label && this->Label[0] != '\0')
    {
    this->LabeledFrame->SetLabelText(this->Label);
    }
  else
    {
    this->LabeledFrame->SetLabelText("Extent");
    }
   
  char labels[3][4] = { "I: ", "J: ", "K: "};
  int i;
  for(i=0; i<3; i++)
    {
    this->MinMax[i]->SetParent(this->LabeledFrame->GetFrame());
    this->MinMax[i]->PackVerticallyOff();
    this->MinMax[i]->ShowMaxLabelOff();
    this->MinMax[i]->SetMinLabelWidth(2);
    this->MinMax[i]->Create();
    this->MinMax[i]->SetRange(this->Range[i*2], this->Range[i*2+1]);
    this->MinMax[i]->SetMinimumLabel(labels[i]);
    this->MinMax[i]->GetMinScale()->SetEndCommand(this, "ScaleModifiedCallback");
    this->MinMax[i]->GetMinScale()->SetEntryCommand(this, "ScaleModifiedCallback");
    this->MinMax[i]->GetMinScale()->SetEntryPositionToTop();
    this->MinMax[i]->GetMinScale()->SetLabelPositionToTop();
    this->MinMax[i]->GetMinScale()->SetLabelText(" Min.");
    this->MinMax[i]->GetMaxScale()->SetEndCommand(this, "ScaleModifiedCallback");
    this->MinMax[i]->GetMaxScale()->SetEntryCommand(this, "ScaleModifiedCallback");
    this->MinMax[i]->GetMaxScale()->SetEntryPositionToTop();
    this->MinMax[i]->GetMaxScale()->SetLabelPositionToTop();
    this->MinMax[i]->GetMaxScale()->SetLabelText(" Max.");
    }
  
  for(i=0; i<3; i++)
    {
    this->Script("pack %s -side top -fill x -expand t -pady 5", 
                 this->MinMax[i]->GetWidgetName());
    }

  this->Script("pack %s -side left -fill x -expand t", 
               this->LabeledFrame->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::ScaleModifiedCallback(double)
{
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::SaveInBatchScript(ofstream *file)
{
  const char* sourceID = this->PVSource->GetProxy()->GetSelfIDAsString();
  
  if (!sourceID || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }
  
  int cc;
  for (cc = 0; cc < 3; cc++)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] SetElement " << 2*cc << " "
          << this->MinMax[cc]->GetMinValue() << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] SetElement " << 2*cc+1 << " "
          << this->MinMax[cc]->GetMaxValue() << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::Accept()
{
  int i;
  
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (ivp)
    {
    ivp->SetNumberOfElements(6);
    for (i = 0; i < 3; i++)
      {
      ivp->SetElement(2*i, static_cast<int>(this->MinMax[i]->GetMinValue()));
      ivp->SetElement(2*i+1, static_cast<int>(this->MinMax[i]->GetMaxValue()));
      }
    }
  else
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceHelper()->GetObjectName());
    }

  this->Superclass::Accept();
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
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
void vtkPVExtentEntry::Initialize()
{
  this->Update();
  this->Accept();
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::ResetInternal()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (ivp)
    {
    this->SetValueInternal(ivp->GetElement(0), ivp->GetElement(1),
                   ivp->GetElement(2), ivp->GetElement(3),
                   ivp->GetElement(4), ivp->GetElement(5));
    }
  
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

  if (this->GetApplication())
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
  this->SetValueInternal(v0, v1, v2, v3, v4, v5); 
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::SetValueInternal(int v0, int v1, int v2, 
                                int v3, int v4, int v5)
{
  double range[2];

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

  if ( v1 >= v0 )
    {
    this->MinMax[0]->SetMinValue(v0);
    this->MinMax[0]->SetMaxValue(v1);
    }

  if ( v3 >= v2 )
    {
    this->MinMax[1]->SetMinValue(v2);
    this->MinMax[1]->SetMaxValue(v3);
    }

  if ( v5 >= v4 )
    {
    this->MinMax[2]->SetMinValue(v4);
    this->MinMax[2]->SetMaxValue(v5);
    }
}


//-----------------------------------------------------------------------------
vtkPVExtentEntry* vtkPVExtentEntry::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVExtentEntry::SafeDownCast(clone);
}

//-----------------------------------------------------------------------------
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
    this->SetLabel("Extent");
    }
  
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if(!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }

  vtkPVXMLElement* ime = element->LookupElement(input_menu);
  if (!ime)
    {
    vtkErrorMacro("Couldn't find InputMenu element" << input_menu);
    return 0;
    }
  
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
void vtkPVExtentEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabeledFrame);
  this->PropagateEnableState(this->InputMenu);

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PropagateEnableState(this->MinMax[cc]);
    }
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
  os << indent << "Label: " << (this->Label ? this->Label : "(none)") << endl;
  os << indent << "Range: " << this->Range[0] << " " << this->Range[1] << endl;
}
