/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProbe.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProbe.h"

#include "vtkIdTypeArray.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPProbeFilter.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkSocketController.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkSystemIncludes.h"
#include "vtkXYPlotActor.h"
#include "vtkXYPlotWidget.h"
#include "vtkPVPlotDisplay.h"
#include "vtkPVRenderModule.h"
#include "vtkCommand.h"

#include <vtkstd/string>
 
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProbe);
vtkCxxRevisionMacro(vtkPVProbe, "1.120");

int vtkPVProbeCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

#define PV_TAG_PROBE_OUTPUT 759362

//===========================================================================
//***************************************************************************
class vtkXYPlotWidgetObserver : public vtkCommand
{
public:
  static vtkXYPlotWidgetObserver *New() 
    {return new vtkXYPlotWidgetObserver;};

  vtkXYPlotWidgetObserver()
    {
      this->PVProbe = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void* calldata)
    {
      if ( this->PVProbe )
        {
        this->PVProbe->ExecuteEvent(wdg, event, calldata);
        }
    }

  vtkPVProbe* PVProbe;
};




//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  static int instanceCount = 0;
  
  this->CommandFunction = vtkPVProbeCommand;

  this->SelectedPointFrame = vtkKWWidget::New();
  this->SelectedPointLabel = vtkKWLabel::New();
  this->PointDataLabel = vtkKWLabel::New();
  
  this->ShowXYPlotToggle = vtkKWCheckButton::New();

  
  this->ProbeFrame = vtkKWWidget::New();
  
  this->XYPlotWidget = 0;
  this->XYPlotObserver = NULL;
  
  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
  
  this->ReplaceInputOff();

  // Special ivar in vtkPVSource just for this subclass.
  // We cannot process inputs that have more than one part.
  this->RequiredNumberOfInputParts = 1;
  
  this->PlotDisplay = vtkPVPlotDisplay::New();
}

vtkPVProbe::~vtkPVProbe()
{  
  if (this->GetPVApplication() && this->GetPVApplication()->GetRenderModule())
    {
    this->GetPVApplication()->GetRenderModule()->RemoveDisplay(this->PlotDisplay);
    }

  this->PlotDisplay->Delete();
  this->PlotDisplay = NULL;

  if ( this->XYPlotWidget )
    {
    this->XYPlotWidget->SetEnabled(0);
    this->XYPlotWidget->SetInteractor(NULL);
    this->XYPlotWidget->Delete();
    this->XYPlotWidget = 0;
    }
  if (this->XYPlotObserver)
    {
    this->XYPlotObserver->Delete();
    this->XYPlotObserver = NULL;
    }
    
  this->SelectedPointLabel->Delete();
  this->SelectedPointLabel = NULL;
  this->SelectedPointFrame->Delete();
  this->SelectedPointFrame = NULL;
  this->PointDataLabel->Delete();
  this->PointDataLabel = NULL;  
  
  this->ShowXYPlotToggle->Delete();
  this->ShowXYPlotToggle = NULL;
  
  this->ProbeFrame->Delete();
  this->ProbeFrame = NULL;  
}

//----------------------------------------------------------------------------
void vtkPVProbe::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  this->PlotDisplay->SetPVApplication(pvApp);

  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  this->vtkPVSource::CreateProperties();
  // We do not support probing groups and multi-block objects. Therefore,
  // we use the first VTKSource id.
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->GetVTKSourceID(0)
                  << "SetSpatialMatch" << 2
                  << vtkClientServerStream::End;
  pm->SendStreamToServer();

  this->ProbeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->ProbeFrame->Create(pvApp, "frame", "");
  
  this->Script("pack %s",
               this->ProbeFrame->GetWidgetName());

  // widgets for points
  this->SelectedPointFrame->SetParent(this->ProbeFrame);
  this->SelectedPointFrame->Create(pvApp, "frame", "");
  
  
  
  this->SelectedPointLabel->SetParent(this->SelectedPointFrame);
  this->SelectedPointLabel->Create(pvApp, "");
  this->SelectedPointLabel->SetLabel("Point");

  this->Script("pack %s -side left",
               this->SelectedPointLabel->GetWidgetName());
  
  this->PointDataLabel->SetParent(this->ProbeFrame);
  this->PointDataLabel->Create(pvApp, "");

  this->ShowXYPlotToggle->SetParent(this->ProbeFrame);
  this->ShowXYPlotToggle->Create(pvApp, "-text \"Show XY-Plot\"");
  this->ShowXYPlotToggle->SetState(1);
  this->Script("%s configure -command {%s SetAcceptButtonColorToRed}",
               this->ShowXYPlotToggle->GetWidgetName(), this->GetTclName());

  this->Script("pack %s",
               this->ShowXYPlotToggle->GetWidgetName());

  if ( !this->XYPlotWidget )
    {
    this->XYPlotWidget = vtkXYPlotWidget::New();
    this->XYPlotWidget->SetXYPlotActor(vtkXYPlotActor::SafeDownCast(
                pm->GetObjectFromID(this->PlotDisplay->GetXYPlotActorID())));

  
    vtkPVGenericRenderWindowInteractor* iren = 
      this->GetPVWindow()->GetInteractor();
    if ( iren )
      {
      this->XYPlotWidget->SetInteractor(iren);
      }

    // This observer synchronizes all processes when
    // the widget changes the plot.
    this->XYPlotObserver = vtkXYPlotWidgetObserver::New();
    this->XYPlotObserver->PVProbe = this;
    this->XYPlotWidget->AddObserver(vtkCommand::InteractionEvent, 
                                    this->XYPlotObserver);
    this->XYPlotWidget->AddObserver(vtkCommand::StartInteractionEvent, 
                                    this->XYPlotObserver);
    this->XYPlotWidget->AddObserver(vtkCommand::EndInteractionEvent, 
                                    this->XYPlotObserver);
    }
}


//----------------------------------------------------------------------------
void vtkPVProbe::ExecuteEvent(vtkObject* vtkNotUsed(wdg), 
                              unsigned long event,  
                              void* vtkNotUsed(calldata))
{
  switch ( event )
    {
    case vtkCommand::StartInteractionEvent:
      //this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOn();
      break;
    case vtkCommand::EndInteractionEvent:
      //this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOff();
      //this->RenderView();
      vtkXYPlotActor* xypa = this->XYPlotWidget->GetXYPlotActor();
      double *pos1 = xypa->GetPositionCoordinate()->GetValue();
      double *pos2 = xypa->GetPosition2Coordinate()->GetValue();
      //this->AddTraceEntry("$kw(%s) SetScalarBarPosition1 %lf %lf", 
      //                    this->GetTclName(), pos1[0], pos1[1]);
      //this->AddTraceEntry("$kw(%s) SetScalarBarPosition2 %lf %lf", 
      //                    this->GetTclName(), pos2[0], pos2[1]);
      //this->AddTraceEntry("$kw(%s) SetScalarBarOrientation %d",
      //                    this->GetTclName(), sact->GetOrientation());

      // Synchronize the server scalar bar.
      vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->PlotDisplay->GetXYPlotActorID() 
                      << "GetPositionCoordinate" 
                      << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << vtkClientServerStream::LastResult 
                      << "SetValue" << pos1[0] << pos1[1]
                      << vtkClientServerStream::End;

      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->PlotDisplay->GetXYPlotActorID() 
                      << "GetPosition2Coordinate" 
                      << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << vtkClientServerStream::LastResult 
                      << "SetValue" << pos2[0] << pos2[1]
                      << vtkClientServerStream::End;
      pm->SendStreamToRenderServer();

      break;
    }
}


//----------------------------------------------------------------------------
void vtkPVProbe::AcceptCallbackInternal()
{
  this->AddTraceEntry("[$kw(%s) GetShowXYPlotToggle] SetState %d",
                      this->GetTclName(),
                      this->ShowXYPlotToggle->GetState());
  
  // call the superclass's method
  this->vtkPVSource::AcceptCallbackInternal();
    
  if (this->PlotDisplay->GetPart() == NULL)
    {
    // Connect to the display.
    // These should be merged.
    this->PlotDisplay->SetPart(this->GetPart(0));
    this->PlotDisplay->SetInput(this->GetPart(0));
    this->GetPart(0)->AddDisplay(this->PlotDisplay);
    this->GetPVApplication()->GetRenderModule()->AddDisplay(this->PlotDisplay);
    }

  // We need to update manually for the case we are probing one point.
  this->PlotDisplay->Update();
  int numPts = this->GetDataInformation()->GetNumberOfPoints();

  if (numPts == 1)
    { // Put the array information in the UI. 
    // Get the collected data from the display.
    vtkPolyData* d = this->PlotDisplay->GetCollectedData();
    vtkPointData* pd = d->GetPointData();
  
    // update the ui to see the point data for the probed point
    vtkIdType j, numComponents;

    // use vtkstd::string since 'label' can grow in length arbitrarily
    vtkstd::string label;
    vtkstd::string arrayData;
    vtkstd::string tempArray;

    this->XYPlotWidget->SetEnabled(0);

    int numArrays = pd->GetNumberOfArrays();
    for (int i = 0; i < numArrays; i++)
      {
      vtkDataArray* array = pd->GetArray(i);
      if (array->GetName())
        {
        numComponents = array->GetNumberOfComponents();
        if (numComponents > 1)
          {
          // make sure we fill buffer from the beginning
          ostrstream arrayStrm;
          arrayStrm << array->GetName() << ": ( " << ends;
          arrayData = arrayStrm.str();
          arrayStrm.rdbuf()->freeze(0);

          for (j = 0; j < numComponents; j++)
            {
            // make sure we fill buffer from the beginning
            ostrstream tempStrm;
            tempStrm << array->GetComponent( 0, j ) << ends; 
            tempArray = tempStrm.str();
            tempStrm.rdbuf()->freeze(0);

            if (j < numComponents - 1)
              {
              tempArray += ",";
              if (j % 3 == 2)
                {
                tempArray += "\n\t";
                }
              else
                {
                tempArray += " ";
                }
              }
            else
              {
              tempArray += " )\n";
              }
            arrayData += tempArray;
            }
          label += arrayData;
          }
        else
          {
          // make sure we fill buffer from the beginning
          ostrstream arrayStrm;
          arrayStrm << array->GetName() << ": " << array->GetComponent( 0, 0 ) << endl << ends;

          label += arrayStrm.str();
          arrayStrm.rdbuf()->freeze(0);
          }
        }
      }
    this->PointDataLabel->SetLabel( label.c_str() );
    this->Script("pack %s", this->PointDataLabel->GetWidgetName());
    }
  else
    {
    this->PointDataLabel->SetLabel("");
    this->Script("pack forget %s", this->PointDataLabel->GetWidgetName());
    }

  if (this->ShowXYPlotToggle->GetState() && numPts > 1)
    {
    vtkPVRenderModule* rm = this->GetPVApplication()->GetRenderModule();
    this->XYPlotWidget->SetCurrentRenderer(rm->GetRenderer2D());
    this->GetPVRenderView()->Enable3DWidget(this->XYPlotWidget);

    // Enable XYPlotActor on server for tiled display.
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << rm->GetRenderer2DID()
                    << "AddActor"
                    << this->PlotDisplay->GetXYPlotActorID() 
                    << vtkClientServerStream::End;
    pm->SendStreamToRenderServer();
    }
  else
    {
    this->XYPlotWidget->SetEnabled(0);
    }
    
}
 
//----------------------------------------------------------------------------
void vtkPVProbe::Deselect(int doPackForget)
{
  this->vtkPVSource::Deselect(doPackForget);
}

//----------------------------------------------------------------------------
int vtkPVProbe::GetDimensionality()
{
  if (!this->GetVTKSourceID(0).ID)
    {
    return 0;
    }
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->GetVTKSourceID(0)
                  << "GetInput" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "GetSource"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "GetClassName"
                  << vtkClientServerStream::End;
  pm->SendStreamToClient();
  const char* name = 0;
  pm->GetLastClientResult().GetArgument(0,0,&name);
  if ( name && vtkString::Equals(name, "vtkLineSource") )
    {
    return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVProbe::HSVtoRGB(float h, float s, float v, float *r, float *g, float *b)
{
  float R, G, B;
  float max = 1.0;
  float third = max / 3.0;
  float temp;

  // compute rgb assuming S = 1.0;
  if (h >= 0.0 && h <= third) // red -> green
    {
    G = h/third;
    R = 1.0 - G;
    B = 0.0;
    }
  else if (h >= third && h <= 2.0*third) // green -> blue
    {
    B = (h - third)/third;
    G = 1.0 - B;
    R = 0.0;
    }
  else // blue -> red
    {
    R = (h - 2.0 * third)/third;
    B = 1.0 - R;
    G = 0.0;
    }
        
  // add Saturation to the equation.
  s = s / max;
  //R = S + (1.0 - S)*R;
  //G = S + (1.0 - S)*G;
  //B = S + (1.0 - S)*B;
  // what happend to this?
  R = s*R + (1.0 - s);
  G = s*G + (1.0 - s);
  B = s*B + (1.0 - s);
      
  // Use value to get actual RGB 
  // normalize RGB first then apply value
  temp = R + G + B; 
  //V = 3 * V / (temp * max);
  // and what happend to this?
  v = 3 * v / (temp);
  R = R * v;
  G = G * v;
  B = B * v;
      
  // clip below 255
  //if (R > 255.0) R = max;
  //if (G > 255.0) G = max;
  //if (B > 255.0) B = max;
  // mixed constant 255 and max ?????
  if (R > max)
    {
    R = max;
    }
  if (G > max)
    {
    G = max;
    }
  if (B > max)
    {
    B = max;
    }
  *r = R;
  *g = G;
  *b = B;
}
//----------------------------------------------------------------------------
void vtkPVProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShowXYPlotToggle: " << this->GetShowXYPlotToggle() << endl;
  os << indent << "XYPlotWidget: " << this->XYPlotWidget << endl;
}
