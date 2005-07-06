/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExponentialKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVExponentialKeyFrame.h"
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkKWThumbWheel.h"
#include "vtkSMExponentialKeyFrameProxy.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkPVExponentialKeyFrame);
vtkCxxRevisionMacro(vtkPVExponentialKeyFrame, "1.9");

//Helper methods to down cast the property and set value.
inline static int DoubleVectPropertySetElement(vtkSMProxy *proxy, 
  const char* propertyname, double val, int index = 0)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    proxy->GetProperty(propertyname));
  if (!dvp)
    {
    return 0;
    }
  return dvp->SetElement(index, val);
}

//-----------------------------------------------------------------------------
vtkPVExponentialKeyFrame::vtkPVExponentialKeyFrame()
{
  this->SetKeyFrameProxyXMLName("ExponentialKeyFrame");
  this->DetermineKeyFrameProxyName();
  this->BaseLabel = vtkKWLabel::New();
  this->StartPowerLabel = vtkKWLabel::New();
  this->EndPowerLabel = vtkKWLabel::New();
  this->BaseThumbWheel = vtkKWThumbWheel::New();
  this->StartPowerThumbWheel = vtkKWThumbWheel::New();
  this->EndPowerThumbWheel = vtkKWThumbWheel::New();
}

//-----------------------------------------------------------------------------
vtkPVExponentialKeyFrame::~vtkPVExponentialKeyFrame()
{
  this->BaseThumbWheel->Delete();
  this->StartPowerThumbWheel->Delete();
  this->EndPowerThumbWheel->Delete();
  this->BaseLabel->Delete();
  this->StartPowerLabel->Delete();
  this->EndPowerLabel->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::ChildCreate(vtkKWApplication* app)
{
  this->Superclass::ChildCreate(app);

  this->BaseLabel->SetParent(this);
  this->BaseLabel->Create(app);
  this->BaseLabel->SetText("Base:");

  this->BaseThumbWheel->SetParent(this);
  this->BaseThumbWheel->PopupModeOn();
  this->BaseThumbWheel->SetValue(0.0);
  this->BaseThumbWheel->SetResolution(0.01);
  this->BaseThumbWheel->Create(app);
  this->BaseThumbWheel->DisplayEntryOn();
  this->BaseThumbWheel->DisplayLabelOff();
  this->BaseThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->BaseThumbWheel->ExpandEntryOn();
  this->BaseThumbWheel->GetEntry()->BindCommand(this, "BaseChangedCallback");
  this->BaseThumbWheel->SetEndCommand(this, "BaseChangedCallback");
  this->BaseThumbWheel->SetEntryCommand(this, "BaseChangedCallback");

  this->StartPowerLabel->SetParent(this);
  this->StartPowerLabel->Create(app);
  this->StartPowerLabel->SetText("Start Power:");

  this->StartPowerThumbWheel->SetParent(this);
  this->StartPowerThumbWheel->PopupModeOn();
  this->StartPowerThumbWheel->SetValue(0.0);
  this->StartPowerThumbWheel->SetResolution(0.01);
  this->StartPowerThumbWheel->Create(app);
  this->StartPowerThumbWheel->DisplayEntryOn();
  this->StartPowerThumbWheel->DisplayLabelOff();
  this->StartPowerThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->StartPowerThumbWheel->ExpandEntryOn();
  this->StartPowerThumbWheel->GetEntry()->BindCommand(this, "StartPowerChangedCallback");
  this->StartPowerThumbWheel->SetEndCommand(this, "StartPowerChangedCallback");
  this->StartPowerThumbWheel->SetEntryCommand(this, "StartPowerChangedCallback");

  this->EndPowerLabel->SetParent(this);
  this->EndPowerLabel->Create(app);
  this->EndPowerLabel->SetText("End Power:");

  this->EndPowerThumbWheel->SetParent(this);
  this->EndPowerThumbWheel->PopupModeOn();
  this->EndPowerThumbWheel->SetValue(0.0);
  this->EndPowerThumbWheel->SetResolution(0.01);
  this->EndPowerThumbWheel->Create(app);
  this->EndPowerThumbWheel->DisplayEntryOn();
  this->EndPowerThumbWheel->DisplayLabelOff();
  this->EndPowerThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->EndPowerThumbWheel->ExpandEntryOn();
  this->EndPowerThumbWheel->GetEntry()->BindCommand(this, "EndPowerChangedCallback");
  this->EndPowerThumbWheel->SetEndCommand(this, "EndPowerChangedCallback");
  this->EndPowerThumbWheel->SetEntryCommand(this, "EndPowerChangedCallback");

  this->Script("grid %s %s -sticky w",
    this->BaseLabel->GetWidgetName(),
    this->BaseThumbWheel->GetWidgetName());
  this->Script("grid %s %s -sticky w",
    this->StartPowerLabel->GetWidgetName(),
    this->StartPowerThumbWheel->GetWidgetName());
  this->Script("grid %s %s -sticky w",
    this->EndPowerLabel->GetWidgetName(),
    this->EndPowerThumbWheel->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::BaseChangedCallback()
{
  this->SetBaseWithTrace(
    this->BaseThumbWheel->GetEntry()->GetValueAsFloat());
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::EndPowerChangedCallback()
{
  this->SetEndPowerWithTrace(
    this->EndPowerThumbWheel->GetEntry()->GetValueAsFloat());
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::StartPowerChangedCallback()
{
  this->SetStartPowerWithTrace(
    this->StartPowerThumbWheel->GetEntry()->GetValueAsFloat());
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::SetBaseWithTrace(double base)
{
  this->SetBase(base);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetBaseWithTrace %f", 
    this->GetTclName(), base);
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::SetBase(double base)
{
  DoubleVectPropertySetElement(this->KeyFrameProxy, "Base", base);
  this->KeyFrameProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
double vtkPVExponentialKeyFrame::GetBase()
{
  return vtkSMExponentialKeyFrameProxy::SafeDownCast(this->KeyFrameProxy)->
    GetBase();
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::SetStartPowerWithTrace(double p)
{
  this->SetStartPower(p);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetStartPowerWithTrace %f", 
    this->GetTclName(), p);
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::SetStartPower(double p)
{ 
  DoubleVectPropertySetElement(this->KeyFrameProxy, "StartPower", p);
  this->KeyFrameProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
double vtkPVExponentialKeyFrame::GetStartPower()
{
  return vtkSMExponentialKeyFrameProxy::SafeDownCast(this->KeyFrameProxy)->
    GetStartPower();
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::SetEndPowerWithTrace(double p)
{
  this->SetEndPower(p);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetEndPowerWithTrace %f", 
    this->GetTclName(), p);
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::SetEndPower(double p)
{
  DoubleVectPropertySetElement(this->KeyFrameProxy, "EndPower", p);
  this->KeyFrameProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
double vtkPVExponentialKeyFrame::GetEndPower()
{
  return vtkSMExponentialKeyFrameProxy::SafeDownCast(this->KeyFrameProxy)->
    GetEndPower();
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::UpdateValuesFromProxy()
{
  this->Superclass::UpdateValuesFromProxy();
  vtkSMExponentialKeyFrameProxy* proxy = vtkSMExponentialKeyFrameProxy::
    SafeDownCast(this->KeyFrameProxy);
  this->BaseThumbWheel->SetValue(proxy->GetBase());
  this->StartPowerThumbWheel->SetValue(proxy->GetStartPower());
  this->EndPowerThumbWheel->SetValue(proxy->GetEndPower());
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  this->PropagateEnableState(this->BaseThumbWheel);
  this->PropagateEnableState(this->StartPowerThumbWheel);
  this->PropagateEnableState(this->EndPowerThumbWheel);
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::SaveState(ofstream* file)
{
  this->Superclass::SaveState(file);
  *file << "$kw(" << this->GetTclName() << ") SetBase "
    << this->GetBase() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetStartPower "
    << this->GetStartPower() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetEndPower "
    << this->GetEndPower() << endl;
}

//-----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
