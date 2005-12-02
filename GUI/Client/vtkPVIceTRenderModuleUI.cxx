/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVIceTRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWScale.h"
#include "vtkPVApplication.h"
#include "vtkPVOptions.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMIceTRenderModuleProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVIceTRenderModuleUI);
vtkCxxRevisionMacro(vtkPVIceTRenderModuleUI, "1.11");

//----------------------------------------------------------------------------
vtkPVIceTRenderModuleUI::vtkPVIceTRenderModuleUI()
{
  this->CompositeOptionEnabled = 1;

  this->CollectLabel = vtkKWLabel::New();
  this->CollectCheck = vtkKWCheckButton::New();
  this->CollectThresholdScale = vtkKWScale::New();
  this->CollectThresholdLabel = vtkKWLabel::New();
  this->CollectThreshold = 100.0;

  this->StillReductionLabel = vtkKWLabel::New();
  this->StillReductionCheck = vtkKWCheckButton::New();
  this->StillReductionFactorScale = vtkKWScale::New();
  this->StillReductionFactorLabel = vtkKWLabel::New();
  this->StillReductionFactor = 0;
}

//----------------------------------------------------------------------------
vtkPVIceTRenderModuleUI::~vtkPVIceTRenderModuleUI()
{
  // Save UI values in registry.
  vtkPVApplication *pvapp = this->GetPVApplication();
  if (pvapp)
    {
    pvapp->SetRegistryValue(2, "RunTime", "CollectThreshold", "%d",
                            this->CollectThreshold);
    pvapp->SetRegistryValue(2, "RunTime", "StillReductionFactor", "%d",
                            this->StillReductionFactor);
    }

  this->CollectLabel->Delete();
  this->CollectLabel = NULL;
  this->CollectCheck->Delete();
  this->CollectCheck = NULL;
  this->CollectThresholdScale->Delete();
  this->CollectThresholdScale = NULL;
  this->CollectThresholdLabel->Delete();
  this->CollectThresholdLabel = NULL;

  this->StillReductionLabel->Delete();
  this->StillReductionLabel = NULL;
  this->StillReductionCheck->Delete();
  this->StillReductionCheck = NULL;
  this->StillReductionFactorScale->Delete();
  this->StillReductionFactorScale = NULL;
  this->StillReductionFactorLabel->Delete();
  this->StillReductionFactorLabel = NULL;
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::Create()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("RenderModuleUI already created");
    return;
    }

  this->Superclass::Create();

  vtkPVApplication *pvapp = 
    vtkPVApplication::SafeDownCast(this->GetApplication());
  // Skip over LOD res and threshold, composite threshold, and subsample rate.
  int row = 10;

  this->StillReductionLabel->SetParent(this->LODScalesFrame);
  this->StillReductionLabel->Create();
  this->StillReductionLabel->SetAnchorToWest();
  this->StillReductionLabel->SetText("Still Subsample Rate:");

  this->StillReductionCheck->SetParent(this->LODScalesFrame);
  this->StillReductionCheck->Create();
  this->StillReductionCheck->SetSelectedState(1);
  this->StillReductionCheck->SetCommand(this, "StillReductionCheckCallback");

  this->StillReductionFactorScale->SetParent(this->LODScalesFrame);
  this->StillReductionFactorScale->Create();
  this->StillReductionFactorScale->SetRange(2, 20);
  this->StillReductionFactorScale->SetResolution(1);
  this->StillReductionFactorScale->SetValue(2);
  this->StillReductionFactorScale->SetCommand(this, "StillReductionFactorScaleCallback");
  this->StillReductionFactorScale->SetBalloonHelpString(
    "Subsampling is a compositing LOD technique. "
    "Still subsampling will use larger pixels during still rendering.");

  this->StillReductionFactorLabel->SetParent(this->LODScalesFrame);
  this->StillReductionFactorLabel->SetText("2 Pixels");
  this->StillReductionFactorLabel->Create();
  this->StillReductionFactorLabel->SetAnchorToWest();
  if (pvapp &&
      pvapp->GetRegistryValue(2, "RunTime", "StillReductionFactor", 0))
    {
    this->SetStillReductionFactor(
      pvapp->GetIntRegistryValue(2, "RunTime", "StillReductionFactor"));
    }
  else
    {
    this->SetStillReductionFactor(1);
    }

  pvapp->Script("grid %s -row %d -column 2 -sticky nws",
                this->StillReductionFactorLabel->GetWidgetName(), row++);
  pvapp->Script("grid %s -row %d -column 0 -sticky nws",
                this->StillReductionLabel->GetWidgetName(), row);
  pvapp->Script("grid %s -row %d -column 1 -sticky nes",
                this->StillReductionCheck->GetWidgetName(), row);
  pvapp->Script("grid %s -row %d -column 2 -sticky news",
                this->StillReductionFactorScale->GetWidgetName(), row++);

  this->CollectLabel->SetParent(this->LODScalesFrame);
  this->CollectLabel->Create();
  this->CollectLabel->SetAnchorToWest();
  this->CollectLabel->SetText("Client Collect:");

  this->CollectCheck->SetParent(this->LODScalesFrame);
  this->CollectCheck->Create();
  this->CollectCheck->SetSelectedState(1);
  this->CollectCheck->SetCommand(this, "CollectCheckCallback");

  this->CollectThresholdScale->SetParent(this->LODScalesFrame);
  this->CollectThresholdScale->Create();
  this->CollectThresholdScale->SetRange(0.0, 1000.0);
  this->CollectThresholdScale->SetResolution(10.0);
  this->CollectThresholdScale->SetValue(this->CollectThreshold);
  this->CollectThresholdScale->SetEndCommand(this,
                                             "CollectThresholdScaleCallback");
  this->CollectThresholdScale->SetCommand(this,
                                          "CollectThresholdLabelCallback");
  this->CollectThresholdScale->SetBalloonHelpString(
    "This slider determines when any geometry is collected on the client."
    "If geometry is not collected on the client, the outline is drawn on"
    "the client (but the tile display still shows the geometry)."
    "Left: Never collect any geometry on the client."
    "Right: Collect larger geometry on client.");

  this->CollectThresholdLabel->SetParent(this->LODScalesFrame);
  this->CollectThresholdLabel->Create();
  this->CollectThresholdLabel->SetAnchorToWest();
  if (pvapp->GetRegistryValue(2, "RunTime", "CollectThreshold", 0))
    {
    this->CollectThreshold
      = pvapp->GetFloatRegistryValue(2, "RunTime", "CollectThreshold");
    }

  // Force the set.
  float tmp = this->CollectThreshold;
  this->CollectThreshold = -1.0;
  this->SetCollectThreshold(tmp);

  this->Script("grid %s -row %d -column 2 -sticky nws", 
               this->CollectThresholdLabel->GetWidgetName(), row++);
  this->Script("grid %s -row %d -column 0 -sticky nws", 
               this->CollectLabel->GetWidgetName(), row);
  this->Script("grid %s -row %d -column 1 -sticky nes", 
               this->CollectCheck->GetWidgetName(), row);
  this->Script("grid %s -row %d -column 2 -sticky news", 
               this->CollectThresholdScale->GetWidgetName(), row++);

  this->Script("grid columnconfigure %s 2 -weight 1",
               this->CollectThresholdScale->GetParent()->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::CollectCheckCallback()
{
  int val = this->CollectCheck->GetSelectedState();

  if (val)
    {
    float threshold = this->CollectThresholdScale->GetValue();
    this->SetCollectThreshold(threshold);
    }
  else
    {
    this->SetCollectThreshold(VTK_LARGE_FLOAT);
    }
}

//-----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::CollectThresholdScaleCallback()
{
  float threshold = this->CollectThresholdScale->GetValue();
  this->SetCollectThreshold(threshold);
}

//-----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::CollectThresholdLabelCallback()
{
  float threshold = this->CollectThresholdScale->GetValue();

  if (threshold == VTK_LARGE_FLOAT)
    {
    this->CollectThresholdLabel->SetText("Always Collect");
    }
  else
    {
    char str[256];
    sprintf(str, "Collect below %.0f MBytes", threshold);
    this->CollectThresholdLabel->SetText(str);
    }
}

//-----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::SetCollectThreshold(float threshold)
{
  if (this->CollectThreshold == threshold) return;

  this->CollectThresholdLabelCallback();

  if (threshold == VTK_LARGE_FLOAT)
    {
    this->CollectCheck->SetSelectedState(0);
    this->CollectThresholdScale->EnabledOff();
    this->CollectThresholdLabel->EnabledOff();
    }
  else
    {
    this->CollectCheck->SetSelectedState(1);
    this->CollectThresholdScale->EnabledOn();
    this->CollectThresholdLabel->EnabledOn();
    this->CollectThresholdScale->SetValue(threshold);
    }

  this->CollectThreshold = threshold;

  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
              this->RenderModuleProxy->GetProperty("CollectGeometryThreshold"));
  dvp->SetElement(0, threshold);
  this->RenderModuleProxy->UpdateVTKObjects();

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %f.", threshold);
  
  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->GetTraceHelper()->AddEntry("catch {$kw(%s) SetCollectThreshold %f}",
                                   this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::StillReductionCheckCallback()
{
  int val = this->StillReductionCheck->GetSelectedState();
  if (val)
    {
    val = (int)(this->StillReductionFactorScale->GetValue());
    }
  else
    { // value of 1 is disabled.
    val = 1;
    }
  this->SetStillReductionFactor(val);
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::StillReductionFactorScaleCallback()
{
  int val = (int)(this->StillReductionFactorScale->GetValue());
  this->SetStillReductionFactor(val);
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::SetStillReductionFactor(int factor)
{
  if (this->StillReductionFactor == factor)
    {
    return;
    }

  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->GetTraceHelper()->AddEntry("catch {$kw(%s) SetStillReductionFactor %d}", 
                      this->GetTclName(), factor);
  this->StillReductionFactor = factor;

  if (factor == 1)
    {
    this->StillReductionFactorScale->EnabledOff();
    this->StillReductionFactorLabel->EnabledOff();
    this->StillReductionCheck->SetSelectedState(0);
    this->StillReductionFactorLabel->SetText("Subsampling Disabled"); 
    vtkTimerLog::MarkEvent("--- Still reduction disabled.");
    }
  else
    {
    this->StillReductionFactorScale->EnabledOn();
    this->StillReductionFactorLabel->EnabledOn();
    this->StillReductionFactorScale->SetValue(factor);
    this->StillReductionCheck->SetSelectedState(1);
    char str[128];
    sprintf(str, "%d Pixels", factor);
    this->StillReductionFactorLabel->SetText(str); 
     vtkTimerLog::FormatAndMarkEvent("--- Still reduction factor %d.", factor);
   }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderModuleProxy->GetProperty("StillReductionFactor"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find StillReductionFactor on RenderModuleProxy.");
    return;
    }
  ivp->SetElement(0, factor);
  this->RenderModuleProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "CollectThreshold: " << this->CollectThreshold << endl;
}

