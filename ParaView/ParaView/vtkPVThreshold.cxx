/*=========================================================================

  Program:   
  Module:    vtkPVThreshold.cxx
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
#include "vtkPVThreshold.h"

#include "vtkDataSet.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVMinMax.h"
#include "vtkPVWindow.h"
#include "vtkSource.h"
#include "vtkStringList.h"
#include "vtkThreshold.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVThreshold);
vtkCxxRevisionMacro(vtkPVThreshold, "1.48.2.1");

int vtkPVThresholdCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVThreshold::vtkPVThreshold()
{
  this->CommandFunction = vtkPVThresholdCommand;
  
  this->AttributeModeFrame = vtkKWWidget::New();
  this->AttributeModeLabel = vtkKWLabel::New();
  this->AttributeModeMenu = vtkKWOptionMenu::New();
  this->MinMaxScale = vtkPVMinMax::New();
  this->AllScalarsCheck = vtkPVLabeledToggle::New();
  
  this->TraceInitialized = 0;
}

//----------------------------------------------------------------------------
vtkPVThreshold::~vtkPVThreshold()
{
  this->AttributeModeLabel->Delete();
  this->AttributeModeLabel = NULL;
  this->AttributeModeMenu->Delete();
  this->AttributeModeMenu = NULL;
  this->AttributeModeFrame->Delete();
  this->AttributeModeFrame = NULL;
  this->MinMaxScale->Delete();
  this->MinMaxScale = NULL;
  this->AllScalarsCheck->Delete();
  this->AllScalarsCheck = NULL;
}

//----------------------------------------------------------------------------
void vtkPVThreshold::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  float range[2];
  
  this->vtkPVSource::CreateProperties();

  this->AddInputMenu("Input", "PVInput", "vtkDataSet",
                     "Set the input to this filter.",
                     this->GetPVWindow()->GetSourceList("Sources"));
  
  this->AttributeModeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->AttributeModeFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->AttributeModeFrame->GetWidgetName());
  
  this->AttributeModeLabel->SetParent(this->AttributeModeFrame);
  this->AttributeModeLabel->Create(pvApp, "");
  this->AttributeModeLabel->SetLabel("Attribute Mode:");
  this->AttributeModeLabel->SetBalloonHelpString("Select whether to operate on point or cell data");
  this->AttributeModeMenu->SetParent(this->AttributeModeFrame);
  this->AttributeModeMenu->Create(pvApp, "");
  this->AttributeModeMenu->AddEntryWithCommand("Point Data", this,
                                               "ChangeAttributeMode point");
  this->AttributeModeMenu->AddEntryWithCommand("Cell Data", this,
                                               "ChangeAttributeMode cell");
  this->AttributeModeMenu->SetCurrentEntry("Point Data");
  this->AttributeModeMenu->SetBalloonHelpString("Select whether to operate on point or cell data");
  this->Script("pack %s %s -side left",
               this->AttributeModeLabel->GetWidgetName(),
               this->AttributeModeMenu->GetWidgetName());
  
  if (this->GetPVInput()->GetVTKData()->GetPointData()->GetScalars())
    {
    this->GetPVInput()->GetVTKData()->GetPointData()->GetScalars()->GetRange(range);
    }
  else
    {
    range[0] = 0;
    range[1] = 1;
    }
  
  this->MinMaxScale->SetParent(this->GetParameterFrame()->GetFrame());
  this->MinMaxScale->SetObjectVariable(this->GetVTKSourceTclName(), "");
  this->MinMaxScale->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  this->MinMaxScale->SetRange(range[0], range[1]);
  this->MinMaxScale->SetResolution((range[1] - range[0]) / 100.0);
  this->MinMaxScale->SetMinimumLabel("Lower Threshold");
  this->MinMaxScale->SetMaximumLabel("Upper Threshold");
  this->MinMaxScale->SetMinimumHelp("Choose the lower value of the threshold");
  this->MinMaxScale->SetMaximumHelp("Choose the upper value of the threshold");
  this->MinMaxScale->SetSetCommand("ThresholdBetween");
  this->MinMaxScale->SetGetMinCommand("GetLowerThreshold");
  this->MinMaxScale->SetGetMaxCommand("GetUpperThreshold");
  pvApp->BroadcastScript("%s ThresholdBetween %f %f",
                         this->GetVTKSourceTclName(), range[0], range[1]);
  this->MinMaxScale->Create(pvApp);
  this->AddPVWidget(this->MinMaxScale);
  
  this->AllScalarsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->AllScalarsCheck->SetLabel("AllScalars");
  this->AllScalarsCheck->SetObjectVariable(this->GetVTKSourceTclName(), 
                                           "AllScalars");
  this->AllScalarsCheck->SetModifiedCommand(this->GetTclName(), 
                                           "SetAcceptButtonColorToRed");
  this->AllScalarsCheck->SetBalloonHelpString("If AllScalars is checked, then a cell is only included if all its points are within the threshold. This is only relevant for point data.");
  this->AllScalarsCheck->Create(pvApp);
  this->AddPVWidget(this->AllScalarsCheck);
  this->AllScalarsCheck->SetState(1);

  this->Script("pack %s %s -fill x -expand t",
               this->MinMaxScale->GetWidgetName(),
               this->AllScalarsCheck->GetWidgetName());

  this->UpdateParameterWidgets();
}

//----------------------------------------------------------------------------
void vtkPVThreshold::UpdateParameterWidgets()
{
  this->vtkPVSource::UpdateParameterWidgets();
  this->UpdateMinMaxScale();
}

//----------------------------------------------------------------------------
void vtkPVThreshold::UpdateMinMaxScale()
{
  const char *mode;
  float range[2];
  range[0] = 0;
  range[1] = 1;

  vtkPVData* input = this->GetPVInput();
  if (input)
    {
    vtkDataSet* thresholdInput = input->GetVTKData();
    vtkDataArray* scalars=0;
    int pointDataFlag;
    
    mode = this->AttributeModeMenu->GetValue();
    if (mode && strcmp(mode, "Point Data") == 0)
      {
      scalars = thresholdInput->GetPointData()->GetScalars();
      pointDataFlag=1;
      }
    else if (mode && strcmp(mode, "Cell Data") == 0)
      {
      scalars = thresholdInput->GetCellData()->GetScalars();
      pointDataFlag=0;
      }
    
    if (scalars)
      {
      const char* name = scalars->GetName();
      if (name)
        {
        input->GetArrayComponentRange(range, pointDataFlag, name, 0);
        }
      else
        {
        scalars->GetRange(range, 0);
        }
      }
    }

  this->MinMaxScale->SetResolution((range[1] - range[0]) / 100.0);
  this->MinMaxScale->SetRange(range[0], range[1]);
  this->MinMaxScale->Reset();
}

//----------------------------------------------------------------------------
void vtkPVThreshold::ChangeAttributeMode(const char* newMode)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if ( ! this->TraceInitialized)
    {
    pvApp->AddTraceEntry("set kw(%s) [$kw(%s) GetAttributeModeMenu]",
                         this->AttributeModeMenu->GetTclName(),
                         this->GetTclName());
    this->TraceInitialized = 1;
    }
  
  pvApp->AddTraceEntry("$kw(%s) SetValue \"%s\"",
                       this->AttributeModeMenu->GetTclName(),
                       this->AttributeModeMenu->GetValue());
  pvApp->AddTraceEntry("$kw(%s) ChangeAttributeMode %s",
                       this->GetTclName(), newMode);
  
  this->SetAcceptButtonColorToRed();
  
  if (strcmp(newMode, "point") == 0)
    {
    ((vtkThreshold*)this->GetVTKSource())->SetAttributeModeToUsePointData();
    pvApp->BroadcastScript("%s SetAttributeModeToUsePointData",
                           this->GetVTKSourceTclName());
    //this->ScalarOperationMenu->UsePointDataOn();
    }
  else if (strcmp(newMode, "cell") == 0)
    {
    ((vtkThreshold*)this->GetVTKSource())->SetAttributeModeToUseCellData();
    pvApp->BroadcastScript("%s SetAttributeModeToUseCellData",
                           this->GetVTKSourceTclName());
    //this->ScalarOperationMenu->UsePointDataOff();
    }
  
  //this->ScalarOperationMenu->FillMenu();
  this->UpdateMinMaxScale();
}

void vtkPVThreshold::UpdateScalars()
{
  float range[2];
  int attributeMode;
  
  //arrayName = this->ScalarOperationMenu->GetValue();
  attributeMode = ((vtkThreshold*)this->GetVTKSource())->GetAttributeMode();
  
  if (attributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
    {
    //dataArray = this->GetPVInput()->GetVTKData()->GetPointData()->GetArray(arrayName);
    //if (dataArray)
    //  {
    //  dataArray->GetRange(range);
    //  }
    }
  else if (attributeMode == VTK_ATTRIBUTE_MODE_USE_CELL_DATA)
    {
    //dataArray = this->GetPVInput()->GetVTKData()->GetCellData()->GetArray(arrayName);
    //if (dataArray)
    //  {
    //  dataArray->GetRange(range);
    //  }
    }
  
  range[0] = 0;
  range[1] = 1;

  this->MinMaxScale->SetResolution((range[1] - range[0]) / 100.0);
  this->MinMaxScale->SetRange(range[0], range[1]);
  this->MinMaxScale->SetMaxValue(range[1]);
  this->MinMaxScale->SetMinValue(range[0]);
}

void vtkPVThreshold::SaveInTclScript(ofstream *file, int interactiveFlag, 
                                     int vtkFlag)
{
  
  if (this->VisitedFlag)
    {
    return;
    }
  this->VisitedFlag = 1;

  this->GetPVInput()->GetPVSource()->SaveInTclScript(file, interactiveFlag,vtkFlag);

  *file << this->VTKSource->GetClassName() << " "
        << this->VTKSourceTclName << "\n";
  
  *file << "\t" << this->VTKSourceTclName << " SetInput [";

  *file << this->GetPVInput()->GetPVSource()->GetVTKSourceTclName()
        << " GetOutput]\n\t";
  
  *file << this->VTKSourceTclName << " SetAttributeModeToUse";
  if (strcmp(this->AttributeModeMenu->GetValue(), "Point Data") == 0)
    {
    *file << "PointData\n\t";
    }
  else
    {
    *file << "CellData\n\t";
    }
  
  this->MinMaxScale->SaveInTclScript(file);
  *file << "\t";
  this->AllScalarsCheck->SaveInTclScript(file);
  *file << "\n";
  
  this->GetPVOutput(0)->SaveInTclScript(file, interactiveFlag, vtkFlag);
}

//----------------------------------------------------------------------------
void vtkPVThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AttributeModeMenu: " << this->GetAttributeModeMenu() << endl;
}
