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
 
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkKWRange.h"
#include "vtkKWLabeledFrame.h"
#include "vtkPVArrayInformation.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPVProcessModule.h"
#include "vtkKWLabel.h"
#include "vtkKWScale.h"
#include "vtkKWFrame.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWMenuButton.h"
#include "vtkKWChangeColorButton.h"
#include "vtkColorTransferFunction.h"
#include "vtkKWWidget.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVVolumeAppearanceEditor);
vtkCxxRevisionMacro(vtkPVVolumeAppearanceEditor, "1.4");

int vtkPVVolumeAppearanceEditorCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::vtkPVVolumeAppearanceEditor()
{
  this->CommandFunction              = vtkPVVolumeAppearanceEditorCommand;
  this->PVRenderView                 = NULL;

  // Don't actually create any of the widgets 
  // This allows a subclass to replace this one

  this->ScalarOpacityFrame           = NULL;
  this->ColorFrame                   = NULL;
  this->BackButton                   = NULL;
  
  this->ScalarOpacityRampLabel       = NULL;
  this->ScalarOpacityRampRange       = NULL;  
  this->ScalarOpacityStartValueLabel = NULL;
  this->ScalarOpacityStartValueScale = NULL;
  this->ScalarOpacityEndValueLabel   = NULL;
  this->ScalarOpacityEndValueScale   = NULL;
  
  this->ColorRampLabel               = NULL;
  this->ColorRampRange               = NULL;
  this->ColorEditorFrame                = NULL;
  this->ColorStartValueButton        = NULL;
  this->ColorEndValueButton          = NULL;
  this->ColorMapLabel                = NULL;
  
  this->ScalarRange[0]               = 0.0;
  this->ScalarRange[1]               = 1.0;
  
  this->MapData                      = NULL;
  this->MapDataSize                  = 0;
  this->MapHeight                    = 25;
  this->MapWidth                     = 20;
}

//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::~vtkPVVolumeAppearanceEditor()
{
  if ( this->ScalarOpacityFrame )
    {
    this->ScalarOpacityFrame->Delete();
    this->ScalarOpacityFrame = NULL;
    }

  if ( this->ColorFrame )
    {
    this->ColorFrame->Delete();
    this->ColorFrame = NULL;
    }

  if ( this->BackButton )
    {
    this->BackButton->Delete();
    this->BackButton = NULL;
    }

  if ( this->ScalarOpacityRampLabel )
    {
    this->ScalarOpacityRampLabel->Delete();
    this->ScalarOpacityRampLabel = NULL;
    }
  
  if ( this->ScalarOpacityRampRange )
    {
    this->ScalarOpacityRampRange->Delete();
    this->ScalarOpacityRampRange = NULL;
    }
  
  if ( this->ScalarOpacityStartValueLabel )
    {
    this->ScalarOpacityStartValueLabel->Delete();
    this->ScalarOpacityStartValueLabel = NULL;    
    }

  if ( this->ScalarOpacityStartValueScale )
    {
    this->ScalarOpacityStartValueScale->Delete();
    this->ScalarOpacityStartValueScale = NULL;    
    }

  if ( this->ScalarOpacityEndValueLabel )
    {
    this->ScalarOpacityEndValueLabel->Delete();
    this->ScalarOpacityEndValueLabel = NULL;    
    }

  if ( this->ScalarOpacityEndValueScale )
    {
    this->ScalarOpacityEndValueScale->Delete();
    this->ScalarOpacityEndValueScale = NULL;    
    }

  if ( this->ColorRampLabel )
    {
    this->ColorRampLabel->Delete();
    this->ColorRampLabel = NULL;
    }
  
  if ( this->ColorRampRange )
    {
    this->ColorRampRange->Delete();
    this->ColorRampRange = NULL;
    }
  
  if ( this->ColorEditorFrame )
    {
    this->ColorEditorFrame->Delete();
    this->ColorEditorFrame = NULL;
    }

  if ( this->ColorStartValueButton )
    {
    this->ColorStartValueButton->Delete();
    this->ColorStartValueButton = NULL;    
    }

  if ( this->ColorEndValueButton )
    {
    this->ColorEndValueButton->Delete();
    this->ColorEndValueButton = NULL;    
    }

  if ( this->ColorMapLabel )
    {
    this->ColorMapLabel->Delete();
    this->ColorMapLabel = NULL;    
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
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("PVColorMap already created");
    return;
    }
  // Superclass create takes a KWApplication, but we need a PVApplication.
  if (pvApp == NULL)
    {
    vtkErrorMacro("Need a PV application");
    return;
    }
  
  this->SetApplication(app);
  
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  this->ScalarOpacityFrame = vtkKWLabeledFrame::New();
  this->ScalarOpacityFrame->SetParent(this);
  this->ScalarOpacityFrame->ShowHideFrameOn();
  this->ScalarOpacityFrame->Create(this->Application, "");
  this->ScalarOpacityFrame->SetLabel("Volume Scalar Opacity");

  this->ColorFrame = vtkKWLabeledFrame::New();
  this->ColorFrame->SetParent(this);
  this->ColorFrame->ShowHideFrameOn();
  this->ColorFrame->Create(this->Application, "");
  this->ColorFrame->SetLabel("Volume Scalar Color");

  this->ScalarOpacityRampLabel = vtkKWLabel::New();
  this->ScalarOpacityRampLabel->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityRampLabel->Create( this->Application, "" );
  this->ScalarOpacityRampLabel->SetLabel("Scalar Opacity Ramp:");
  this->ScalarOpacityRampLabel->SetBalloonHelpString(
    "Set the range for the scalar opacity ramp used to display the volume rendered data." );
  
  this->ScalarOpacityRampRange = vtkKWRange::New();
  this->ScalarOpacityRampRange->SetParent(this->ScalarOpacityFrame->GetFrame());
  this->ScalarOpacityRampRange->ShowEntriesOn();
  this->ScalarOpacityRampRange->Create(this->Application);
  this->ScalarOpacityRampRange->SetEndCommand( this, "ScalarOpacityRampChanged" );
  
  this->ScalarOpacityStartValueLabel = vtkKWLabel::New();
  this->ScalarOpacityStartValueLabel->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityStartValueLabel->Create( this->Application, "" );
  this->ScalarOpacityStartValueLabel->SetLabel("Ramp Start Opacity:");  
  
  this->ScalarOpacityStartValueScale = vtkKWScale::New();
  this->ScalarOpacityStartValueScale->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityStartValueScale->Create( this->Application, "" );
  this->ScalarOpacityStartValueScale->SetRange( 0.0, 1.0 );
  this->ScalarOpacityStartValueScale->SetResolution( 0.01 );
  this->ScalarOpacityStartValueScale->DisplayEntry();
  this->ScalarOpacityStartValueScale->DisplayEntryAndLabelOnTopOff();
  this->ScalarOpacityStartValueScale->SetEndCommand( this, "ScalarOpacityRampChanged" );
  
  this->ScalarOpacityEndValueLabel = vtkKWLabel::New();
  this->ScalarOpacityEndValueLabel->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityEndValueLabel->Create( this->Application, "" );
  this->ScalarOpacityEndValueLabel->SetLabel("Ramp End Opacity:");  

  this->ScalarOpacityEndValueScale = vtkKWScale::New();
  this->ScalarOpacityEndValueScale->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityEndValueScale->Create( this->Application, "" );
  this->ScalarOpacityEndValueScale->SetRange( 0.0, 1.0 );
  this->ScalarOpacityEndValueScale->SetResolution( 0.01 );
  this->ScalarOpacityEndValueScale->DisplayEntry();
  this->ScalarOpacityEndValueScale->DisplayEntryAndLabelOnTopOff();
  this->ScalarOpacityEndValueScale->SetEndCommand( this, "ScalarOpacityRampChanged" );


  this->Script("grid %s %s -sticky nsew -padx 2 -pady 2", 
               this->ScalarOpacityRampLabel->GetWidgetName(),  
               this->ScalarOpacityRampRange->GetWidgetName() );

  this->Script("grid %s %s -sticky nsew -padx 2 -pady 2", 
               this->ScalarOpacityStartValueLabel->GetWidgetName(),
               this->ScalarOpacityStartValueScale->GetWidgetName() );
  
  this->Script("grid %s %s -sticky nsew -padx 2 -pady 2", 
               this->ScalarOpacityEndValueLabel->GetWidgetName(),
               this->ScalarOpacityEndValueScale->GetWidgetName() );

  this->ColorRampLabel = vtkKWLabel::New();
  this->ColorRampLabel->SetParent( this->ColorFrame->GetFrame() );
  this->ColorRampLabel->Create( this->Application, "" );
  this->ColorRampLabel->SetLabel("Color Ramp:");
  this->ColorRampLabel->SetBalloonHelpString(
    "Set the range for the color ramp used to display the volume rendered data." );
  
  this->ColorRampRange = vtkKWRange::New();
  this->ColorRampRange->SetParent(this->ColorFrame->GetFrame());
  this->ColorRampRange->ShowEntriesOn();
  this->ColorRampRange->Create(this->Application);
  this->ColorRampRange->SetEndCommand( this, "ColorRampChanged" );

  this->ColorEditorFrame = vtkKWWidget::New();
  this->ColorEditorFrame->SetParent(this->ColorFrame->GetFrame());
  this->ColorEditorFrame->Create(this->Application, "frame", "");

  this->ColorStartValueButton = vtkKWChangeColorButton::New();
  this->ColorStartValueButton->SetParent(this->ColorEditorFrame);
  this->ColorStartValueButton->ShowLabelOff();
  this->ColorStartValueButton->Create(this->Application, "");
  this->ColorStartValueButton->SetColor(1.0, 0.0, 0.0);
  this->ColorStartValueButton->SetCommand(this, "ColorButtonCallback");
  this->ColorStartValueButton->SetBalloonHelpString("Select the minimum color.");

  this->ColorMapLabel = vtkKWLabel::New();
  this->ColorMapLabel->SetParent(this->ColorEditorFrame);
  this->ColorMapLabel->Create(this->Application, 
                    "-relief flat -bd 0 -highlightthickness 0 -padx 0 -pady 0");
  this->Script("bind %s <Configure> {%s ColorMapLabelConfigureCallback %s}", 
               this->ColorMapLabel->GetWidgetName(), 
               this->GetTclName(), "%w %h");

  this->ColorEndValueButton = vtkKWChangeColorButton::New();
  this->ColorEndValueButton->SetParent(this->ColorEditorFrame);
  this->ColorEndValueButton->ShowLabelOff();
  this->ColorEndValueButton->Create(this->Application, "");
  this->ColorEndValueButton->SetColor(0.0, 0.0, 1.0);
  this->ColorEndValueButton->SetCommand(this, "ColorButtonCallback");
  this->ColorEndValueButton->SetBalloonHelpString("Select the maximum color.");

  this->Script("grid %s %s %s -sticky news -padx 1 -pady 2",
               this->ColorStartValueButton->GetWidgetName(),
               this->ColorMapLabel->GetWidgetName(),
               this->ColorEndValueButton->GetWidgetName());
  
  this->Script("grid columnconfigure %s 1 -weight 1",
               this->ColorMapLabel->GetParent()->GetWidgetName());

  this->Script("grid %s %s -sticky nsew -padx 2 -pady 2", 
               this->ColorRampLabel->GetWidgetName(),  
               this->ColorRampRange->GetWidgetName() );

  this->Script("grid %s -columnspan 2 -sticky nsew -padx 2 -pady 2", 
               this->ColorEditorFrame->GetWidgetName());
  
  
  // Back button
  this->BackButton = vtkKWPushButton::New();
  this->BackButton->SetParent(this);
  this->BackButton->Create(this->Application, "-text {Back}");
  this->BackButton->SetCommand(this, "BackButtonCallback");

  this->Script("pack %s %s %s -side top -anchor n -fill x -padx 2 -pady 2", 
               this->ScalarOpacityFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName(),
               this->BackButton->GetWidgetName());
  

  int weights[2];
  weights[0] = 0;
  weights[1] = 1;  

  float factors[2];
  factors[0] = 1.5;
  factors[1] = 1.0;
  
  const char *widgets[2];
  widgets[0] = this->ScalarOpacityFrame->GetFrame()->GetWidgetName();
  widgets[1] = this->ColorFrame->GetFrame()->GetWidgetName();
  
  vtkKWTkUtilities::SynchroniseGridsColumnMinimumSize(
    pvApp->GetMainInterp(), 2, widgets, NULL, NULL);
    

}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ColorMapLabelConfigureCallback(int width, int height)
{
  this->UpdateMap(width, height);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::UpdateMap(int width, int height)
{
  int size;
  int i, j;
  double val, step;
  double *rgba;  
  unsigned char *ptr;  

  size = width*height;
  if (this->MapDataSize < size)
    {
    if (this->MapData)
      {
      delete [] this->MapData;
      }
    this->MapData = new unsigned char[size*3];
    this->MapDataSize = size;
    }
  this->MapWidth = width;
  this->MapHeight = height;

  vtkPVApplication *pvApp = NULL;
  
  if ( this->Application )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->Application);    
    }
  
  if ( !this->PVSource || !this->ArrayInfo || !pvApp ||
       this->PVSource->GetNumberOfParts() == 0 )
    {
    return;
    }
  
  vtkPVPart *part = this->PVSource->GetPart(0);

  vtkColorTransferFunction *colorFunc = 
      vtkColorTransferFunction::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(part->GetPartDisplay()->
                        GetVolumeColorID()));

  step = (this->ScalarRange[1]-this->ScalarRange[0])/(double)(width);
  ptr = this->MapData;
  for (j = 0; j < height; ++j)
    {
    for (i = 0; i < width; ++i)
      {
      val = this->ScalarRange[0] + ((double)(i)*step);
      rgba = colorFunc->GetColor(val);
      
      ptr[0] = static_cast<unsigned char>(rgba[0]*255.0 + 0.5);
      ptr[1] = static_cast<unsigned char>(rgba[1]*255.0 + 0.5);
      ptr[2] = static_cast<unsigned char>(rgba[2]*255.0 + 0.5);
      ptr += 3;
      }
    }

  if (size > 0)
    {
    this->ColorMapLabel->SetImageOption(this->MapData, width, height, 3);
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::BackButtonCallback()
{
  if (this->PVRenderView == NULL)
    {
    return;
    }

  // This has a side effect of gathering and display information.
  this->PVRenderView->GetPVWindow()->GetCurrentPVSource()->GetPVOutput()->UpdateProperties();
  this->PVRenderView->GetPVWindow()->ShowCurrentSourceProperties();
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
void vtkPVVolumeAppearanceEditor::SetPVSourceAndArrayInfo(vtkPVSource *source,
                                                          vtkPVArrayInformation *arrayInfo)
{
  this->PVSource = source;
  this->ArrayInfo = arrayInfo;
  
  vtkPVApplication *pvApp = NULL;
  
  if ( this->Application )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->Application);    
    }
  
  if ( this->PVSource && this->ArrayInfo && pvApp &&
       this->PVSource->GetNumberOfParts() > 0 )
    {
    // as a side effect, reset the GUI for the current data
    arrayInfo->GetComponentRange(0, this->ScalarRange);
  
    this->ScalarOpacityRampRange->SetWholeRange( this->ScalarRange[0], 
                                                 this->ScalarRange[1] );
    this->ColorRampRange->SetWholeRange( this->ScalarRange[0], 
                                                 this->ScalarRange[1] );
      
    vtkPVPart *part = this->PVSource->GetPart(0);

    vtkPiecewiseFunction *opacityFunc = 
      vtkPiecewiseFunction::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(part->GetPartDisplay()->
                        GetVolumeOpacityID()));

    vtkColorTransferFunction *colorFunc = 
      vtkColorTransferFunction::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(part->GetPartDisplay()->
                        GetVolumeColorID()));

    int size = opacityFunc->GetSize();
    
    if ( size != 2 )
      {
      vtkErrorMacro("Expecting 2 points in opacity function!");
      return;
      }
    
    double *ptr = opacityFunc->GetDataPointer();
    this->ScalarOpacityRampRange->SetRange(ptr[0], ptr[2]);
    this->ScalarOpacityRampRange->SetResolution( (ptr[2]-ptr[0])/10000.0 );
    this->ScalarOpacityStartValueScale->SetValue( ptr[1] );
    this->ScalarOpacityEndValueScale->SetValue( ptr[3] );    
    
    size = colorFunc->GetSize();
    if ( size != 2 )
      {
      vtkErrorMacro("Expecting 2 points in color function!");
      return;
      }
    ptr = colorFunc->GetDataPointer();
    
    this->ColorRampRange->SetRange(ptr[0], ptr[4]);
    this->ColorRampRange->SetResolution( (ptr[4]-ptr[0])/10000.0 );
    
    this->ColorStartValueButton->SetColor( ptr[1], ptr[2], ptr[3] );
    this->ColorEndValueButton->SetColor(   ptr[5], ptr[6], ptr[7] );
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ScalarOpacityRampChanged()
{
  this->ScalarOpacityRampChangedInternal();
  this->AddTraceEntry("$kw(%s) ScalarOpacityRampChanged", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ScalarOpacityRampChangedInternal()
{
  vtkPVApplication *pvApp = NULL;
  
  if ( this->Application )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->Application);    
    }
  
  if ( this->PVSource && this->ArrayInfo && pvApp )
    {
    float range[2];
    this->ScalarOpacityRampRange->GetRange(range);
    
    double startValue = this->ScalarOpacityStartValueScale->GetValue();
    double endValue = this->ScalarOpacityEndValueScale->GetValue();

    int numParts = this->PVSource->GetNumberOfParts();
    int i;
    vtkPVPart *part;
    
    for (i = 0; i < numParts; i++)
      {
      part =  this->PVSource->GetPart(i);
      vtkClientServerID volumeOpacityID;
      
      volumeOpacityID = part->GetPartDisplay()->GetVolumeOpacityID();
      
      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      vtkClientServerStream& stream = pm->GetStream();
      
      stream << vtkClientServerStream::Invoke << volumeOpacityID 
             << "RemoveAllPoints" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << volumeOpacityID 
             << "AddPoint" << range[0] << startValue << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << volumeOpacityID 
             << "AddPoint" << range[1] << endValue << vtkClientServerStream::End;
      pm->SendStreamToClientAndRenderServer();
      }
    this->RenderView();
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ColorButtonCallback( float vtkNotUsed(r), 
                                                       float vtkNotUsed(g), 
                                                       float vtkNotUsed(b) )
{
  this->ColorRampChanged();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ColorRampChanged()
{
  this->ColorRampChangedInternal();
  this->AddTraceEntry("$kw(%s) ScalarOpacityRampChanged", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ColorRampChangedInternal()
{
  vtkPVApplication *pvApp = NULL;
  
  if ( this->Application )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->Application);    
    }
  
  if ( this->PVSource && this->ArrayInfo && pvApp )
    {
    float range[2];
    this->ColorRampRange->GetRange(range);
    
    double *startColor = this->ColorStartValueButton->GetColor();
    double *endColor = this->ColorEndValueButton->GetColor();

    int numParts = this->PVSource->GetNumberOfParts();
    int i;
    vtkPVPart *part;
    
    for (i = 0; i < numParts; i++)
      {
      part =  this->PVSource->GetPart(i);
      vtkClientServerID volumeColorID;
      
      volumeColorID = part->GetPartDisplay()->GetVolumeColorID();
      
      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      vtkClientServerStream& stream = pm->GetStream();
      
      stream << vtkClientServerStream::Invoke << volumeColorID 
             << "RemoveAllPoints" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << volumeColorID 
             << "AddRGBPoint" << range[0] 
             << startColor[0] << startColor[1] << startColor[2] 
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << volumeColorID 
             << "AddRGBPoint" << range[1] 
             << endColor[0] << endColor[1] << endColor[2] 
             << vtkClientServerStream::End;
      pm->SendStreamToClientAndRenderServer();
      }
    this->RenderView();
    }
  
  if (this->MapWidth > 0 && this->MapHeight > 0)
    {
    this->UpdateMap(this->MapWidth, this->MapHeight);
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->BackButton);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

