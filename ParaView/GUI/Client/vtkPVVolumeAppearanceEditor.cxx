/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVolumeAppearanceEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVVolumeAppearanceEditor.h"
 
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkKWColorTransferFunctionEditor.h"
#include "vtkKWEvent.h"
#include "vtkKWPiecewiseFunctionEditor.h"
#include "vtkKWPushButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVVolumePropertyWidget.h"
#include "vtkPVWindow.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkVolumeProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVVolumeAppearanceEditor);
vtkCxxRevisionMacro(vtkPVVolumeAppearanceEditor, "1.39");

class vtkPVVolumeAppearanceEditorObserver : public vtkCommand
{
public:
  static vtkPVVolumeAppearanceEditorObserver* New() 
    {return new vtkPVVolumeAppearanceEditorObserver;};

  vtkPVVolumeAppearanceEditorObserver()
    {
    this->PVVolumeAppearanceEditor = 0;
    }

  virtual void Execute(vtkObject* , unsigned long event, void* )
    {
      if ( this->PVVolumeAppearanceEditor )
        {
        switch(event)
          {
          case vtkKWEvent::VolumePropertyChangedEvent:
            this->PVVolumeAppearanceEditor->VolumePropertyChangedCallback();
            this->PVVolumeAppearanceEditor->RenderView();
            break;
          case vtkKWEvent::VolumePropertyChangingEvent:
            this->PVVolumeAppearanceEditor->VolumePropertyChangingCallback();
            // We don't call EventuallyRender since that leads to 
            // non-smooth movement.
            this->PVVolumeAppearanceEditor->PVRenderView->
              GetPVWindow()->GetInteractor()->Render();
            break;
          }
        }
    }

  vtkPVVolumeAppearanceEditor* PVVolumeAppearanceEditor;
};

//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::vtkPVVolumeAppearanceEditor()
{
  this->PVRenderView                 = NULL;

  // Don't actually create any of the widgets 
  // This allows a subclass to replace this one

  this->BackButton                   = NULL;
  this->PVSource                     = NULL;
  this->ArrayInfo                    = NULL;

  this->VolumePropertyWidget         = NULL;
  this->InternalVolumeProperty       = NULL;
  this->VolumeAppearanceObserver = vtkPVVolumeAppearanceEditorObserver::New();
  this->VolumeAppearanceObserver->PVVolumeAppearanceEditor = this;
}

//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::~vtkPVVolumeAppearanceEditor()
{
  if (this->VolumeAppearanceObserver)
    {
    this->VolumeAppearanceObserver->Delete();
    this->VolumeAppearanceObserver = NULL;
    }
  if ( this->BackButton )
    {
    this->BackButton->Delete();
    this->BackButton = NULL;
    }

  if ( this->VolumePropertyWidget )
    {
    this->VolumePropertyWidget->Delete();
    this->VolumePropertyWidget = NULL;
    }

  if (this->InternalVolumeProperty)
    {
    this->InternalVolumeProperty->Delete();
    this->InternalVolumeProperty = NULL;
    }

  this->SetPVRenderView(NULL);
}
//----------------------------------------------------------------------------
// No register count because of reference loop.
void vtkPVVolumeAppearanceEditor::SetPVRenderView(vtkPVRenderView *rv)
{
  this->PVRenderView = rv;
}
//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  // Superclass create takes a KWApplication, but we need a PVApplication.

  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  if (pvApp == NULL)
    {
    vtkErrorMacro("Need a PV application");
    return;
    }
 
  // Back button
  this->BackButton = vtkKWPushButton::New();
  this->BackButton->SetParent(this);
  this->BackButton->Create(this->GetApplication());
  this->BackButton->SetText("Back");
  this->BackButton->SetCommand(this, "BackButtonCallback");


  this->VolumePropertyWidget = vtkPVVolumePropertyWidget::New();
  this->VolumePropertyWidget->SetParent(this);
  this->VolumePropertyWidget->ComponentSelectionVisibilityOff();
  this->VolumePropertyWidget->InterpolationTypeVisibilityOff();
  this->VolumePropertyWidget->MaterialPropertyVisibilityOff();
  this->VolumePropertyWidget->GradientOpacityFunctionVisibilityOff();
  this->VolumePropertyWidget->ComponentWeightsVisibilityOff();
  this->VolumePropertyWidget->GetScalarOpacityFunctionEditor()->WindowLevelModeButtonVisibilityOff();
  this->VolumePropertyWidget->Create(pvApp);
  this->VolumePropertyWidget->AddObserver(
    vtkKWEvent::VolumePropertyChangedEvent, this->VolumeAppearanceObserver);
  this->VolumePropertyWidget->AddObserver(
    vtkKWEvent::VolumePropertyChangingEvent, this->VolumeAppearanceObserver);

  this->Script("pack %s %s -side top -anchor n -fill x -padx 2 -pady 2", 
               this->VolumePropertyWidget->GetWidgetName(),
               this->BackButton->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::VolumePropertyInternalCallback()
{
  vtkPVApplication *pvApp = NULL;

  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());
    }

  vtkSMDataObjectDisplayProxy* pDisp  = this->PVSource->GetDisplayProxy();

  // Scalar Opacity (vtkPiecewiseFunction)
  vtkKWPiecewiseFunctionEditor *kwfunc =
    this->VolumePropertyWidget->GetScalarOpacityFunctionEditor();
  vtkPiecewiseFunction *func = kwfunc->GetPiecewiseFunction();
  double *points = func->GetDataPointer();

  // Unit distance:
  vtkKWScaleWithEntry* scale =
    this->VolumePropertyWidget->GetScalarOpacityUnitDistanceScale();
  double unitDistance = scale->GetValue();

  // Color Ramp (vtkColorTransferFunction)
  vtkKWColorTransferFunctionEditor *kwcolor =
    this->VolumePropertyWidget->GetScalarColorFunctionEditor();
  vtkColorTransferFunction* color = kwcolor->GetColorTransferFunction();
  double *rgb = color->GetDataPointer();

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("Points"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Points on DisplayProxy.");
    return;
    }
    
  // 1. ScalarOpacity
  this->GetTraceHelper()->AddEntry("$kw(%s) RemoveAllScalarOpacityPoints",
    this->GetTclName());
  for(int j=0; j<func->GetSize(); j++)
    {
    // we don't directly call the AppendScalarOpacityPoint method, since 
    // it's slow (as it calls UpdateVTKObjects for each point.
    this->GetTraceHelper()->AddEntry("$kw(%s) AppendScalarOpacityPoint %f %f", this->GetTclName(), 
      points[2*j], points[2*j+1]);
    }
  dvp->SetNumberOfElements(func->GetSize()*2);
  dvp->SetElements(points);


  //2. Color Ramp, similar to ScalarOpacity
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("RGBPoints"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property RGBPoints on DisplayProxy.");
    return;
    }

  this->GetTraceHelper()->AddEntry("$kw(%s) RemoveAllColorPoints",
    this->GetTclName());

  for(int k=0; k<color->GetSize(); k++)
    {
    this->GetTraceHelper()->AddEntry("$kw(%s) AppendColorPoint %f %f %f %f",
      this->GetTclName(),
      rgb[4*k], rgb[4*k+1], rgb[4*k+2], rgb[4*k+3]);
    }
  dvp->SetNumberOfElements(color->GetSize()*4);
  dvp->SetElements(rgb);

  //3. ScalarOpacityUnitDistance
  this->SetScalarOpacityUnitDistance( unitDistance );

  //4. HSVWrap
  this->SetHSVWrap( color->GetHSVWrap() );

  //5. ColorSpace.
  this->SetColorSpace( color->GetColorSpace() );
  pDisp->UpdateVTKObjects();
  this->GetTraceHelper()->AddEntry("$kw(%s) RefreshGUI", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::RefreshGUI()
{
  this->UpdateFromProxy();
  this->VolumePropertyWidget->Update();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SetColorSpace(int w)
{
  if ( !this->PVSource)
    {
    return;
    }

  // Save trace 
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  this->GetTraceHelper()->AddEntry("$kw(%s) SetColorSpace %d", 
    this->GetTclName(), w );

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("ColorSpace"));

  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ColorSpace on "
      "DisplayProxy.");
    return;
    }
  ivp->SetElement(0, w);
  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SetHSVWrap(int w)
{
  if ( !this->PVSource)
    {
    return;
    }

  // Save trace 
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  this->GetTraceHelper()->AddEntry("$kw(%s) SetHSVWrap %d", 
    this->GetTclName(), w );

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("HSVWrap"));

  if (!ivp)
    {
    vtkErrorMacro("Failed to find property HSVWrap on "
      "DisplayProxy.");
    return;
    }
  ivp->SetElement(0, w);
  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::VolumePropertyChangedCallback()
{
  this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOn();
  this->VolumePropertyInternalCallback();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::VolumePropertyChangingCallback()
{
  //this->PVRenderView->SetRenderModeToInteractive();
  this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOff();
  this->VolumePropertyInternalCallback();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::BackButtonCallback()
{
  if (this->PVRenderView == NULL)
    {
    return;
    }

  // This has a side effect of gathering and display information.
  this->PVRenderView->GetPVWindow()->GetCurrentPVSource()->GetPVOutput()->Update();
 
  // Use the Callback version of the method to get the trace
  this->PVRenderView->GetPVWindow()->ShowCurrentSourcePropertiesCallback();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::RenderView()
{
  if (this->PVRenderView)
    {
    this->PVRenderView->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::Close()
{
  this->VolumePropertyWidget->SetVolumeProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SetPVSourceAndArrayInfo(vtkPVSource *source,
                                                          vtkPVArrayInformation *arrayInfo)
{
  this->PVSource = source;
  this->ArrayInfo = arrayInfo;

  vtkPVApplication *pvApp = NULL;

  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());
    }


  vtkPVDataInformation* dataInfo = source->GetDataInformation();
  if ( !this->PVSource || !this->ArrayInfo || !pvApp || !dataInfo ||
    this->PVSource->GetNumberOfParts() <= 0 )
    {
    return;
    }

  // Create the VolumeProperty/OpacityFunction/ColorTransferFuntion
  // that will be manipulated by the widget.

  this->VolumePropertyWidget->SetDataInformation(dataInfo);
  this->VolumePropertyWidget->SetArrayName(this->ArrayInfo->GetName());
  if (this->PVSource->GetDisplayProxy()->GetScalarModeCM() == 
    vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA)
    {
    this->VolumePropertyWidget->SetScalarModeToUsePointFieldData();
    }
  else
    {
    this->VolumePropertyWidget->SetScalarModeToUseCellFieldData();
    }

  if (!this->InternalVolumeProperty)
    {
    this->InternalVolumeProperty = vtkVolumeProperty::New();

    vtkPiecewiseFunction* opacityFunc = vtkPiecewiseFunction::New();
    vtkColorTransferFunction* colorFunc = vtkColorTransferFunction::New();
    // Add some default points to the transfer functions.
    // These don't matter really, we just add them so that 
    // they don't flash errors until, they are populated with correct
    // values.
    opacityFunc->AddPoint(0,0);
    opacityFunc->AddPoint(1,1);
    colorFunc->AddRGBPoint(0, 1, 1, 1);
    colorFunc->AddRGBPoint(1, 1, 1, 1);
    this->InternalVolumeProperty->SetScalarOpacity(opacityFunc);
    this->InternalVolumeProperty->SetColor(colorFunc);
    opacityFunc->Delete();
    colorFunc->Delete();

    this->VolumePropertyWidget->SetVolumeProperty(
      this->InternalVolumeProperty); 
    }
  this->RefreshGUI();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::UpdateFromProxy()
{
  unsigned int i;
  vtkPiecewiseFunction* opacityFunc = 
    this->InternalVolumeProperty->GetScalarOpacity();
  vtkColorTransferFunction* colorFunc =
    this->InternalVolumeProperty->GetRGBTransferFunction();
  
  colorFunc->RemoveAllPoints();
  opacityFunc->RemoveAllPoints();
  
  
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  vtkSMDoubleVectorProperty* dvp ;
  vtkSMIntVectorProperty* ivp;
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("Points")); // OpacityFunction:Points.
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Points on DisplayProxy.");
    return;
    }
  
  for (i=0; (i + 1) < dvp->GetNumberOfElements(); i+=2)
    {
    opacityFunc->AddPoint(dvp->GetElement(i), dvp->GetElement(i+1));
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("RGBPoints"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property RGBPoints on DisplayProxy.");
    return;
    }

  for (i=0; (i+3) < dvp->GetNumberOfElements(); i+=4)
    {
    colorFunc->AddRGBPoint(dvp->GetElement(i),
      dvp->GetElement(i+1), dvp->GetElement(i+2), dvp->GetElement(i+3));
    }
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("ColorSpace"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ColorSpace on DisplayProxy.");
    return;
    }
  colorFunc->SetColorSpace(ivp->GetElement(0));

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("HSVWrap"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property HSVWrap on DisplayProxy.");
    return;
    }
  colorFunc->SetHSVWrap(ivp->GetElement(0));

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("ScalarOpacityUnitDistance"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property ScalarOpacityUnitDistance on DisplayProxy.");
    return;
    }

  this->InternalVolumeProperty->SetScalarOpacityUnitDistance(dvp->GetElement(0));
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->BackButton);
  this->PropagateEnableState(this->VolumePropertyWidget);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SetScalarOpacityUnitDistance(double d)
{
  if ( !this->PVSource && this->ArrayInfo )
    {
    return;
    }

  // Save trace 
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  
  this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarOpacityUnitDistance %f", 
    this->GetTclName(), d );
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("ScalarOpacityUnitDistance"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property ScalarOpacityUnitDistance on "
      "DisplayProxy.");
    return;
    }
  dvp->SetElement(0, d);
  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::AppendColorPoint(double s, double r, 
  double g, double b)
{
  if ( !this->PVSource)
    {
    vtkErrorMacro("PVSource not set!");
    return ;
    }

  // Save trace:
  this->GetTraceHelper()->AddEntry("$kw(%s) AppendColorPoint %f %f %f %f", 
    this->GetTclName(), s, r, g, b);

  vtkSMDataObjectDisplayProxy *pDisp = this->PVSource->GetDisplayProxy();
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("RGBPoints")); 
  int num = dvp->GetNumberOfElements();
  
  dvp->SetNumberOfElements(num+4);
  dvp->SetElement(num, s);
  dvp->SetElement(num+1, r);
  dvp->SetElement(num+2, g);
  dvp->SetElement(num+3, b);

  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::RemoveAllColorPoints()
{
  if ( !this->PVSource)
    {
    vtkErrorMacro("PVSource not set!");
    return ;
    }

  // Save trace:
  vtkSMDataObjectDisplayProxy *pDisp = this->PVSource->GetDisplayProxy();
  this->GetTraceHelper()->AddEntry("$kw(%s) RemoveAllColorPoints ",
    this->GetTclName());

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("RGBPoints")); 
  dvp->SetNumberOfElements(0);
  pDisp->UpdateVTKObjects(); 
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::RemoveAllScalarOpacityPoints()
{
  if (!this->PVSource)
    {
    vtkErrorMacro("Source not set!");
    return;
    }

  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  // Save trace:
  this->GetTraceHelper()->AddEntry("$kw(%s) RemoveAllScalarOpacityPoints ", 
    this->GetTclName());

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("Points")); 
  dvp->SetNumberOfElements(0);
  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::AppendScalarOpacityPoint(double scalar,
  double opacity)
{
  if (!this->PVSource)
    {
    vtkErrorMacro("Source not set!");
    return;
    }
  
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  // Save trace:
  this->GetTraceHelper()->AddEntry("$kw(%s) AppendScalarOpacityPoint %f %f", this->GetTclName(), 
    scalar, opacity);
  
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("Points")); 
  int num = dvp->GetNumberOfElements();
  dvp->SetNumberOfElements(num+2);
  dvp->SetElement(num, scalar);
  dvp->SetElement(num+1, opacity);
  pDisp->UpdateVTKObjects();

}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SaveState(ofstream *file)
{
  vtkPVApplication *pvApp = NULL;

  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());
    }

  if ( !this->PVSource || !this->ArrayInfo || !pvApp )
    {
    return;
    }
  *file << "set kw(" << this->GetTclName() << ") [$kw("
    << this->PVRenderView->GetPVWindow()->GetTclName()
    << ") GetVolumeAppearanceEditor]" << endl;

  // this is handled in vtkPVDisplayGUI for each individual source:
  //    *file << "[$kw(" << this->PVSource->GetTclName() << ") GetPVOutput] "
  //      << "VolumeRenderPointField {" << this->ArrayInfo->GetName() << "} "
  //      << this->ArrayInfo->GetNumberOfComponents() << endl;

  *file << "[$kw(" << this->PVSource->GetTclName() << ") GetPVOutput] "
    << "ShowVolumeAppearanceEditor" << endl;

  // Scalar Opacity (vtkPiecewiseFunction)
  vtkKWPiecewiseFunctionEditor *kwfunc =
    this->VolumePropertyWidget->GetScalarOpacityFunctionEditor();
  vtkPiecewiseFunction *func = kwfunc->GetPiecewiseFunction();
  double *points = func->GetDataPointer();

  // Unit distance:
  vtkKWScaleWithEntry* scale =
    this->VolumePropertyWidget->GetScalarOpacityUnitDistanceScale();
  double unitDistance = scale->GetValue();

  // Color Ramp (vtkColorTransferFunction)
  vtkKWColorTransferFunctionEditor *kwcolor =
    this->VolumePropertyWidget->GetScalarColorFunctionEditor();
  vtkColorTransferFunction* color = kwcolor->GetColorTransferFunction();
  double *rgb = color->GetDataPointer();

  // 1. ScalarOpacity
  *file << "$kw(" << this->GetTclName() << ") "
    << "RemoveAllScalarOpacityPoints" << endl;

  for(int j=0; j<func->GetSize(); j++)
    {
    // Copy points one by one from the vtkPiecewiseFunction:
    *file << "$kw(" << this->GetTclName() << ") "
      << "AppendScalarOpacityPoint " << points[2*j] << " " << points[2*j+1]
      << endl;
    }

  //2. ScalarOpacityUnitDistance
  *file << "$kw(" << this->GetTclName() << ") "
    << "SetScalarOpacityUnitDistance " << unitDistance
    << endl;

  //3. Color Ramp, similar to ScalarOpacity
  *file << "$kw(" << this->GetTclName() << ") "
    << "RemoveAllColorPoints" << endl;

  for(int k=0; k<color->GetSize(); k++)
    {
    *file << "$kw(" << this->GetTclName() << ") "
      << "AppendColorPoint " << rgb[4*k] << " " << rgb[4*k+1]
      << " " << rgb[4*k+2] << " " << rgb[4*k+3]
      << endl;
    }
  *file << "$kw(" << this->GetTclName() << ") "
    << "SetHSVWrap " << color->GetHSVWrap() << endl;

  *file << "$kw(" << this->GetTclName() << ") " 
    << "SetColorSpace " << color->GetColorSpace() << endl;
}

