/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.cxx
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
#include "vtkPVRenderView.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWCornerAnnotation.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWNotebook.h"
#include "vtkKWPushButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWScale.h"
#include "vtkKWSplitFrame.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVCameraIcon.h"
#include "vtkPVConfig.h"
#include "vtkPVData.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInteractorStyleControl.h"
#include "vtkPVNavigationWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSourceList.h"
#include "vtkPVTreeComposite.h"
#include "vtkPVWindow.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkRenderer.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#endif



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderView);
vtkCxxRevisionMacro(vtkPVRenderView, "1.206");

int vtkPVRenderViewCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//===========================================================================
//***************************************************************************
class vtkPVRenderViewObserver : public vtkCommand
{
public:
  static vtkPVRenderViewObserver *New() 
    {return new vtkPVRenderViewObserver;};

  vtkPVRenderViewObserver()
    {
      this->PVRenderView = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void* calldata)
    {
      if ( this->PVRenderView )
        {
        this->PVRenderView->ExecuteEvent(wdg, event, calldata);
        this->AbortFlagOn();
        }
    }

  vtkPVRenderView* PVRenderView;
};

//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
  if (getenv("PV_SEPARATE_RENDER_WINDOW") != NULL)
    {
    this->TopLevelRenderWindow = vtkKWWidget::New();
    this->TopLevelRenderWindow->SetParent(this->Frame);
    this->VTKWidget->SetParent(NULL);
    this->VTKWidget->SetParent(this->TopLevelRenderWindow);
    }
  else
    {
    this->TopLevelRenderWindow = NULL;
    }    

  this->CommandFunction = vtkPVRenderViewCommand;
  
  this->Interactive = 0;

  this->UseReductionFactor = 1;
  
  this->RenderWindow->SetDesiredUpdateRate(1.0);  

  this->SplitFrame = vtkKWSplitFrame::New();

  this->EventuallyRenderFlag = 0;
  this->RenderPending = NULL;

  this->MenuEntryUnderline = 4;
  this->SetMenuEntryName(VTK_PV_VIEW_MENU_LABEL);
  this->SetMenuEntryHelp("Show global view parameters (background color, annoations2 etc.)");

  this->StandardViewsFrame = vtkKWLabeledFrame::New();
  this->XMaxViewButton = vtkKWPushButton::New();
  this->XMinViewButton = vtkKWPushButton::New();
  this->YMaxViewButton = vtkKWPushButton::New();
  this->YMinViewButton = vtkKWPushButton::New();
  this->ZMaxViewButton = vtkKWPushButton::New();
  this->ZMinViewButton = vtkKWPushButton::New();
  
  this->ParallelRenderParametersFrame = vtkKWLabeledFrame::New();
  this->RenderParametersFrame = vtkKWLabeledFrame::New();

  this->TriangleStripsCheck = vtkKWCheckButton::New();
  this->ParallelProjectionCheck = vtkKWCheckButton::New();
  this->ImmediateModeCheck = vtkKWCheckButton::New();
  this->InterruptRenderCheck = vtkKWCheckButton::New();
  this->CompositeWithFloatCheck = vtkKWCheckButton::New();
  this->CompositeWithRGBACheck = vtkKWCheckButton::New();
  this->CompositeCompressionCheck = vtkKWCheckButton::New();

  this->ReductionCheck = vtkKWCheckButton::New();
  this->FrameRateFrame = vtkKWWidget::New();
  this->FrameRateLabel = vtkKWLabel::New();
  this->FrameRateScale = vtkKWScale::New();

  this->LODFrame = vtkKWLabeledFrame::New();
 
  this->LODThresholdFrame = vtkKWWidget::New();
  this->LODThresholdLabel = vtkKWLabel::New();
  this->LODThresholdScale = vtkKWScale::New();
  this->LODThresholdValue = vtkKWLabel::New();
  this->LODResolutionFrame = vtkKWWidget::New();
  this->LODResolutionLabel = vtkKWLabel::New();
  this->LODResolutionScale = vtkKWScale::New();
  this->LODResolutionValue = vtkKWLabel::New();
  this->CollectThresholdFrame = vtkKWWidget::New();
  this->CollectThresholdLabel = vtkKWLabel::New();
  this->CollectThresholdScale = vtkKWScale::New();
  this->CollectThresholdValue = vtkKWLabel::New();

  this->ManipulatorControl2D = vtkPVInteractorStyleControl::New();
  this->ManipulatorControl2D->SetRegisteryName("2D");
  this->ManipulatorControl3D = vtkPVInteractorStyleControl::New();
  this->ManipulatorControl3D->SetRegisteryName("3D");

  this->RendererTclName     = 0;
  this->CompositeTclName    = 0;
  this->RenderWindowTclName = 0;
  this->InteractiveCompositeTime = 0;
  this->InteractiveRenderTime    = 0;
  this->StillRenderTime          = 0;
  this->StillCompositeTime       = 0;
  this->Composite                = 0;

  this->DisableRenderingFlag = 0;
  this->NavigationFrame = vtkKWLabeledFrame::New();
  this->NavigationWindow = vtkPVNavigationWindow::New();
  this->SelectionWindow = vtkPVSourceList::New();

  this->ShowSelectionWindow = 0;
  this->ShowNavigationWindow = 0;

  this->ParaViewOptionsFrame = vtkKWLabeledFrame::New();
  this->NavigationWindowButton = vtkKWRadioButton::New();
  this->SelectionWindowButton = vtkKWRadioButton::New();
  this->Display3DWidgets = vtkKWCheckButton::New();

  this->LODThreshold = 1000;
  this->LODResolution = 50;
  this->CollectThreshold = 2.0;

  int cc;
  for ( cc = 0; cc < 6; cc ++ )
    {
    this->CameraIcons[cc] = vtkPVCameraIcon::New();
    }
  this->CameraIconsFrame = vtkKWLabeledFrame::New();

  this->Observer = vtkPVRenderViewObserver::New();
  this->Observer->PVRenderView = this;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ShowNavigationWindowCallback(int registery)
{
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->NavigationFrame->GetFrame()->GetWidgetName());
  this->Script("pack %s -fill both -expand t -side top -anchor n", 
               this->NavigationWindow->GetWidgetName());
  this->NavigationFrame->SetLabel("Navigation Window");
  this->ShowSelectionWindow = 0;
  this->ShowNavigationWindow = 1;

  this->NavigationWindowButton->StateOn();
  this->SelectionWindowButton->StateOff();
  if ( registery )
    {
    this->Application->SetRegisteryValue(2, "RunTime","SourcesBrowser",
                                         "NavigationWindow");
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ShowSelectionWindowCallback(int registery)
{
  if ( !this->Application )
    {
    return;
    }
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->NavigationFrame->GetFrame()->GetWidgetName());
  this->Script("pack %s -fill both -expand t -side top -anchor n", 
               this->SelectionWindow->GetWidgetName());
  this->NavigationFrame->SetLabel("Selection Window");
  this->ShowNavigationWindow = 0;
  this->ShowSelectionWindow = 1;

  this->NavigationWindowButton->StateOff();
  this->SelectionWindowButton->StateOn();
  if ( registery )
    {
    this->Application->SetRegisteryValue(2, "RunTime","SourcesBrowser",
                                         "SelectionWindow");
    }
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
  vtkPVApplication *pvApp = 0;
  if ( this->Application )
    {
    pvApp = this->GetPVApplication();
    }

  this->ParaViewOptionsFrame->Delete();
  this->NavigationWindowButton->Delete();
  this->SelectionWindowButton->Delete();
  this->Display3DWidgets->Delete();

  if ( this->SelectionWindow )
    {
    this->SelectionWindow->Delete();
    }
  this->SplitFrame->Delete();
  this->SplitFrame = NULL;
 
  this->NavigationFrame->Delete();
  this->NavigationFrame = NULL;
 
  this->NavigationWindow->Delete();
  this->NavigationWindow = NULL;

  this->LODFrame->Delete();
  this->LODFrame = NULL;

  // Tree Composite
  if (this->Composite)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->CompositeTclName);
      }
    else
      {
      this->Composite->Delete();
      }
    this->SetCompositeTclName(NULL);
    this->Composite = NULL;
    }
  
  if (this->Renderer)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->RendererTclName);
      }
    else
      {
      this->Renderer->Delete();
      }
    this->SetRendererTclName(NULL);
    this->Renderer = NULL;
    }
  
  if (this->RenderWindow)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->RenderWindowTclName);
      }
    else
      {
      this->RenderWindow->Delete();
      }
    this->SetRenderWindowTclName(NULL);
    this->RenderWindow = NULL;
    }
  
  // undo the binding we set up
  if ( this->Application )
    {
    this->Script("bind %s <Motion> {}", this->VTKWidget->GetWidgetName());
    }
  if (this->RenderPending && this->Application )
    {
    this->Script("after cancel %s", this->RenderPending);
    }
  this->SetRenderPending(NULL);

  if (this->TopLevelRenderWindow)
    {
    this->TopLevelRenderWindow->Delete();
    this->TopLevelRenderWindow = NULL;
    }
  
  this->StandardViewsFrame->Delete();
  this->StandardViewsFrame = NULL;
  this->XMaxViewButton->Delete();
  this->XMaxViewButton = NULL;
  this->XMinViewButton->Delete();
  this->XMinViewButton = NULL;
  this->YMaxViewButton->Delete();
  this->YMaxViewButton = NULL;
  this->YMinViewButton->Delete();
  this->YMinViewButton = NULL;
  this->ZMaxViewButton->Delete();
  this->ZMaxViewButton = NULL;
  this->ZMinViewButton->Delete();
  this->ZMinViewButton = NULL;

  this->RenderParametersFrame->Delete();
  this->RenderParametersFrame = 0;

  this->ParallelRenderParametersFrame->Delete();
  this->ParallelRenderParametersFrame = 0;

  this->ParallelProjectionCheck->Delete();
  this->ParallelProjectionCheck = NULL;

  this->TriangleStripsCheck->Delete();
  this->TriangleStripsCheck = NULL;

  this->ImmediateModeCheck->Delete();
  this->ImmediateModeCheck = NULL;

  this->InterruptRenderCheck->Delete();
  this->InterruptRenderCheck = NULL;

  this->CompositeWithFloatCheck->Delete();
  this->CompositeWithFloatCheck = NULL;

  this->CompositeWithRGBACheck->Delete();
  this->CompositeWithRGBACheck = NULL;
  
  this->CompositeCompressionCheck->Delete();
  this->CompositeCompressionCheck = NULL;


  this->ManipulatorControl2D->Delete();
  this->ManipulatorControl3D->Delete();
  
  this->ReductionCheck->Delete();
  this->ReductionCheck = NULL;
  
  this->FrameRateFrame->Delete();
  this->FrameRateFrame = NULL;
  this->FrameRateLabel->Delete();
  this->FrameRateLabel = NULL;
  this->FrameRateScale->Delete();
  this->FrameRateScale = NULL;

  this->LODThresholdFrame->Delete();
  this->LODThresholdFrame = NULL;
  this->LODThresholdLabel->Delete();
  this->LODThresholdLabel = NULL;
  this->LODThresholdScale->Delete();
  this->LODThresholdScale = NULL;
  this->LODThresholdValue->Delete();
  this->LODThresholdValue = NULL;

  this->LODResolutionFrame->Delete();
  this->LODResolutionFrame = NULL;
  this->LODResolutionLabel->Delete();
  this->LODResolutionLabel = NULL;
  this->LODResolutionScale->Delete();
  this->LODResolutionScale = NULL;
  this->LODResolutionValue->Delete();
  this->LODResolutionValue = NULL;

  this->CollectThresholdFrame->Delete();
  this->CollectThresholdFrame = NULL;
  this->CollectThresholdLabel->Delete();
  this->CollectThresholdLabel = NULL;
  this->CollectThresholdScale->Delete();
  this->CollectThresholdScale = NULL;
  this->CollectThresholdValue->Delete();
  this->CollectThresholdValue = NULL;

  this->CameraIconsFrame->Delete();
  this->CameraIconsFrame = 0;
  int cc;
  for ( cc = 0; cc < 6; cc ++ )
    {
    if ( this->CameraIcons[cc] )
      {
      this->CameraIcons[cc]->SetRenderView(0);
      this->CameraIcons[cc]->Delete();
      this->CameraIcons[cc] = 0;
      }
    }
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
void PVRenderViewAbortCheck(void *arg)
{
  vtkPVRenderView *me = (vtkPVRenderView*)arg;
  int abort;

  // if we are printing then do not abort
  if (me->GetPrinting())
    {
    return;
    }
  
  abort = me->ShouldIAbort();
  if (abort == 1)
    {
    me->GetRenderWindow()->SetAbortRender(1);
    me->EventuallyRender();
    }
  if (abort == 2)
    {
    //("Abort 2");
    me->GetRenderWindow()->SetAbortRender(2);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CreateRenderObjects(vtkPVApplication *pvApp)
{
  // Get rid of renderer created by the superclass
  this->Renderer->Delete();

  this->Renderer = (vtkRenderer*)pvApp->MakeTclObject("vtkRenderer", "Ren1");
  this->RendererTclName = NULL;
  this->SetRendererTclName("Ren1");
  
  // Get rid of render window created by the superclass
  this->RenderWindow->Delete();
  this->RenderWindow = (vtkRenderWindow*)pvApp->MakeTclObject("vtkRenderWindow", "RenWin1");
  this->RenderWindow->AddObserver(vtkCommand::CursorChangedEvent, this->Observer);

  this->RenderWindowTclName = NULL;
  this->SetRenderWindowTclName("RenWin1");
  
  if (this->RenderWindow->IsA("vtkOpenGLRenderWindow") &&
      (pvApp->GetController()->GetNumberOfProcesses() > 1))
    {
    pvApp->BroadcastScript("%s SetMultiSamples 0", this->RenderWindowTclName);
    }

  // Create the compositer.
  this->Composite = static_cast<vtkPVTreeComposite*>(pvApp->MakeTclObject("vtkPVTreeComposite", "TreeComp1"));

  // Try using a more efficient compositer (if it exists).
  // This should be a part of a module.
  pvApp->BroadcastScript("if {[catch {vtkCompressCompositer pvTmp}] == 0} {TreeComp1 SetCompositer pvTmp; pvTmp Delete}");

  this->CompositeTclName = NULL;
  this->SetCompositeTclName("TreeComp1");

        // If we are using SGI pipes, create a new Controller/Communicator/Group
  // to use for compositing.
  if (pvApp->GetUseRenderingGroup())
    {
    int numPipes = pvApp->GetNumberOfPipes();
    // I would like to create another controller with a subset of world, but ...
    // For now, I added it as a hack to the composite manager.
    pvApp->BroadcastScript("%s SetNumberOfProcesses %d",
                           this->CompositeTclName, numPipes);
    }

  pvApp->BroadcastScript("%s AddRenderer %s", this->RenderWindowTclName,
                         this->RendererTclName);
  pvApp->BroadcastScript("%s SetRenderWindow %s", this->CompositeTclName,
                         this->RenderWindowTclName);
  pvApp->BroadcastScript("%s InitializeRMIs", this->CompositeTclName);

  if ( getenv("PV_DISABLE_COMPOSITE_INTERRUPTS") )
    {
    pvApp->BroadcastScript("%s EnableAbortOff", this->CompositeTclName);
    }

  if ( getenv("PV_OFFSCREEN") )
    {
    pvApp->BroadcastScript("%s InitializeOffScreen", this->CompositeTclName);
    }

}


//----------------------------------------------------------------------------
// Here we are going to change only the satellite procs.
void vtkPVRenderView::PrepareForDelete()
{
  vtkPVApplication* pvapp = this->GetPVApplication();
  if (pvapp)
    {
    pvapp->SetRegisteryValue(2, "RunTime", "UseParallelProjection", "%d",
                             this->ParallelProjectionCheck->GetState());
    pvapp->SetRegisteryValue(2, "RunTime", "UseStrips", "%d",
                             this->TriangleStripsCheck->GetState());
    pvapp->SetRegisteryValue(2, "RunTime", "UseImmediateMode", "%d",
                             this->ImmediateModeCheck->GetState());
    pvapp->SetRegisteryValue(2, "RunTime", "UseReduction", "%d",
                             this->ReductionCheck->GetState());
    pvapp->SetRegisteryValue(2, "RunTime", "FrameRate", "%f",
                             this->FrameRateScale->GetValue());
    pvapp->SetRegisteryValue(2, "RunTime", "LODThreshold", "%d",
                             this->LODThreshold);
    pvapp->SetRegisteryValue(2, "RunTime", "LODResolution", "%d",
                             this->LODResolution);
#ifdef VTK_USE_MPI
    pvapp->SetRegisteryValue(2, "RunTime", "CollectThreshold", "%f",
                             this->CollectThreshold);
    pvapp->SetRegisteryValue(2, "RunTime", "InterruptRender", "%d",
                             this->InterruptRenderCheck->GetState());
    pvapp->SetRegisteryValue(2, "RunTime", "UseFloatInComposite", "%d",
                             this->CompositeWithFloatCheck->GetState());
    pvapp->SetRegisteryValue(2, "RunTime", "UseRGBAInComposite", "%d",
                             this->CompositeWithRGBACheck->GetState());
    pvapp->SetRegisteryValue(2, "RunTime", "UseCompressionInComposite", "%d",
                             this->CompositeCompressionCheck->GetState());
#endif
    }

  // Circular reference.
  if (this->Composite)
    {
    this->Composite->SetRenderView(NULL);
    }
  if ( this->ManipulatorControl2D )
    {
    this->ManipulatorControl2D->SetManipulatorCollection(0);
    }
  if ( this->ManipulatorControl3D )
    {
    this->ManipulatorControl3D->SetManipulatorCollection(0);
    }
  if ( this->SelectionWindow )
    {
    this->SelectionWindow->PrepareForDelete();
    this->SelectionWindow->Delete();
    this->SelectionWindow = 0;
    }
  int cc;
  for ( cc = 0; cc < 6; cc ++ )
    {
    if ( this->CameraIcons[cc] )
      {
      this->CameraIcons[cc]->SetRenderView(0);
      this->CameraIcons[cc]->Delete();
      this->CameraIcons[cc] = 0;
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderView::Close()
{
  this->PrepareForDelete();
  vtkKWView::Close();
}  


//----------------------------------------------------------------------------
vtkRenderer *vtkPVRenderView::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkPVRenderView::GetRenderWindow()
{
  return this->RenderWindow;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Create(vtkKWApplication *app, const char *args)
{
  char *local;
  const char *wname;
  
  local = new char [strlen(args)+100];

  if (this->Application)
    {
    vtkErrorMacro("RenderView already created");
    return;
    }
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("RenderView already created");
    return;
    }
  this->SetApplication(app);


  // Application has to be set before we can get a tcl name.
  // Otherwise I would have done this in "CreateRenderObjects".
  // Create the compositer.
  if (this->Composite)
    {
    // Since the render view is only on process 0, do not broadcast.
    this->GetPVApplication()->Script("%s SetRenderView %s", 
                                     this->CompositeTclName, 
                                     this->GetTclName());
    }
  
  // create the frame
  wname = this->GetWidgetName();
  this->Script("frame %s -bd 0 %s",wname,args);
  //this->Script("pack %s -expand yes -fill both",wname);
  
  // create the label
  this->Frame->Create(app,"frame","-bd 3 -relief ridge");
  this->Script("pack %s -expand yes -fill both -side top -anchor nw",
               this->Frame->GetWidgetName());
  this->Frame2->Create(app,"frame","-bd 0 -bg #888");
  this->Script("pack %s -fill x -side top -anchor nw",
               this->Frame2->GetWidgetName());
  this->Label->Create(app,"label","-fg #fff -text {3D View} -bd 0");
  this->Script("pack %s  -side left -anchor w",this->Label->GetWidgetName());
  this->Script("bind %s <Any-ButtonPress> {%s MakeSelected}",
               this->Label->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Any-ButtonPress> {%s MakeSelected}",
               this->Frame2->GetWidgetName(), this->GetTclName());

  // Create the control frame - only pack it if support option enabled
  this->ControlFrame->Create(app,"frame","-bd 0");
  if (this->SupportControlFrame)
    {
    this->Script("pack %s -expand t -fill both -side top -anchor nw",
                 this->ControlFrame->GetWidgetName());
    }
  
  // Separate window for the renderer.
  if (getenv("PV_SEPARATE_RENDER_WINDOW") != NULL)
    {
    this->TopLevelRenderWindow->Create(app, "toplevel", "");
    this->Script("wm title %s %s", 
                 this->TopLevelRenderWindow->GetWidgetName(),
                 this->Application->GetApplicationName());
    }

  // add the -rw argument
  sprintf(local,"%s -rw Addr=%p",args,this->RenderWindow);
  this->Script("vtkTkRenderWidget %s %s",
               this->VTKWidget->GetWidgetName(),local);
  //this->Script("vtkTkRenderWidget %s %s",
  //             this->VTKWidget->GetWidgetName(),args);
  this->Script("pack %s -expand yes -fill both -side top -anchor nw",
               this->VTKWidget->GetWidgetName());
  
  // Expose.
  this->Script("bind %s <Expose> {%s Exposed}", 
               this->GetTclName(),
               this->GetTclName());

  // Configure
  this->Script("bind %s <Configure> {%s Configured}", 
               this->GetTclName(),
               this->GetTclName());

  this->SplitFrame->SetParent(this->GetPropertiesParent());
  this->SplitFrame->SetOrientationToVertical();
  this->SplitFrame->Create(this->Application);
  this->SplitFrame->SetFrame1MinimumSize(80);
  this->SplitFrame->SetFrame1Size(80);
  this->SplitFrame->SetSeparatorSize(5);
  this->Script("pack %s -fill both -expand t -side top", 
               this->SplitFrame->GetWidgetName());

  this->NavigationFrame->SetParent(this->SplitFrame->GetFrame1());
  this->NavigationFrame->ShowHideFrameOff();
  this->NavigationFrame->Create(this->Application);  
  this->NavigationFrame->SetLabel("Navigation");
  this->Script("pack %s -fill both -expand t -side top", 
               this->NavigationFrame->GetWidgetName());

  this->NavigationWindow->SetParent(this->NavigationFrame->GetFrame());
  this->NavigationWindow->SetWidth(341);
  this->NavigationWindow->SetHeight(545);
  this->NavigationWindow->Create(this->Application, 0); 

  this->SelectionWindow->SetParent(this->NavigationFrame->GetFrame());
  this->SelectionWindow->SetWidth(341);
  this->SelectionWindow->SetHeight(545);
  this->SelectionWindow->Create(this->Application, 0); 

  if (this->Application->GetRegisteryValue(2, 
                                           "SourcesBrowser", 
                                           "SelectionWindow", 0))
    {
    if ( this->Application->BooleanRegisteryCheck(2, "SourcesBrowser",
                                                  "SelectionWindow") )
      {
      this->ShowSelectionWindowCallback(0);
      }
    else
      {
      this->ShowNavigationWindowCallback(0);
      }
    }
  else
    {
    this->ShowSelectionWindowCallback(0);
    }

  if ( 
    !this->Application->GetRegisteryValue(2, "RunTime", "Display3DWidgets", 0) ||
    this->GetPVWindow()->GetIntRegisteryValue(2, "RunTime", "Display3DWidgets") )
    {
    this->SetDisplay3DWidgets(1);
    }
  else
    {
    this->SetDisplay3DWidgets(0);
    }
  
  this->EventuallyRender();
  delete [] local;
}


//----------------------------------------------------------------------------
vtkKWWidget *vtkPVRenderView::GetSourceParent()
{
  return this->SplitFrame->GetFrame2();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Display3DWidgetsCallback()
{
  int val = this->Display3DWidgets->GetState();
  this->SetDisplay3DWidgets(val);
  this->Application->SetRegisteryValue(2, "RunTime","Display3DWidgets",
                                      (val?"1":"0"));
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetDisplay3DWidgets(int s)
{
  this->Display3DWidgets->SetState(s);
  this->GetPVApplication()->SetDisplay3DWidgets(s);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CreateViewProperties()
{
  this->vtkKWView::CreateViewProperties();

  vtkPVWindow* pvwindow = this->GetPVWindow();
  vtkPVApplication* pvapp = this->GetPVApplication();
  //this->RenderParametersFrame->ShowHideFrameOn();  

  this->RenderParametersFrame->SetParent( this->GeneralProperties->GetFrame() );
  this->RenderParametersFrame->ShowHideFrameOn();
  this->RenderParametersFrame->Create(this->Application);
  this->RenderParametersFrame->SetLabel("Advanced Render Parameters");
  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->RenderParametersFrame->GetWidgetName());

  this->ParallelProjectionCheck->SetParent(
    this->RenderParametersFrame->GetFrame());
  this->ParallelProjectionCheck->Create(this->Application, "");
  this->ParallelProjectionCheck->SetText("Use Parallel Projection");
  if (pvapp && pvwindow && 
      pvapp->GetRegisteryValue(2, "RunTime", "UseParallelProjection", 0))
    {
    this->ParallelProjectionCheck->SetState(
      pvwindow->GetIntRegisteryValue(2, "RunTime", "UseParallelProjection"));
    this->ParallelProjectionCallback();
    }
  else
    {
    this->ParallelProjectionCheck->SetState(0);
    }
  this->ParallelProjectionCheck->SetCommand(this, "ParallelProjectionCallback");
  this->ParallelProjectionCheck->SetBalloonHelpString(
    "Toggle the use of parallel projection vesus perspective.");
  
  this->TriangleStripsCheck->SetParent(
    this->RenderParametersFrame->GetFrame());
  this->TriangleStripsCheck->Create(this->Application, "");
  this->TriangleStripsCheck->SetText("Use Triangle Strips");
  if (pvapp && pvwindow && 
      pvapp->GetRegisteryValue(2, "RunTime", "UseStrips", 0))
    {
    this->TriangleStripsCheck->SetState(
      pvwindow->GetIntRegisteryValue(2, "RunTime", "UseStrips"));
    }
  else
    {
    this->TriangleStripsCheck->SetState(0);
    }
  this->TriangleStripsCheck->SetCommand(this, "TriangleStripsCallback");
  this->TriangleStripsCheck->SetBalloonHelpString(
    "Toggle the use of triangle strips when rendering polygonal data");
  
  this->ImmediateModeCheck->SetParent(this->RenderParametersFrame->GetFrame());
  this->ImmediateModeCheck->Create(this->Application, 
                                   "-text \"Use Immediate Mode Rendering\"");
  this->ImmediateModeCheck->SetCommand(this, "ImmediateModeCallback");
  if (pvapp && pvwindow && 
      pvapp->GetRegisteryValue(2, "RunTime", "UseImmediateMode", 0))
    {
    this->ImmediateModeCheck->SetState(
      pvwindow->GetIntRegisteryValue(2, "RunTime", "UseImmediateMode"));
    }
  else
    {
    this->ImmediateModeCheck->SetState(1);
    }
  this->ImmediateModeCheck->SetBalloonHelpString("Toggle the use of immediate mode rendering (when off, display lists are used)");

  this->ReductionCheck->SetParent(this->RenderParametersFrame->GetFrame());
  this->ReductionCheck->Create(this->Application, "-text Reduction");
  this->ReductionCheck->SetCommand(this, "ReductionCheckCallback");
  if (pvapp && pvwindow &&
      pvapp->GetRegisteryValue(2, "RunTime", "UseReduction", 0))
    {
    this->ReductionCheck->SetState(
      pvwindow->GetIntRegisteryValue(2, "RunTime", "UseReduction"));
    }
  else
    {
    this->ReductionCheck->SetState(1);
    }
  this->ReductionCheck->SetBalloonHelpString(
    "If selected, tree compositing will scale the size of the render window "
    "based on how long the previous render took.");

  this->FrameRateFrame->SetParent(this->RenderParametersFrame->GetFrame());
  this->FrameRateFrame->Create(this->Application, "frame", "");
  this->FrameRateLabel->SetParent(this->FrameRateFrame);
  this->FrameRateLabel->Create(this->Application, "");
  this->FrameRateLabel->SetLabel("Reduction");
  this->FrameRateScale->SetParent(this->FrameRateFrame);
  this->FrameRateScale->Create(this->Application, 
                               "-resolution 0.1 -orient horizontal");
  this->FrameRateScale->SetRange(0, 50);
  if (pvapp && pvwindow &&
      pvapp->GetRegisteryValue(2, "RunTime", "FrameRate", 0))
    {
    this->FrameRateScale->SetValue(
      pvwindow->GetFloatRegisteryValue(2, "RunTime", "FrameRate"));
    }
  else
    {
    this->FrameRateScale->SetValue(3.0);
    }
  this->FrameRateScale->SetCommand(this, "FrameRateScaleCallback");
  this->FrameRateScale->SetBalloonHelpString(
    "This slider adjusts the target for subsample compression during "
    "compositing.  "
    "Left: Use slow full-resolution compositing. Right: Fast pixelated "
    "compositing.");

  this->Script("pack %s %s %s -side top -anchor w",
               this->ParallelProjectionCheck->GetWidgetName(),
               this->TriangleStripsCheck->GetWidgetName(),
               this->ImmediateModeCheck->GetWidgetName());

  this->LODFrame->SetParent( this->GeneralProperties->GetFrame() );
  this->LODFrame->ShowHideFrameOn();
  this->LODFrame->Create(this->Application);
  this->LODFrame->SetLabel("LOD Parameters");
  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->LODFrame->GetWidgetName());

  // Determines when the decimated LOD is used.
  this->LODThresholdFrame->SetParent(this->LODFrame->GetFrame());
  this->LODThresholdFrame->Create(this->Application, "frame", "");
  this->LODThresholdLabel->SetParent(this->LODThresholdFrame);
  this->LODThresholdLabel->Create(this->Application, "-width 18 -anchor w");
  this->LODThresholdLabel->SetLabel("LOD Threshold:");
  this->LODThresholdScale->SetParent(this->LODThresholdFrame);
  this->LODThresholdScale->Create(this->Application, 
                               "-resolution 0.1 -orient horizontal");
  this->LODThresholdScale->SetRange(0, 18);
  this->LODThresholdScale->SetResolution(0.1);
  this->LODThresholdValue->SetParent(this->LODThresholdFrame);
  this->LODThresholdValue->Create(this->Application, "");
  if (pvapp && pvwindow &&
      pvapp->GetRegisteryValue(2, "RunTime", "LODThreshold", 0))
    {
    this->SetLODThreshold(
      pvwindow->GetIntRegisteryValue(2, "RunTime", "LODThreshold"));
    }
  if (this->LODThreshold > 0 )
    {
    this->LODThresholdScale->SetValue(18.0 - log((double)(this->LODThreshold)));
    }
  this->LODThresholdScale->SetCommand(this, "LODThresholdScaleCallback");
  this->LODThresholdScale->SetBalloonHelpString(
    "This slider adjusts when the decimated LOD is used. Threshold is based on "
    "number of points.  "
    "Left: Use slow full-resolution models. Right: Use fast decimated models .");

  pvapp->Script("pack %s %s %s -side left", 
                this->LODThresholdLabel->GetWidgetName(), 
                this->LODThresholdScale->GetWidgetName(),
                this->LODThresholdValue->GetWidgetName());
  
  // Determines the complexity of the decimated LOD
  this->LODResolutionFrame->SetParent(this->LODFrame->GetFrame());
  this->LODResolutionFrame->Create(this->Application, "frame", "");
  this->LODResolutionLabel->SetParent(this->LODResolutionFrame);
  this->LODResolutionLabel->Create(this->Application, "-width 18  -anchor w");
  this->LODResolutionLabel->SetLabel("LOD Resolution:");
  this->LODResolutionScale->SetParent(this->LODResolutionFrame);
  this->LODResolutionScale->Create(this->Application, 
                               "-orient horizontal");
  this->LODResolutionScale->SetRange(10, 160);
  this->LODResolutionScale->SetResolution(1.0);
  this->LODResolutionValue->SetParent(this->LODResolutionFrame);
  this->LODResolutionValue->Create(this->Application, "");
  if (pvapp && pvwindow &&
      pvapp->GetRegisteryValue(2, "RunTime", "LODResolution", 0))
    {
    this->SetLODResolution(
      pvwindow->GetIntRegisteryValue(2, "RunTime", "LODResolution"));
    }
  this->LODResolutionScale->SetValue(150 - this->LODResolution);
  this->LODResolutionScale->SetCommand(this, "LODResolutionScaleCallback");
  this->LODResolutionScale->SetBalloonHelpString(
    "This slider determines the resolution of the decimated level of details.  "
    "The value is the dimension for each axis in the quadric clustering filter. "
    "Left: Use slow high-resolution models. Right: Use fast simple models .");
  pvapp->Script("pack %s %s %s -side left", 
                this->LODResolutionLabel->GetWidgetName(), 
                this->LODResolutionScale->GetWidgetName(),
                this->LODResolutionValue->GetWidgetName());

  this->InterruptRenderCheck->SetParent(
    this->LODFrame->GetFrame());
  this->InterruptRenderCheck->Create(this->Application, 
                                     "-text \"Allow Rendering Interrupts\"");
  this->InterruptRenderCheck->SetCommand(this, "InterruptRenderCallback");
  
  if (pvwindow && pvapp && pvapp->GetRegisteryValue(2, "RunTime", 
                                                    "InterruptRender", 0))
    {
    this->InterruptRenderCheck->SetState(
      pvwindow->GetIntRegisteryValue(2, "RunTime", "InterruptRender"));
    }
  else
    {
    this->InterruptRenderCheck->SetState(1);
    }
  this->InterruptRenderCallback();
  this->InterruptRenderCheck->SetBalloonHelpString(
    "Toggle the use of  render interrupts (when using MPI, this uses "
    "asynchronous messaging). When off, renders can not be interrupted.");

  this->Script("pack %s %s -side top -anchor w",
               this->LODResolutionFrame->GetWidgetName(),
               this->LODThresholdFrame->GetWidgetName());

  if (pvapp->GetController()->GetNumberOfProcesses() > 1)
    {
    // Determines when geometry is collected to process 0 for rendering.
    this->CollectThresholdFrame->SetParent(this->LODFrame->GetFrame());
    this->CollectThresholdFrame->Create(this->Application, "frame", "");
    this->CollectThresholdLabel->SetParent(this->CollectThresholdFrame);
    this->CollectThresholdLabel->Create(this->Application, 
                                        "-width 18  -anchor w");
    this->CollectThresholdLabel->SetLabel("Collection Threshold:");
    this->CollectThresholdScale->SetParent(this->CollectThresholdFrame);
    this->CollectThresholdScale->Create(this->Application, 
                                        "-orient horizontal");
    this->CollectThresholdScale->SetRange(0.0, 6.0);
    this->CollectThresholdScale->SetResolution(0.1);
    this->CollectThresholdValue->SetParent(this->CollectThresholdFrame);
    this->CollectThresholdValue->Create(this->Application, "");
    if (pvapp && pvwindow &&
        pvapp->GetRegisteryValue(2, "RunTime", "CollectThreshold", 0))
      {
      this->SetCollectThreshold(
        pvwindow->GetIntRegisteryValue(2, "RunTime", "CollectThreshold"));
      }
    this->CollectThresholdScale->SetValue(this->CollectThreshold);
    this->CollectThresholdScale->SetCommand(this, 
                                            "CollectThresholdScaleCallback");
    this->CollectThresholdScale->SetBalloonHelpString(
      "This slider determines when models are collected to process 0 for local "
      "rendering.  "
      "Threshold critera is based on size of model in mega bytes.  "
      "Left: Always leave models distributed. Right: Move even large models to "
      "process 0.");    pvapp->Script("pack %s %s %s -side left", 
                  this->CollectThresholdLabel->GetWidgetName(), 
                  this->CollectThresholdScale->GetWidgetName(),
                  this->CollectThresholdValue->GetWidgetName());
    this->Script("pack %s -side top -anchor w", 
                 this->CollectThresholdFrame->GetWidgetName());
    }

  this->Script("pack %s -side top -anchor w",
               this->InterruptRenderCheck->GetWidgetName());


  if (pvapp->GetController()->GetNumberOfProcesses() > 1)
    {
    this->ParallelRenderParametersFrame->SetParent( 
      this->GeneralProperties->GetFrame() );
    this->ParallelRenderParametersFrame->ShowHideFrameOn();
    this->ParallelRenderParametersFrame->Create(this->Application);
    this->ParallelRenderParametersFrame->SetLabel(
      "Parallel Rendering Parameters");
    this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
                 this->ParallelRenderParametersFrame->GetWidgetName());

  
    this->CompositeWithFloatCheck->SetParent(
      this->ParallelRenderParametersFrame->GetFrame());
    this->CompositeWithFloatCheck->Create(this->Application, 
                                          "-text \"Composite With Floats\"");
    this->CompositeWithRGBACheck->SetParent(
      this->ParallelRenderParametersFrame->GetFrame());
    this->CompositeWithRGBACheck->Create(this->Application, 
                                         "-text \"Composite RGBA\"");
    this->CompositeCompressionCheck->SetParent(
      this->ParallelRenderParametersFrame->GetFrame());
    this->CompositeCompressionCheck->Create(this->Application, 
                                            "-text \"Composite Compression\"");
  
    this->CompositeWithFloatCheck->SetCommand(this, 
                                              "CompositeWithFloatCallback");
    if (pvwindow && pvapp && 
        pvapp->GetRegisteryValue(2, "RunTime", "UseFloatInComposite", 0))
      {
      this->CompositeWithFloatCheck->SetState(
        pvwindow->GetIntRegisteryValue(2, "RunTime", "UseFloatInComposite"));
      this->CompositeWithFloatCallback();
      }
    else
      {
      this->CompositeWithFloatCheck->SetState(0);
      }
    this->CompositeWithFloatCheck->SetBalloonHelpString(
      "Toggle the use of char/float values when compositing. "
      "If rendering defects occur, try turning this on.");
  
    this->CompositeWithRGBACheck->SetCommand(this, "CompositeWithRGBACallback");
    if (pvwindow && pvapp && pvapp->GetRegisteryValue(2, "RunTime", 
                                                      "UseRGBAInComposite", 0))
      {
      this->CompositeWithRGBACheck->SetState(
        pvwindow->GetIntRegisteryValue(2, "RunTime", "UseRGBAInComposite"));
      this->CompositeWithRGBACallback();
      }
    else
      {
      this->CompositeWithRGBACheck->SetState(0);
      }
    this->CompositeWithRGBACheck->SetBalloonHelpString(
      "Toggle the use of RGB/RGBA values when compositing. "
      "This is here to bypass some bugs in some graphics card drivers.");

    this->CompositeCompressionCheck->SetCommand(this, 
                                                "CompositeCompressionCallback");
    if (pvwindow && pvapp && 
        pvapp->GetRegisteryValue(2, "RunTime",  "UseCompressionInComposite", 0))
      {
      this->CompositeCompressionCheck->SetState(
        pvwindow->GetIntRegisteryValue(2, "RunTime", 
                                       "UseCompressionInComposite"));
      this->CompositeCompressionCallback();
      }
    else
      {
      this->CompositeCompressionCheck->SetState(1);
      }
    this->CompositeCompressionCheck->SetBalloonHelpString(
      "Toggle the use of run length encoding when compositing. "
      "This is here to compare performance.  "
      "It should not change the final rendered image.");
  
    this->Script("pack %s %s %s -side top -anchor w",
                 this->CompositeWithFloatCheck->GetWidgetName(),
                 this->CompositeWithRGBACheck->GetWidgetName(),
                 this->CompositeCompressionCheck->GetWidgetName());
    }

  this->ParaViewOptionsFrame->SetParent(this->GeneralProperties->GetFrame());
  this->ParaViewOptionsFrame->ShowHideFrameOn();
  this->ParaViewOptionsFrame->Create(this->Application);
  this->ParaViewOptionsFrame->SetLabel("ParaView Options");
  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->ParaViewOptionsFrame->GetWidgetName());

  this->SelectionWindowButton->SetParent(
    this->ParaViewOptionsFrame->GetFrame());
  this->SelectionWindowButton->Create(this->Application, 0);
  this->SelectionWindowButton->SetText("Selection Window");
  this->SelectionWindowButton->SetCommand(
    this, "ShowSelectionWindowCallback 1");

  this->NavigationWindowButton->SetParent(
    this->ParaViewOptionsFrame->GetFrame());
  this->NavigationWindowButton->Create(this->Application, 0);
  this->NavigationWindowButton->SetText("Navigation Window");
  this->NavigationWindowButton->SetCommand(
    this, "ShowNavigationWindowCallback 1");

  this->Script("pack %s %s -side top -padx 2 -pady 2 -anchor w",
               this->SelectionWindowButton->GetWidgetName(),
               this->NavigationWindowButton->GetWidgetName());

  this->Display3DWidgets->SetParent(
    this->ParaViewOptionsFrame->GetFrame());
  this->Display3DWidgets->Create(this->Application, 0);
  this->Display3DWidgets->SetText("Display 3D Widgets Automatically");
  this->Display3DWidgets->SetCommand(this, "Display3DWidgetsCallback");

  this->Script("pack %s -side top -padx 2 -pady 2 -anchor w",
               this->Display3DWidgets->GetWidgetName());

  this->Notebook->AddPage("Camera", "Camera and viewing navigation properties page");
  vtkKWWidget* page = this->Notebook->GetFrame("Camera");

  vtkKWFrame* frame = vtkKWFrame::New();
  frame->SetParent(page);
  frame->Create(this->Application, 1);
  this->Script("pack %s -fill both -expand yes", 
               frame->GetWidgetName());

  this->StandardViewsFrame->SetParent( frame->GetFrame() );
  this->StandardViewsFrame->Create(this->Application);
  this->StandardViewsFrame->SetLabel("Standard Views");

  this->XMaxViewButton->SetParent(this->StandardViewsFrame->GetFrame());
  this->XMaxViewButton->SetLabel("+X");
  this->XMaxViewButton->Create(this->Application, "");
  this->XMaxViewButton->SetCommand(this, "StandardViewCallback 1 0 0");
  this->Script("grid configure %s -column 0 -row 0 -padx 2 -pady 2 -ipadx 5 -sticky ew",
               this->XMaxViewButton->GetWidgetName());
  this->XMinViewButton->SetParent(this->StandardViewsFrame->GetFrame());
  this->XMinViewButton->SetLabel("-X");
  this->XMinViewButton->Create(this->Application, "");
  this->XMinViewButton->SetCommand(this, "StandardViewCallback -1 0 0");
  this->Script("grid configure %s -column 0 -row 1 -padx 2 -pady 2 -ipadx 5 -sticky ew",
               this->XMinViewButton->GetWidgetName());

  this->YMaxViewButton->SetParent(this->StandardViewsFrame->GetFrame());
  this->YMaxViewButton->SetLabel("+Y");
  this->YMaxViewButton->Create(this->Application, "");
  this->YMaxViewButton->SetCommand(this, "StandardViewCallback 0 1 0");
  this->Script("grid configure %s -column 1 -row 0 -padx 2 -pady 2 -ipadx 5 -sticky ew",
               this->YMaxViewButton->GetWidgetName());
  this->YMinViewButton->SetParent(this->StandardViewsFrame->GetFrame());
  this->YMinViewButton->SetLabel("-Y");
  this->YMinViewButton->Create(this->Application, "");
  this->YMinViewButton->SetCommand(this, "StandardViewCallback 0 -1 0");
  this->Script("grid configure %s -column 1 -row 1 -padx 2 -pady 2 -ipadx 5 -sticky ew",
               this->YMinViewButton->GetWidgetName());

  this->ZMaxViewButton->SetParent(this->StandardViewsFrame->GetFrame());
  this->ZMaxViewButton->SetLabel("+Z");
  this->ZMaxViewButton->Create(this->Application, "");
  this->ZMaxViewButton->SetCommand(this, "StandardViewCallback 0 0 1");
  this->Script("grid configure %s -column 2 -row 0 -padx 2 -pady 2 -ipadx 5 -sticky ew",
               this->ZMaxViewButton->GetWidgetName());
  this->ZMinViewButton->SetParent(this->StandardViewsFrame->GetFrame());
  this->ZMinViewButton->SetLabel("-Z");
  this->ZMinViewButton->Create(this->Application, "");
  this->ZMinViewButton->SetCommand(this, "StandardViewCallback 0 0 -1");
  this->Script("grid configure %s -column 2 -row 1 -padx 2 -pady 2 -ipadx 5 -sticky ew",
               this->ZMinViewButton->GetWidgetName());
  this->ManipulatorControl2D->SetParent(frame->GetFrame());
  this->ManipulatorControl2D->Create(pvapp, 0);
  this->ManipulatorControl2D->SetLabel("2D Movements");
  this->ManipulatorControl3D->SetParent(frame->GetFrame());
  this->ManipulatorControl3D->Create(pvapp, 0);
  this->ManipulatorControl3D->SetLabel("3D Movements");

  int cc;
  this->CameraIconsFrame->SetParent(frame->GetFrame());
  this->CameraIconsFrame->ShowHideFrameOn();
  this->CameraIconsFrame->Create(this->Application);
  this->CameraIconsFrame->SetLabel("Stored Camera Positions");
  vtkKWWidget* cframe = this->CameraIconsFrame->GetFrame();
  for ( cc = 0; cc < 6; cc ++ )
    {
    int x, y;
    this->CameraIcons[cc]->SetRenderView(this);
    this->CameraIcons[cc]->SetParent(cframe);
    this->CameraIcons[cc]->Create(this->Application);

    x = cc % 3;
    y = cc / 3;

    this->Script("grid configure %s -column %d -row %d "
                 "-padx 0 -pady 0 -ipadx 0 -ipady 0 -sticky news",
                 this->CameraIcons[cc]->GetWidgetName(),
                 x, y);
                 
    }

  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->StandardViewsFrame->GetWidgetName());
  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->CameraIconsFrame->GetWidgetName());
  this->Script("pack %s %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->ManipulatorControl2D->GetWidgetName(),
               this->ManipulatorControl3D->GetWidgetName());

  frame->Delete();

  this->Notebook->Raise("General");

  if ( this->Application->BooleanRegisteryCheck(2, "SourcesBrowser",
                                                "SelectionWindow") )
    {
    this->ShowSelectionWindowCallback(0);
    }
  else
    {
    this->ShowNavigationWindowCallback(0);
    }

  if ( !this->Application->GetRegisteryValue(2, "RunTime", "Display3DWidgets", 0) ||
       pvwindow->GetIntRegisteryValue(2, "RunTime", "Display3DWidgets") )
    {
    this->SetDisplay3DWidgets(1);
    }
  else
    {
    this->SetDisplay3DWidgets(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StandardViewCallback(float x, float y, float z)
{
  vtkCamera *cam = this->Renderer->GetActiveCamera();
  cam->SetFocalPoint(0.0, 0.0, 0.0);
  cam->SetPosition(x, y, z);
  if (x == 0.0)
    {
    cam->SetViewUp(1.0, 0.0, 0.0);
    }
  else
    {
    cam->SetViewUp(0.0, 1.0, 0.0);
    }

  this->GetRenderer()->ResetCamera();
  this->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVRenderView::FrameRateScaleCallback()
{
  float newRate = this->FrameRateScale->GetValue();
  if (newRate <= 0.0)
    {
    newRate = 0.00001;
    }
  this->SetInteractiveUpdateRate(newRate);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ReductionCheckCallback()
{
  int reduce = this->ReductionCheck->GetState();
  this->SetUseReductionFactor(reduce);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::UpdateNavigationWindow(vtkPVSource *currentSource, 
                                             int nobind)
{
  if (this->NavigationWindow)
    {
    this->NavigationWindow->Update(currentSource, nobind);
    }
  if (this->SelectionWindow)
    {
    this->SelectionWindow->Update(currentSource, nobind);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackgroundColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Set the color of the interface button.
  this->BackgroundColor->SetColor(r, g, b);
  // Since setting the color of the button from a script does
  // not invoke the callback, We also trace the view.
  this->AddTraceEntry("$kw(%s) SetBackgroundColor %f %f %f",
                      this->GetTclName(), r, g, b);
  pvApp->BroadcastScript("%s SetBackground %f %f %f",
                         this->RendererTclName, r, g, b);
  this->EventuallyRender();
}

//----------------------------------------------------------------------------
// a litle more complex than just "bind $widget <Expose> {%W Render}"
// we have to handle all pending expose events otherwise they que up.
void vtkPVRenderView::Exposed()
{
  if (this->InExpose) return;
  this->InExpose = 1;
  this->Script("update");
  this->EventuallyRender();
  this->InExpose = 0;
}

//----------------------------------------------------------------------------
// called on Configure event. Configure events might not generate an
// expose event if the size of the view gets smaller.
// At the moment, just call Exposed(), which does what we want to do,
// i.e. eventually render. If an Expose event was also generated after
// that Configure event, it will be discard because of the InExpose ivar
// logic (see above)

void vtkPVRenderView::Configured()
{
  this->Exposed();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Update()
{
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ComputeVisiblePropBounds(float bounds[6])
{ 
  if (this->Composite)
    {
    this->Composite->ComputeVisiblePropBounds(this->GetRenderer(), bounds);
    }
  else
    {
    this->GetRenderer()->ComputeVisiblePropBounds(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCamera()
{
  vtkCamera *cam;
  double *n;
  double mag2;

  // Lets see if we can correct the situation when camera ivars go arwy.
  // Unfortunately, I cannot reproduce the problem.
  cam = this->GetRenderer()->GetActiveCamera();
  n = cam->GetViewPlaneNormal();
  mag2 = n[0]*n[0] + n[1]*n[1] + n[2]*n[2];
  if (mag2 > 99999.0 || mag2 < -99999.0)
    {
    // Must be a problem.
    cam->SetPosition(0.0, 0.0, -1.0);
    cam->SetFocalPoint(0.0, 0.0, -1.0);
    cam->SetViewUp(0.0, 1.0, 0.0);
    }

  this->GetRenderer()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetCameraState(float p0, float p1, float p2,
                                     float fp0, float fp1, float fp2,
                                     float up0, float up1, float up2)
{
  vtkCamera *cam; 
  
  // This is to trace effects of loaded scripts. 
  this->AddTraceEntry(
    "$kw(%s) SetCameraState %.3f %.3f %.3f  %.3f %.3f %.3f  %.3f %.3f %.3f", 
    this->GetTclName(), p0, p1, p2, fp0, fp1, fp2, up0, up1, up2); 
  
  cam = this->GetRenderer()->GetActiveCamera(); 
  cam->SetPosition(p0, p1, p2); 
  cam->SetFocalPoint(fp0, fp1, fp2); 
  cam->SetViewUp(up0, up1, up2); 
  
  this->EventuallyRender(); 
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCameraClippingRange()
{
  // Avoid serialization.
#ifdef VTK_USE_MPI
  this->Composite->ResetCameraClippingRange(this->GetRenderer());
#else
  // If not parallel, forward to the renderer.
  this->GetRenderer()->ResetCameraClippingRange();
#endif
}

void vtkPVRenderView::AddBindings()
{
  this->Script("bind %s <Motion> {%s MotionCallback %%x %%y}",
               this->VTKWidget->GetWidgetName(), this->GetTclName());
}
    
//----------------------------------------------------------------------------
vtkPVApplication* vtkPVRenderView::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}


//----------------------------------------------------------------------------
void vtkPVRenderView::AddPVData(vtkPVData *pvc)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvc == NULL)
    {
    return;
    }  

  pvc->SetPVRenderView(this);
    
  if (pvc->GetPropTclName() != NULL)
    {
    pvApp->Script("%s AddProp %s", this->RendererTclName,
                  pvc->GetPropTclName());
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemovePVData(vtkPVData *pvc)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvc == NULL)
    {
    return;
    }

  pvc->SetPVRenderView(NULL);
  if (pvc->GetPropTclName() != NULL)
    {
    pvApp->Script("%s RemoveProp %s", this->RendererTclName,
                  pvc->GetPropTclName());
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StartRender()
{
  float renderTime = 1.0 / this->RenderWindow->GetDesiredUpdateRate();
  int *windowSize = this->RenderWindow->GetSize();
  int area, reducedArea, reductionFactor;
  float timePerPixel;
  float getBuffersTime, setBuffersTime, transmitTime;
  float newReductionFactor;
  float maxReductionFactor;
  
  if (!this->UseReductionFactor)
    {
    this->Composite->SetReductionFactor(1);
    return;
    }
  
  // Do not let the width go below 150.
  maxReductionFactor = windowSize[0] / 150.0;

  renderTime *= 0.5;
  area = windowSize[0] * windowSize[1];
  reductionFactor = this->Composite->GetReductionFactor();
  reducedArea = area / (reductionFactor * reductionFactor);
  getBuffersTime = this->Composite->GetGetBuffersTime();
  setBuffersTime = this->Composite->GetSetBuffersTime();
  transmitTime = this->Composite->GetCompositeTime();

  // Do not consider SetBufferTime because 
  //it is not dependent on reduction factor.,
  timePerPixel = (getBuffersTime + transmitTime) / reducedArea;
  newReductionFactor = sqrt(area * timePerPixel / renderTime);
  
  if (newReductionFactor > maxReductionFactor)
    {
    newReductionFactor = maxReductionFactor;
    }
  if (newReductionFactor < 1.0)
    {
    newReductionFactor = 1.0;
    }

  //cerr << "---------------------------------------------------------\n";
  //cerr << "New ReductionFactor: " << newReductionFactor << ", oldFact: " 
  //     << reductionFactor << endl;
  //cerr << "Alloc.Comp.Time: " << renderTime << ", area: " << area 
  //     << ", pixelTime: " << timePerPixel << endl;
  //cerr << "GetBufTime: " << getBuffersTime << ", SetBufTime: " << setBuffersTime
  //     << ", transTime: " << transmitTime << endl;
  
  this->Composite->SetReductionFactor((int)newReductionFactor);
}

void vtkPVRenderView::UpdateAllPVData()
{
  vtkPVWindow* pvwindow = this->GetPVWindow();
  vtkPVApplication *pvApp = this->GetPVApplication();
  if ( !pvwindow || !pvApp )
    {
    return;
    }
  vtkPVSourceCollection* col = 0;
  //cout << "Update all PVData" << endl;
  col = pvwindow->GetSourceList("Sources");
  if ( col )
    {
    vtkCollectionIterator *it = col->NewIterator();
    it->InitTraversal();
    vtkPVSource* source = 0;
    while ( !it->IsDoneWithTraversal() )
      {
      source = static_cast<vtkPVSource*>(it->GetObject());
      vtkPVData* data = source->GetPVOutput();
      if ( source->GetInitialized() && source->GetVisibility() )
        {
        data->ForceUpdate(pvApp);
        }
      it->GoToNextItem();
      }
    it->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Render()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int abort;

  this->Update();

  this->UpdateAllPVData();

  this->RenderWindow->SetDesiredUpdateRate(this->InteractiveUpdateRate);
  //this->RenderWindow->SetDesiredUpdateRate(20.0);

  // Some aborts require us to que another render.
  abort = this->ShouldIAbort();
  if (abort)
    {
    if (abort == 1)
      {
      this->EventuallyRender();
      }
    return;
    }

  if (this->Composite)
    {
    this->StartRender();
    }

  // I have this test here so that setting the frame rate slider to 0 will
  // cause full res to render.  It needs to be cleaned up.
  if (this->InteractiveUpdateRate > 0.001)
    {
    pvApp->SetGlobalLODFlag(1);
    }
  else
    {
    pvApp->SetGlobalLODFlag(0);
    }
  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");

  if (this->Composite)
    {
    this->InteractiveRenderTime = this->Composite->GetMaxRenderTime();
    this->InteractiveCompositeTime = this->Composite->GetCompositeTime()
      + this->Composite->GetGetBuffersTime()
      + this->Composite->GetSetBuffersTime();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::EventuallyRender()
{
  if (this->DisableRenderingFlag)
    {
    return;
    }

  if (this->EventuallyRenderFlag)
    {
    return;
    }
  this->EventuallyRenderFlag = 1;
  //cout << "EventuallyRender()" << endl;

  // Keep track of whether there is a render pending so that if a render is
  // pending when this object is deleted, we can cancel the "after" command.
  // We don't want to have this object register itself because this can
  // cause leaks if we exit before EventuallyRenderCallBack is called.
  this->Script("update idletasks");
  this->Script("after idle {%s EventuallyRenderCallBack}",this->GetTclName());
  this->SetRenderPending(this->Application->GetMainInterp()->result);

}
                      
//----------------------------------------------------------------------------
void vtkPVRenderView::EventuallyRenderCallBack()
{
  //cout << "EventuallyRenderCallback()" << endl;
  int abort;
  vtkPVApplication *pvApp = this->GetPVApplication();
  this->UpdateAllPVData();

  // sanity check
  if (this->EventuallyRenderFlag == 0 || !this->RenderPending)
    {
    vtkErrorMacro("Inconsistent EventuallyRenderFlag");
    return;
    }
  this->EventuallyRenderFlag = 0;
  this->RenderWindow->SetDesiredUpdateRate(
    this->GetPVWindow()->GetGenericInteractor()->GetStillUpdateRate());

  // I do not know if these are necessary here.
  abort = this->ShouldIAbort();
  if (abort)
    {
    if (abort == 1)
      {
      this->EventuallyRender();
      }
    return;
    }

  this->ResetCameraClippingRange();
  if (this->Composite)
    {
    this->StartRender();
    }

  pvApp->SetGlobalLODFlag(0);
  vtkTimerLog::MarkStartEvent("Still Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");

  if (this->Composite)
    {
    this->StillRenderTime = this->Composite->GetMaxRenderTime();
    this->StillCompositeTime = this->Composite->GetCompositeTime()
      + this->Composite->GetGetBuffersTime()
      + this->Composite->GetSetBuffersTime();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::TriangleStripsCallback()
{
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;
  vtkPVData *pvd;
  vtkPVApplication *pvApp;

  pvApp = this->GetPVApplication();
  pvWin = this->GetPVWindow();
  if (pvWin == NULL)
    {
    vtkErrorMacro("Missing window.");
    return;
    }
  sources = pvWin->GetSourceList("Sources");
  
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    pvd = pvs->GetPVOutput();
    pvApp->BroadcastScript("%s SetUseStrips %d",
                           pvd->GetGeometryTclName(),
                           this->TriangleStripsCheck->GetState());
    }

  if (this->TriangleStripsCheck->GetState())
    {
    vtkTimerLog::MarkEvent("--- Enable triangle strips.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable triangle strips.");
    }

}


//----------------------------------------------------------------------------
void vtkPVRenderView::ParallelProjectionCallback()
{

  if (this->ParallelProjectionCheck->GetState())
    {
    vtkTimerLog::MarkEvent("--- Enable parallel projection.");
    this->Renderer->GetActiveCamera()->ParallelProjectionOn();
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable parallel projection.");
    this->Renderer->GetActiveCamera()->ParallelProjectionOff();
    }
  this->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ImmediateModeCallback()
{
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;
  vtkPVData *pvd;
  vtkPVApplication *pvApp;

  pvApp = this->GetPVApplication();
  pvWin = this->GetPVWindow();
  if (pvWin == NULL)
    {
    vtkErrorMacro("Missing window.");
    return;
    }
  sources = pvWin->GetSourceList("Sources");
  
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    pvd = pvs->GetPVOutput();
    pvApp->BroadcastScript("%s SetImmediateModeRendering %d",
                           pvd->GetMapperTclName(),
                           this->ImmediateModeCheck->GetState());
    pvApp->BroadcastScript("%s SetImmediateModeRendering %d",
                           pvd->GetLODMapperTclName(),
                           this->ImmediateModeCheck->GetState());
    }

  if (this->ImmediateModeCheck->GetState())
    {
    vtkTimerLog::MarkEvent("--- Disable display lists.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Enable display lists.");
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderView::LODThresholdScaleCallback()
{
  float value = this->LODThresholdScale->GetValue();
  int threshold;

  // Value should be between 0 and 18.
  // producing threshold between 65Mil and 1.
  threshold = static_cast<int>(exp(18.0 - value));
  
  // Use internal method so we do not reset the slider.
  // I do not know if it would cause a problem, but ...
  this->SetLODThresholdInternal(threshold);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %d.", 
                                  threshold);
  this->AddTraceEntry("$kw(%s) SetLODThreshold %d",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetLODThreshold(int threshold)
{
  float value;
  
  if (threshold <= 0.0)
    {
    value = VTK_LARGE_FLOAT;
    }
  else
    {
    value = 18.0 - log((double)(threshold));
    }
  this->LODThresholdScale->SetValue(value);

  this->SetLODThresholdInternal(threshold);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %d.", 
                                  threshold);
  this->AddTraceEntry("$kw(%s) SetLODThreshold %d",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetLODThresholdInternal(int threshold)
{
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;
  vtkPVData *pvd;
  vtkPVApplication *pvApp;
  char str[256];

  sprintf(str, " %d Points", threshold);
  this->LODThresholdValue->SetLabel(str);

  this->LODThreshold = threshold;

  pvApp = this->GetPVApplication();
  pvWin = this->GetPVWindow();
  if (pvWin == NULL)
    {
    vtkErrorMacro("Missing window.");
    return;
    }
  sources = pvWin->GetSourceList("Sources");
  sources->InitTraversal();

  while ( (pvs = sources->GetNextPVSource()) )
    {
    if (pvs->GetInitialized())
      {
      pvd = pvs->GetPVOutput();
      // The performs the check and enables or disables decimation LOD.
      pvd->UpdateProperties();
      }
    }
}




//----------------------------------------------------------------------------
void vtkPVRenderView::LODResolutionScaleCallback()
{
  int value = static_cast<int>(this->LODResolutionScale->GetValue());
  value = 170 - value;

  // Use internal method so we do not reset the slider.
  // I do not know if it would cause a problem, but ...
  this->SetLODResolutionInternal(value);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Resolution %d.", value);
  this->AddTraceEntry("$kw(%s) SetLODResolution %d",
                      this->GetTclName(), value);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetLODResolution(int value)
{
  this->LODResolutionScale->SetValue(150 - value);

  this->SetLODResolutionInternal(value);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Resolution %d.", value);
  this->AddTraceEntry("$kw(%s) SetLODResolution %d",
                      this->GetTclName(), value);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetLODResolutionInternal(int resolution)
{
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;
  vtkPVData *pvd;
  vtkPVApplication *pvApp;
  char str[256];

  sprintf(str, " %dx%dx%d", resolution, resolution, resolution);
  this->LODResolutionValue->SetLabel(str);

  this->LODResolution = resolution;

  pvApp = this->GetPVApplication();
  pvWin = this->GetPVWindow();
  if (pvWin == NULL)
    {
    vtkErrorMacro("Missing window.");
    return;
    }
  sources = pvWin->GetSourceList("Sources");
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    pvd = pvs->GetPVOutput();
    pvd->SetLODResolution(resolution);
    }
}





//----------------------------------------------------------------------------
void vtkPVRenderView::CollectThresholdScaleCallback()
{
  float threshold = this->CollectThresholdScale->GetValue();

  // Use internal method so we do not reset the slider.
  // I do not know if it would cause a problem, but ...
  this->SetCollectThresholdInternal(threshold);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %f.", threshold);
  this->AddTraceEntry("$kw(%s) SetCollectThreshold %f",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetCollectThreshold(float threshold)
{
  this->CollectThresholdScale->SetValue(threshold);

  this->SetCollectThresholdInternal(threshold);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %f.", threshold);
  this->AddTraceEntry("$kw(%s) SetCollectThreshold %f",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetCollectThresholdInternal(float threshold)
{
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;
  vtkPVData *pvd;
  vtkPVApplication *pvApp;
  char str[256];

  sprintf(str, " %.1f MBytes", threshold);
  this->CollectThresholdValue->SetLabel(str);

  this->CollectThreshold = threshold;

  pvApp = this->GetPVApplication();
  pvWin = this->GetPVWindow();
  if (pvWin == NULL)
    {
    vtkErrorMacro("Missing window.");
    return;
    }
  sources = pvWin->GetSourceList("Sources");
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    pvd = pvs->GetPVOutput();
    pvd->SetCollectThreshold(threshold);
    }
}




//----------------------------------------------------------------------------
void vtkPVRenderView::InterruptRenderCallback()
{
  if (this->Composite)
    {
    vtkPVApplication* pvApp = this->GetPVApplication();
    if (pvApp->GetController()->GetNumberOfProcesses() > 1 )
      {
      pvApp->BroadcastScript(
        "%s SetEnableAbort %d",this->CompositeTclName,
        this->InterruptRenderCheck->GetState());
      }
    else
      {
      if (this->InterruptRenderCheck->GetState())
        {
        this->RenderWindow->SetAbortCheckMethod(
          PVRenderViewAbortCheck, (void*)this);
        }
      else
        {
        this->RenderWindow->SetAbortCheckMethod(0, 0);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CompositeWithFloatCallback()
{
  int val = this->CompositeWithFloatCheck->GetState();
  this->CompositeWithFloatCallback(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CompositeWithFloatCallback(int val)
{
  this->AddTraceEntry("$kw(%s) CompositeWithFloatCallback %d", 
                      this->GetTclName(), val);
  if ( this->CompositeWithFloatCheck->GetState() != val )
    {
    this->CompositeWithFloatCheck->SetState(val);
    }
 
  if (this->Composite)
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseChar %d",
                                              this->CompositeTclName,
                                              !val);
    // Limit of composite manager.
    if (val != 0) // float
      {
      this->CompositeWithRGBACheck->SetState(1);
      }
    this->EventuallyRender();
    }

  if (this->CompositeWithFloatCheck->GetState())
    {
    vtkTimerLog::MarkEvent("--- Get color buffers as floats.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Get color buffers as unsigned char.");
    }

}

//----------------------------------------------------------------------------
void vtkPVRenderView::CompositeWithRGBACallback()
{
  int val = this->CompositeWithRGBACheck->GetState();
  this->CompositeWithRGBACallback(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CompositeWithRGBACallback(int val)
{
  this->AddTraceEntry("$kw(%s) CompositeWithRGBACallback %d", 
                      this->GetTclName(), val);
  if ( this->CompositeWithRGBACheck->GetState() != val )
    {
    this->CompositeWithRGBACheck->SetState(val);
    }
  if (this->Composite)
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseRGB %d",
                                              this->CompositeTclName,
                                              !val);
    // Limit of composite manager.
    if (val != 1) // RGB
      {
      this->CompositeWithFloatCheck->SetState(0);
      }
    this->EventuallyRender();
    }

  if (this->CompositeWithRGBACheck->GetState())
    {
    vtkTimerLog::MarkEvent("--- Use RGBA pixels to get color buffers.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Use RGB pixels to get color buffers.");
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderView::CompositeCompressionCallback()
{
  int val = this->CompositeCompressionCheck->GetState();
  this->CompositeCompressionCallback(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CompositeCompressionCallback(int val)
{
  this->AddTraceEntry("$kw(%s) CompositeCompressionCallback %d", 
                      this->GetTclName(), val);
  if ( this->CompositeCompressionCheck->GetState() != val )
    {
    this->CompositeCompressionCheck->SetState(val);
    }
  if (this->Composite)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    if (val)
      {
      pvApp->BroadcastScript("vtkCompressCompositer pvTemp");
      }
    else
      {
      pvApp->BroadcastScript("vtkTreeCompositer pvTemp");
      }
    pvApp->BroadcastScript("%s SetCompositer pvTemp", this->CompositeTclName);
    pvApp->BroadcastScript("pvTemp Delete");
    this->EventuallyRender();
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable compression when compositing.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable compression when compositing.");
    }

}


//----------------------------------------------------------------------------
vtkPVWindow *vtkPVRenderView::GetPVWindow()
{
  vtkPVWindow *pvWin = vtkPVWindow::SafeDownCast(this->GetParentWindow());

  return pvWin;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SaveInTclScript(ofstream *file,
                                      int interactiveFlag, int vtkFlag)
{
  vtkCamera *camera;
  float position[3];
  float focalPoint[3];
  float viewUp[3];
  float viewAngle;
  float clippingRange[2];
  float *color;
  int *size;

  size = this->RenderWindow->GetSize();
  if (vtkFlag)
    {
    *file << "vtkRenderer " << this->RendererTclName << "\n\t";
    color = this->Renderer->GetBackground();
    *file << this->RendererTclName << " SetBackground "
          << color[0] << " " << color[1] << " " << color[2] << endl;
    *file << "vtkRenderWindow " << this->RenderWindowTclName << "\n\t"
          << this->RenderWindowTclName << " AddRenderer "
          << this->RendererTclName << "\n\t";
    *file << this->RenderWindowTclName << " SetSize " << size[0] << " " << size[1] << endl;
    if (vtkFlag && interactiveFlag)
      {
      *file << "vtkRenderWindowInteractor iren\n\t"
            << "iren SetRenderWindow " << this->RenderWindowTclName << "\n\n";
      }
    }

  camera = this->GetRenderer()->GetActiveCamera();
  camera->GetPosition(position);
  camera->GetFocalPoint(focalPoint);
  camera->GetViewUp(viewUp);
  viewAngle = camera->GetViewAngle();
  camera->GetClippingRange(clippingRange);
  
  *file << "# camera parameters\n"
        << "set camera [" << this->RendererTclName << " GetActiveCamera]\n\t"
        << "$camera SetPosition " << position[0] << " " << position[1] << " "
        << position[2] << "\n\t"
        << "$camera SetFocalPoint " << focalPoint[0] << " " << focalPoint[1]
        << " " << focalPoint[2] << "\n\t"
        << "$camera SetViewUp " << viewUp[0] << " " << viewUp[1] << " "
        << viewUp[2] << "\n\t"
        << "$camera SetViewAngle " << viewAngle << "\n\t"
        << "$camera SetClippingRange " << clippingRange[0] << " "
        << clippingRange[1] << "\n";
}


//----------------------------------------------------------------------------
int* vtkPVRenderView::GetRenderWindowSize()
{
  if ( this->GetRenderWindow() )
    {
    return this->GetRenderWindow()->GetSize();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::UpdateCameraManipulators()
{
  vtkPVInteractorStyleControl *iscontrol3D = this->GetManipulatorControl3D();
  vtkPVInteractorStyleControl *iscontrol2D = this->GetManipulatorControl2D();
  iscontrol3D->UpdateMenus();
  iscontrol2D->UpdateMenus();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetupCameraManipulators()
{
  vtkPVInteractorStyleControl *iscontrol3D = this->GetManipulatorControl3D();
  vtkPVInteractorStyleControl *iscontrol2D = this->GetManipulatorControl2D();

  iscontrol3D->SetCurrentManipulator(0, 0, "Rotate");
  iscontrol3D->SetCurrentManipulator(1, 0, "Pan");
  iscontrol3D->SetCurrentManipulator(2, 0, "Zoom");
  iscontrol3D->SetCurrentManipulator(0, 1, "Roll");
  iscontrol3D->SetCurrentManipulator(1, 1, "Center");
  iscontrol3D->SetCurrentManipulator(2, 1, "Pan");
  iscontrol3D->SetCurrentManipulator(0, 2, "FlyIn");
  iscontrol3D->SetCurrentManipulator(2, 2, "FlyOut");
  iscontrol3D->SetDefaultManipulator("Rotate");
  iscontrol3D->UpdateMenus();

  iscontrol2D->SetCurrentManipulator(0, 1, "Roll");
  iscontrol2D->SetCurrentManipulator(1, 0, "Pan");
  iscontrol2D->SetCurrentManipulator(2, 1, "Pan");
  iscontrol2D->SetCurrentManipulator(2, 0, "Zoom");
  iscontrol2D->SetDefaultManipulator("Pan");
  iscontrol2D->UpdateMenus();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StoreCurrentCamera(int position)
{
  if ( this->CameraIcons[position] )
    {
    this->CameraIcons[position]->StoreCamera();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RestoreCurrentCamera(int position)
{
  if ( this->CameraIcons[position] )
    {
    this->CameraIcons[position]->RestoreCamera();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SerializeSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeSelf(os, indent);
  vtkCamera* cam = this->GetRenderer()->GetActiveCamera();
  os << indent << "CameraParallelScale " << cam->GetParallelScale() << endl;
  os << indent << "CameraViewAngle " << cam->GetViewAngle() << endl;
  os << indent << "CameraClippingRange " << cam->GetClippingRange()[0] << " "
     << cam->GetClippingRange()[1] << endl;
  os << indent << "CameraFocalPoint " 
     << cam->GetFocalPoint()[0] << " "
     << cam->GetFocalPoint()[1] << " " 
     << cam->GetFocalPoint()[2] << endl;
  os << indent << "CameraPosition " 
     << cam->GetPosition()[0] << " "
     << cam->GetPosition()[1] << " " 
     << cam->GetPosition()[2] << endl;
  os << indent << "CameraViewUp " 
     << cam->GetViewUp()[0] << " "
     << cam->GetViewUp()[1] << " " 
     << cam->GetViewUp()[2] << endl;
}

//------------------------------------------------------------------------------
void vtkPVRenderView::SerializeRevision(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeRevision(os,indent);
  os << indent << "vtkPVRenderView ";
  this->ExtractRevision(os,"$Revision: 1.206 $");
}

//------------------------------------------------------------------------------
void vtkPVRenderView::SerializeToken(istream& is, const char token[1024])
{
  int cc;
  if ( vtkString::Equals(token, "CameraPosition") )
    {
    float cor[3];
    for ( cc = 0; cc < 3; cc ++ )
      {
      cor[cc] = 0.0;
      if (! (is >> cor[cc]) )
        {
        vtkErrorMacro("Problem Parsing session file");
        }
      }
    this->Renderer->GetActiveCamera()->SetPosition(cor);
    }
  else if ( vtkString::Equals(token, "CameraFocalPoint") )
    {
    float cor[3];
    for ( cc = 0; cc < 3; cc ++ )
      {
      cor[cc] = 0.0;
      if (! (is >> cor[cc]) )
        {
        vtkErrorMacro("Problem Parsing session file");
        }
      }
    this->Renderer->GetActiveCamera()->SetFocalPoint(cor);
    }
  else if ( vtkString::Equals(token, "CameraViewUp") )
    {
    float cor[3];
    for ( cc = 0; cc < 3; cc ++ )
      {
      cor[cc] = 0.0;
      if (! (is >> cor[cc]) )
        {
        vtkErrorMacro("Problem Parsing session file");
        }
      }
    this->Renderer->GetActiveCamera()->SetViewUp(cor);
    }
  else if ( vtkString::Equals(token, "CameraClippingRange") )
    {
    float cor[2];
    for ( cc = 0; cc < 2; cc ++ )
      {
      cor[cc] = 0.0;
      if (! (is >> cor[cc]) )
        {
        vtkErrorMacro("Problem Parsing session file");
        }
      }
    this->Renderer->GetActiveCamera()->SetClippingRange(cor);
    }
  else if ( vtkString::Equals(token, "CameraParallelScale") )
    {
    float cor;
    cor = 0.0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->Renderer->GetActiveCamera()->SetParallelScale(cor);
    }
  else if ( vtkString::Equals(token, "CameraViewAngle") )
    {
    float cor;
    cor = 0.0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->Renderer->GetActiveCamera()->SetViewAngle(cor);
    }
  else
    {
    //cout << "Unknown Token for " << this->GetClassName() << ": " 
    //     << token << endl;
    this->Superclass::SerializeToken(is,token);  
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SaveAsImage(const char* filename) 
{
  this->EventuallyRender();
  this->Script("update");
  this->Superclass::SaveAsImage(filename);
}


//----------------------------------------------------------------------------
void vtkPVRenderView::ExecuteEvent(vtkObject*, unsigned long event, void* par)
{
  if ( event == vtkCommand::CursorChangedEvent )
    {
    int val = *(static_cast<int*>(par));
    const char* image = "left_ptr";
    switch ( val ) 
      {
      case VTK_CURSOR_ARROW:
        image = "arror";
        break;
      case VTK_CURSOR_SIZENE:
        image = "top_right_corner";
        break;
      case VTK_CURSOR_SIZENW:        
        image = "top_left_corner";
        break;
      case VTK_CURSOR_SIZESW:
        image = "bottom_left_corner";
        break;
      case VTK_CURSOR_SIZESE:
        image = "bottom_right_corner";
        break;
      case VTK_CURSOR_SIZENS:
        image = "sb_v_double_arrow";
        break;
      case VTK_CURSOR_SIZEWE:
        image = "sb_h_double_arrow";
        break;
      case VTK_CURSOR_SIZEALL:
        image = "hand2";
        break;
      case VTK_CURSOR_HAND:
        image = "hand2";
        break;
      }
    this->Script("%s config -cursor %s", 
                 this->GetPVWindow()->GetWidgetName(), image);
    } 
}

//----------------------------------------------------------------------------
void vtkPVRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ImmediateModeCheck: " 
     << this->GetImmediateModeCheck() << endl;
  os << indent << "InteractiveCompositeTime: " 
     << this->GetInteractiveCompositeTime() << endl;
  os << indent << "InteractiveRenderTime: " 
     << this->GetInteractiveRenderTime() << endl;
  os << indent << "SplitFrame: " 
     << this->GetSplitFrame() << endl;
  os << indent << "NavigationFrame: " 
     << this->GetNavigationFrame() << endl;
  os << indent << "RendererTclName: " 
     << (this->GetRendererTclName()?this->GetRendererTclName():"<none>") << endl;
  os << indent << "StillCompositeTime: " 
     << this->GetStillCompositeTime() << endl;
  os << indent << "StillRenderTime: " << this->GetStillRenderTime() << endl;
  os << indent << "TriangleStripsCheck: " 
     << this->GetTriangleStripsCheck() << endl;
  os << indent << "UseReductionFactor: " 
     << this->GetUseReductionFactor() << endl;
  os << indent << "DisableRenderingFlag: " 
     << (this->DisableRenderingFlag ? "on" : "off") << endl;
  os << indent << "ManipulatorControl2D: " 
     << this->ManipulatorControl2D << endl;
  os << indent << "ManipulatorControl3D: " 
     << this->ManipulatorControl3D << endl;
  os << indent << "LODThreshold: " << this->LODThreshold << endl;
  os << indent << "LODResolution: " << this->LODResolution << endl;
  os << indent << "CollectThreshold: " << this->CollectThreshold << endl;
}

