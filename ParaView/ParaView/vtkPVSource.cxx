/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSource.cxx
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
#include "vtkPVSource.h"

#include "vtkArrayMap.txx"
#include "vtkCollectionIterator.h"
#include "vtkDataSet.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWPushButton.h"
#include "vtkKWSerializer.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVInputMenu.h"
#include "vtkPVRenderView.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkVector.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSource);
vtkCxxRevisionMacro(vtkPVSource, "1.248.2.13");

int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

vtkCxxSetObjectMacro(vtkPVSource, View, vtkKWView);

//----------------------------------------------------------------------------
vtkPVSource::vtkPVSource()
{
  this->CommandFunction = vtkPVSourceCommand;

  // Number of instances cloned from this prototype
  this->PrototypeInstanceCount = 0;

  this->Name = 0;
  this->Label = 0;
  this->ModuleName = 0;
  this->MenuName = 0;
  this->ShortHelp = 0;
  this->LongHelp  = 0;

  // Initialize the data only after  Accept is invoked for the first time.
  // This variable is used to determine that.
  this->Initialized = 0;

  // The notebook which holds Parameters, Display and Information pages.
  this->Notebook = vtkKWNotebook::New();
  this->Notebook->AlwaysShowTabsOn();

  this->PVInputs = NULL;
  this->NumberOfPVInputs = 0;
  this->PVOutputs = NULL;
  this->NumberOfPVOutputs = 0;

  // The underlying VTK object. This will change. PVSource will
  // support multiple VTK sources/filters.
  this->VTKSource = NULL;
  this->VTKSourceTclName = NULL;

  // The frame which contains the parameters related to the data source
  // and the Accept/Reset/Delete buttons.
  this->Parameters = vtkKWWidget::New();
  
  this->ParameterFrame = vtkKWFrame::New();
  this->ButtonFrame = vtkKWWidget::New();
  this->MainParameterFrame = vtkKWWidget::New();
  this->AcceptButton = vtkKWPushButton::New();
  this->ResetButton = vtkKWPushButton::New();
  this->DeleteButton = vtkKWPushButton::New();

  this->DescriptionFrame = vtkKWWidget::New();
  this->NameLabel = vtkKWLabeledLabel::New();
  this->TypeLabel = vtkKWLabeledLabel::New();
  this->LongHelpLabel = vtkKWLabeledLabel::New();
  this->LabelEntry = vtkKWLabeledEntry::New();
      
  this->Widgets = vtkPVWidgetCollection::New();
    
  this->ExtentTranslatorTclName = NULL;

  this->ReplaceInput = 1;

  this->ParametersParent = NULL;
  this->View = NULL;

  this->VisitedFlag = 0;

  this->OutputClassName = 0;
  this->SourceClassName = 0;

  this->InputClassNames = vtkVector<const char*>::New();

  this->IsPermanent = 0;

  this->HideDisplayPage = 0;
  this->HideParametersPage = 0;
  this->HideInformationPage = 0;
  
  this->AcceptCallbackFlag = 0;

  this->SourceGrabbed = 0;

  this->ToolbarModule = 0;
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  int i;
  
  for (i = 0; i < this->NumberOfPVOutputs; i++)
    {
    if (this->PVOutputs[i])
      {
      this->PVOutputs[i]->UnRegister(this);
      this->PVOutputs[i] = NULL;
      }
    }
  
  if (this->PVOutputs)
    {
    delete [] this->PVOutputs;
    this->PVOutputs = 0;
    }
  
  this->NumberOfPVOutputs = 0;
  
  this->RemoveAllPVInputs();

  this->SetVTKSource(NULL, NULL);

  // Do not use SetName() or SetLabel() here. These make
  // the navigation window update when it should not.
  delete[] this->Name;
  delete[] this->Label;

  this->SetMenuName(0);
  this->SetShortHelp(0);
  this->SetLongHelp(0);

  // This is necessary in order to make the parent frame release it's
  // reference to the widgets. Otherwise, the widgets get deleted only
  // when the parent (usually the parameters notebook page) is deleted.
  this->Notebook->SetParent(0);
  this->Notebook->Delete();
  this->Notebook = NULL;

  this->Widgets->Delete();
  this->Widgets = NULL;

  this->AcceptButton->Delete();
  this->AcceptButton = NULL;  
  
  this->ResetButton->Delete();
  this->ResetButton = NULL;  
  
  this->DeleteButton->Delete();
  this->DeleteButton = NULL;

  this->DescriptionFrame->Delete();
  this->DescriptionFrame = NULL;

  this->NameLabel->Delete();
  this->NameLabel = NULL;

  this->TypeLabel->Delete();
  this->TypeLabel = NULL;

  this->LongHelpLabel->Delete();
  this->LongHelpLabel = NULL;

  this->LabelEntry->Delete();
  this->LabelEntry = NULL;

  this->ParameterFrame->Delete();
  this->ParameterFrame = NULL;

  this->MainParameterFrame->Delete();
  this->MainParameterFrame = NULL;

  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;

  this->Parameters->Delete();
  this->Parameters = NULL;
    

  if (this->ExtentTranslatorTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete", 
                                this->ExtentTranslatorTclName);
    this->SetExtentTranslatorTclName(NULL);
    }

  if (this->ParametersParent)
    {
    this->ParametersParent->UnRegister(this);
    this->ParametersParent = NULL;
    }
  this->SetView(NULL);

  this->SetOutputClassName(0);
  this->SetSourceClassName(0);

  this->InputClassNames->Delete();
  this->InputClassNames = 0;

  this->SetModuleName(0);

}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVSource::GetInputPVSource()
{
  vtkPVData* data = this->GetPVInput();
  if ( data )
    {
    return data->GetPVSource();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPVInput(vtkPVData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    vtkErrorMacro(
      "No Application. Create the source before setting the input.");
    return;
    }

  this->SetNthPVInput(0, pvd);

  pvApp->BroadcastScript("%s SetInput %s", this->GetVTKSourceTclName(),
                         pvd->GetVTKDataTclName());


  this->GetPVRenderView()->UpdateNavigationWindow(this, 0);

  // Try to set the actor translate
  //float *pt;
  //pvd->GetActorTranslate(pt);
  //this->GetPVOutput()->SetActorTranslate(pt);
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPVOutput(vtkPVData *pvd)
{
  this->SetNthPVOutput(0, pvd);
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void vtkPVSourceStartProgress(void* vtkNotUsed(arg))
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  //vtkSource *vtkSource = me->GetVTKSource();
  //static char str[200];
  
  //if (vtkSource && me->GetWindow())
  //  {
  //  sprintf(str, "Processing %s", vtkSource->GetClassName());
  //  me->GetWindow()->SetStatusText(str);
  //  }
}
//----------------------------------------------------------------------------
void vtkPVSourceReportProgress(void* vtkNotUsed(arg))
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  //vtkSource *vtkSource = me->GetVTKSource();

  //if (me->GetWindow())
  //  {
  //  me->GetWindow()->GetProgressGauge()->SetValue((int)(vtkSource->GetProgress() * 100));
  //  }
}
//----------------------------------------------------------------------------
void vtkPVSourceEndProgress(void* vtkNotUsed(arg))
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  
  //if (me->GetWindow())
  //  {
  //  me->GetWindow()->SetStatusText("");
  //  me->GetWindow()->GetProgressGauge()->SetValue(0);
  //  }
}

//----------------------------------------------------------------------------
// Tcl does the reference counting, so we are not going to put an 
// additional reference of the data.
void vtkPVSource::SetVTKSource(vtkSource *source, const char *tclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    return;
    }
  
  if (this->VTKSourceTclName)
    {
    pvApp->BroadcastScript("if {[info command %s] != \"\"} { %s Delete }", 
                           this->VTKSourceTclName, this->VTKSourceTclName);
    delete [] this->VTKSourceTclName;
    this->VTKSourceTclName = NULL;
    this->VTKSource = NULL;
    }
  if (tclName)
    {
    this->VTKSourceTclName = new char[strlen(tclName) + 1];
    strcpy(this->VTKSourceTclName, tclName);
    this->VTKSource = source;
    // Set up the progress methods.
    //source->SetStartMethod(vtkPVSourceStartProgress, this);
    //source->SetProgressMethod(vtkPVSourceReportProgress, this);
    //source->SetEndMethod(vtkPVSourceEndProgress, this);
    
    pvApp->Script("%s AddObserver ModifiedEvent {%s VTKSourceModifiedMethod}",
                  tclName, this->GetTclName());
    
    pvApp->BroadcastScript(
      "%s SetStartMethod {$Application LogStartEvent {Execute %s}}", 
      tclName, tclName);
    pvApp->BroadcastScript(
      "%s SetEndMethod {$Application LogEndEvent {Execute %s}}", 
      tclName, tclName);


    }
}

//----------------------------------------------------------------------------
vtkPVWindow* vtkPVSource::GetPVWindow()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp == NULL)
    {
    return NULL;
    }
  
  return pvApp->GetMainWindow();
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVSource::GetPVApplication()
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
void vtkPVSource::CreateProperties()
{
  // If the user has not set the parameters parent.
  if (this->ParametersParent == NULL)
    {
    vtkErrorMacro("ParametersParent has not been set.");
    }

  this->Notebook->SetParent(this->ParametersParent);
  this->Notebook->Create(this->Application,"");

  // Set up the pages of the notebook.
  if (!this->HideParametersPage)
    {
    this->Notebook->AddPage("Parameters");
    this->Parameters->SetParent(this->Notebook->GetFrame("Parameters"));
    }
  else
    {
    this->Parameters->SetParent(this->ParametersParent);
    }

  this->Parameters->Create(this->Application,"frame","");
  if (!this->HideParametersPage)
    {
    this->Script("pack %s -pady 2 -fill x -expand yes",
                 this->Parameters->GetWidgetName());
    }

  // For initializing the trace of the notebook.
  this->GetParametersParent()->SetTraceReferenceObject(this);
  this->GetParametersParent()->SetTraceReferenceCommand("GetParametersParent");

  // Set the description frame
  // Try to do something that looks like the parameters, i.e. fixed-width
  // labels and "expandable" values. This has to be fixed later when the
  // parameters will be properly aligned (i.e. gridded)

  this->DescriptionFrame ->SetParent(this->Parameters);
  this->DescriptionFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill both -expand t -side top -padx 2 -pady 2", 
               this->DescriptionFrame->GetWidgetName());

  const char *label1_opt = "-width 12 -anchor e";

  this->NameLabel->SetParent(this->DescriptionFrame);
  this->NameLabel->Create(this->Application);
  this->NameLabel->GetLabel1()->SetLabel("Name:");
  this->Script("%s configure -anchor w", 
               this->NameLabel->GetLabel2()->GetWidgetName());
  this->Script("%s config %s", 
               this->NameLabel->GetLabel1()->GetWidgetName(), label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->NameLabel->GetLabel2()->GetWidgetName());
  vtkKWTkUtilities::ChangeFontToBold(
    this->Application->GetMainInterp(),
    this->NameLabel->GetLabel2()->GetWidgetName());

  this->TypeLabel->SetParent(this->DescriptionFrame);
  this->TypeLabel->Create(this->Application);
  this->TypeLabel->GetLabel1()->SetLabel("Class:");
  this->Script("%s configure -anchor w", 
               this->TypeLabel->GetLabel2()->GetWidgetName());
  this->Script("%s config %s", 
               this->TypeLabel->GetLabel1()->GetWidgetName(), label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->TypeLabel->GetLabel2()->GetWidgetName());

  this->LabelEntry->SetParent(this->DescriptionFrame);
  this->LabelEntry->Create(this->Application);
  this->LabelEntry->GetLabel()->SetLabel("Label:");
  this->Script("%s config %s", 
               this->LabelEntry->GetLabel()->GetWidgetName(),label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->LabelEntry->GetEntry()->GetWidgetName());
  this->Script("bind %s <KeyPress-Return> {%s DescriptionEntryCallback}",
               this->LabelEntry->GetEntry()->GetWidgetName(), 
               this->GetTclName());

  this->LongHelpLabel->SetParent(this->DescriptionFrame);
  this->LongHelpLabel->Create(this->Application);
  this->LongHelpLabel->GetLabel1()->SetLabel("Description:");
  this->LongHelpLabel->GetLabel2()->AdjustWrapLengthToWidthOn();
  this->Script("%s configure -anchor w", 
               this->LongHelpLabel->GetLabel2()->GetWidgetName());
  this->Script("%s config %s", 
               this->LongHelpLabel->GetLabel1()->GetWidgetName(), label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->LongHelpLabel->GetLabel2()->GetWidgetName());

  this->Script("grid %s -sticky news", 
               this->NameLabel->GetWidgetName());
  this->Script("grid %s -sticky news", 
               this->TypeLabel->GetWidgetName());
  this->Script("grid %s -sticky news", 
               this->LabelEntry->GetWidgetName());
  this->Script("grid %s -sticky news", 
               this->LongHelpLabel->GetWidgetName());
  this->Script("grid columnconfigure %s 0 -weight 1", 
               this->LongHelpLabel->GetParent()->GetWidgetName());

  // The main parameter frame

  this->MainParameterFrame->SetParent(this->Parameters);
  this->MainParameterFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill both -expand t -side top", 
               this->MainParameterFrame->GetWidgetName());

  this->ButtonFrame->SetParent(this->MainParameterFrame);
  this->ButtonFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill both -expand t -side top", 
               this->ButtonFrame->GetWidgetName());

  this->ParameterFrame->SetParent(this->MainParameterFrame);

  this->ParameterFrame->Create(this->Application, 1);
  this->Script("pack %s -fill both -expand t -side top", 
               this->ParameterFrame->GetWidgetName());

  vtkKWWidget *frame = vtkKWWidget::New();
  frame->SetParent(this->ButtonFrame);
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());  
  
  this->AcceptButton->SetParent(frame);
  this->AcceptButton->Create(this->Application, 
                             "-text Accept -background red1");
  this->AcceptButton->SetCommand(this, "PreAcceptCallback");
  this->AcceptButton->SetBalloonHelpString(
    "Cause the current values in the user interface to take effect");

  this->ResetButton->SetParent(frame);
  this->ResetButton->Create(this->Application, "-text Reset");
  this->ResetButton->SetCommand(this, "ResetCallback");
  this->ResetButton->SetBalloonHelpString(
    "Revert to the previous parameters of the module.");

  this->DeleteButton->SetParent(frame);
  this->DeleteButton->Create(this->Application, "-text Delete");
  this->DeleteButton->SetCommand(this, "DeleteCallback");
  this->DeleteButton->SetBalloonHelpString(
    "Remove the current module.  "
    "This can only be done if no other modules depends on the current one.");

  this->Script("pack %s %s %s -padx 2 -pady 2 -side left -fill x -expand t",
               this->AcceptButton->GetWidgetName(), 
               this->ResetButton->GetWidgetName(), 
               this->DeleteButton->GetWidgetName());

  frame->Delete();  
 
  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateDescriptionFrame()
{
  if (!this->Application)
    {
    return;
    }

  if (this->NameLabel && this->NameLabel->IsCreated())
    {
    this->NameLabel->GetLabel2()->SetLabel(this->Name ? this->Name : "");
    }

  if (this->TypeLabel && this->TypeLabel->IsCreated())
    {
    if (this->GetVTKSource()) 
      {
      this->TypeLabel->GetLabel2()->SetLabel(
        this->GetVTKSource()->GetClassName());
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid %s", this->TypeLabel->GetWidgetName());
        }
      }
    else
      {
      this->TypeLabel->GetLabel2()->SetLabel("");
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid remove %s", this->TypeLabel->GetWidgetName());
        }
      }
    }

  if (this->LabelEntry && this->LabelEntry->IsCreated())
    {
    this->LabelEntry->GetEntry()->SetValue(this->Label);
    }

  if (this->LongHelpLabel && this->LongHelpLabel->IsCreated())
    {
    if (this->LongHelp && 
        !(this->GetPVWindow() && 
          !this->GetPVWindow()->GetShowSourcesLongHelp())) 
      {
      this->LongHelpLabel->GetLabel2()->SetLabel(this->LongHelp);
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid %s", this->LongHelpLabel->GetWidgetName());
        }
      }
    else
      {
      this->LongHelpLabel->GetLabel2()->SetLabel("");
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid remove %s", this->LongHelpLabel->GetWidgetName());
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::GrabFocus()
{
  this->SourceGrabbed = 1;

  this->GetPVWindow()->DisableToolbarButtons();
  this->GetPVWindow()->DisableMenus();
  this->GetPVRenderView()->UpdateNavigationWindow(this, 1);
}

//----------------------------------------------------------------------------
void vtkPVSource::UnGrabFocus()
{

  if ( this->SourceGrabbed )
    {
    this->GetPVWindow()->EnableToolbarButtons();
    this->GetPVWindow()->EnableMenus();
    this->GetPVRenderView()->UpdateNavigationWindow(this, 0);
    }
  this->SourceGrabbed = 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::Pack()
{
  // The update is needed to work around a packing problem which
  // occur for large windows. Do not remove.
  this->Script("update");

  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ParametersParent->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand t",
               this->GetPVRenderView()->GetNavigationFrame()->GetWidgetName());
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVSource::Select()
{
  this->Pack();

  vtkPVData *data;
  
  this->UpdateProperties();
  // This may best be merged with the UpdateProperties call but ...
  // We make the call here to update the input menu, 
  // which is now just another pvWidget.
  this->UpdateParameterWidgets();
  
  // This assumes that a source only has one (visible) output.
  // This assumption should normally hold because when the
  // source has multiple outputs, the design requires creating
  // "connection" objects for each output. The actual outputs
  // of a multi-output source are not visible.
  data = this->GetPVOutput();
  if (data)
    {
    // Update the Display and Information page.
    data->UpdateProperties();
    }

  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->UpdateNavigationWindow(this, this->SourceGrabbed);
    }

  int i;
  vtkPVWidget *pvw = 0;
  this->Widgets->InitTraversal();
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvw = this->Widgets->GetNextPVWidget();
    pvw->Select();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Deselect(int doPackForget)
{
  if (doPackForget)
    {
    this->Script("pack forget %s", this->Notebook->GetWidgetName());
    }
  int i;
  vtkPVWidget *pvw = 0;
  this->Widgets->InitTraversal();
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvw = this->Widgets->GetNextPVWidget();
    pvw->Deselect();
    }
}


//----------------------------------------------------------------------------
char* vtkPVSource::GetName()
{
  return this->Name;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetName (const char* arg) 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " 
                << this->Name << " to " << arg ); 
  if ( this->Name && arg && (!strcmp(this->Name,arg))) 
    { 
    return;
    } 
  if (this->Name) 
    { 
    delete [] this->Name; 
    } 
  if (arg) 
    { 
    this->Name = new char[strlen(arg)+1]; 
    strcpy(this->Name,arg); 
    } 
  else 
    { 
    this->Name = NULL;
    }
  this->Modified();
  
  // Make sure the description frame is upto date.
  this->UpdateDescriptionFrame();

  // Update the nav window (that usually might display name + description)
  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->UpdateNavigationWindow(this, this->SourceGrabbed);
    }
} 

//----------------------------------------------------------------------------
char* vtkPVSource::GetLabel() 
{ 
  // Design choice: if the description is empty, initialize it with the
  // Tcl name, so that the user knows what to overrides in the nav window.

  if (this->Label == NULL)
    {
    this->SetLabelNoTrace(this->GetName());
    }
  return this->Label;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetLabel(const char* arg) 
{ 
  this->SetLabelNoTrace(arg);
  // Trace here, not in SetLabel (design choice)
  this->GetPVApplication()->AddTraceEntry("$kw(%s) SetLabel {%s}",
                                          this->GetTclName(),
                                          this->Label);
  this->GetPVApplication()->AddTraceEntry("$kw(%s) DescriptionEntryCallback",
                                          this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::SetLabelNoTrace(const char* arg) 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " 
                << this->Label << " to " << arg ); 
  if ( this->Label && arg && (!strcmp(this->Label,arg))) 
    { 
    return;
    } 
  if (this->Label) 
    { 
    delete [] this->Label; 
    } 
  if (arg) 
    { 
    this->Label = new char[strlen(arg)+1]; 
    strcpy(this->Label,arg); 
    } 
  else 
    { 
    this->Label = NULL;
    }
  this->Modified();

  // Make sure the description frame is upto date.
  this->UpdateDescriptionFrame();

  // Update the nav window (that usually might display name + description)
  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->UpdateNavigationWindow(this, this->SourceGrabbed);
    }
} 

//----------------------------------------------------------------------------
void vtkPVSource::LabelEntryCallback()
{
  this->SetLabel(this->LabelEntry->GetValue());
}

//----------------------------------------------------------------------------
// We should really be dealing with the outputs.  Remove this method.
void vtkPVSource::SetVisibility(int v)
{
  int i;
  vtkPVData *ac;
  
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i])
      {
      ac = this->PVOutputs[i];
      if (ac)
        {
        ac->SetVisibility(v);
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkPVSource::GetVisibility()
{
  if ( this->GetPVOutput(0) && this->GetPVOutput(0)->GetVisibility() )
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallback()
{
  // This method is purposely not virtual.  The AcceptCallbackFlag
  // must be 1 for the duration of the accept callback no matter what
  // subclasses might do.  All of the real AcceptCallback funcionality
  // should be implemented in AcceptCallbackInternal.
  this->AcceptCallbackFlag = 1;
  this->AcceptCallbackInternal();
  this->AcceptCallbackFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::PreAcceptCallback()
{
  this->Script("%s configure -cursor watch",
               this->GetPVWindow()->GetWidgetName());
  this->Script("after idle {%s AcceptCallback}", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallbackInternal()
{
  // I had trouble with this object destructing too early
  // in this method, because of an unregistered reference ...
  // It was an error condition (no output was created).
  this->Register(this);

  int dialogAlreadyUp=0;
  // This is here to prevent the user from killing the
  // application from inside the accept callback.
  if (!this->Application->GetDialogUp())
    {
    this->Application->SetDialogUp(1);
    }
  else
    {
    dialogAlreadyUp=1;
    }
  this->Accept(0);
  this->GetPVApplication()->AddTraceEntry("$kw(%s) AcceptCallback",
                                          this->GetTclName());
  if (!dialogAlreadyUp)
    {
    this->Application->SetDialogUp(0);
    }


  // I had trouble with this object destructing too early
  // in this method, because of an unregistered reference ...
  this->UnRegister(this);
}

//----------------------------------------------------------------------------
void vtkPVSource::Accept(int hideFlag, int hideSource)
{
  vtkPVWindow *window;

  this->Script("update");

  window = this->GetPVWindow();

  this->SetAcceptButtonColorToWhite();
  
  // We need to pass the parameters from the UI to the VTK objects before
  // we check whether to insert ExtractPieces.  Otherwise, we'll get errors
  // about unspecified file names, etc., when ExecuteInformation is called on
  // the VTK source.  (The vtkPLOT3DReader is a good example of this.)
  this->UpdateVTKSourceParameters();
  
  // This adds an extract filter only when the MaximumNumberOfPieces is 1.
  // This is only the case the first time the accept is called.
  int i;
  int numOutputs = this->GetNumberOfPVOutputs();
  for (i=0; i<numOutputs; i++)
    {
    if (this->GetNthPVOutput(i))
      {
      this->GetNthPVOutput(i)->InsertExtractPiecesIfNecessary();
      }
    }

  // Initialize the output if necessary.
  if ( ! this->Initialized)
    { // This is the first time, initialize data.    
    vtkPVData *input;
    vtkPVData *ac;
    
    ac = this->GetPVOutput(0);
    if (ac == NULL)
      { // I suppose we should try and delete the source.
      vtkErrorMacro("Could not get output.");
      this->DeleteCallback();    
      return;
      }
    
    window->GetMainView()->AddPVData(ac);
    if (!this->GetHideDisplayPage())
      {
      this->Notebook->AddPage("Display");
      }
    if (!this->GetHideInformationPage())
      {
      this->Notebook->AddPage("Information");
      }
    ac->CreateProperties();
    ac->Initialize();
    // Make the last data invisible.
    input = this->GetPVInput();
    if (input)
      {
      if (this->ReplaceInput && input->GetPropertiesCreated() && hideSource)
        {
        input->SetVisibilityInternal(0);
        }
      }

    this->UnGrabFocus();

    // Set the current data of the window.
    if ( ! hideFlag)
      {
      window->SetCurrentPVData(this->GetNthPVOutput(0));
      }
    else
      {
      this->GetPVOutput()->SetVisibilityInternal(0);
      }

    // The best test I could come up with to only reset
    // the camera when the first source is created.
    if (window->GetSourceList("Sources")->GetNumberOfItems() == 1)
      {
      float bds[6];
      ac->GetBounds(bds);
      window->SetCenterOfRotation(0.5*(bds[0]+bds[1]), 
                                  0.5*(bds[2]+bds[3]),
                                  0.5*(bds[4]+bds[5]));
      window->ResetCenterCallback();
      window->GetMainView()->ResetCamera();
      }

    this->Initialized = 1;
    }

  window->GetMenuView()->CheckRadioButton(
                                  window->GetMenuView(), "Radio", 2);
  this->UpdateProperties();
  this->GetPVRenderView()->EventuallyRender();

  // Update the selection menu.
  window->UpdateSelectMenu();

  // Regenerate the data property page in case something has changed.
  if (this->NumberOfPVOutputs > 0)
    {
    this->GetNthPVOutput(0)->UpdateProperties();
    }

  this->Script("update");  

#ifdef _WIN32
  this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
  this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif  

}

//----------------------------------------------------------------------------
void vtkPVSource::ResetCallback()
{
  this->UpdateParameterWidgets();
  if (this->Initialized)
    {
    this->GetPVRenderView()->EventuallyRender();
    this->Script("update");

    this->SetAcceptButtonColorToWhite();
    }
}

//---------------------------------------------------------------------------
void vtkPVSource::DeleteCallback()
{
  int i;
  int initialized = this->Initialized;
  vtkPVData *ac;
  vtkPVSource *prev = NULL;
  vtkPVSource *current = 0;
  vtkPVWindow *window = this->GetPVWindow();

  // Condition here because I am trying to delete glyph sources.  
  this->GetPVOutput()->DeleteCallback();

  // Just in case cursor was left in a funny state.
#ifdef _WIN32
  this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
  this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif  

  if ( ! this->Initialized)
    {
    // Remove the local grab
    this->UnGrabFocus();

    this->Script("update");   
    this->Initialized = 1;
    }
  
  if (this->GetNumberOfPVConsumers() > 0 )
    {
    vtkErrorMacro("An output is used.  We cannot delete this source.");
    return;
    }
  
  // Remove all the dummy pvsources attached as
  // connections to each output (only when there
  // are multiple outputs)
  int numOutputs = this->GetNumberOfPVOutputs();
  if (numOutputs > 1)
   {
   for (i=0; i<numOutputs; i++)
     {
     vtkPVData* output = this->GetPVOutput(i);
     if ( output )
       {
       int numCons2= output->GetNumberOfPVConsumers();
       for (int j=0; j<numCons2; j++)
         {
         vtkPVSource* consumer = output->GetPVConsumer(j);
         if ( consumer )
           {
           consumer->DeleteCallback();
           }
         }
       }
     }
   }

  // Save this action in the trace file.
  this->GetPVApplication()->AddTraceEntry("$kw(%s) DeleteCallback",
                                          this->GetTclName());

  // Get the input so we can make it visible and make it current.
  if (this->GetNumberOfPVInputs() > 0)
    {
    prev = this->PVInputs[0]->GetPVSource();
    // Just a sanity check
    if (prev == NULL)
      {
      vtkErrorMacro("Expecting an input but none found.");
      }
    else
      {
      prev->GetPVOutput()->SetVisibilityInternal(1);
      }
    }

  // Remove this source from the inputs users collection.
  for (i = 0; i < this->GetNumberOfPVInputs(); i++)
    {
    if (this->PVInputs[i])
      {
      this->PVInputs[i]->RemovePVConsumer(this);
      }
    }
    
  // Look for a source to make current.
  if (prev == NULL)
    {
    prev = this->GetPVWindow()->GetPreviousPVSource();
    }
  // Just remember it. We set the current pv source later.
  //this->GetPVWindow()->SetCurrentPVSourceCallback(prev);
  if ( prev == NULL && window->GetSourceList("Sources")->GetNumberOfItems() > 0 )
    {
    vtkCollectionIterator *it = window->GetSourceList("Sources")->NewIterator();
    it->InitTraversal();
    while ( !it->IsDoneWithTraversal() )
      {
      prev = static_cast<vtkPVSource*>( it->GetObject() );
      if ( prev != this )
        {
        break;
        }
      else
        {
        prev = 0;
        }
      it->GoToNextItem();
      }
    it->Delete();
    }

  current = window->GetCurrentPVSource();
  if ( this == current )
    {
    current = prev;
  
    if (prev == NULL)
      {
      // Unpack the properties.  This is required if prev is NULL.
      this->Script("catch {eval pack forget [pack slaves %s]}",
                   this->ParametersParent->GetWidgetName());
      
      // Show the 3D View settings
      vtkPVApplication *pvApp = 
        vtkPVApplication::SafeDownCast(this->Application);
      vtkPVWindow *window = pvApp->GetMainWindow();
      this->Script("%s invoke \"%s\"", 
                   window->GetMenuView()->GetWidgetName(),
                   VTK_PV_VIEW_MENU_LABEL);
      }
    }
        
  // Remove all of the actors mappers. from the renderer.
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i])
      {
      ac = this->GetPVOutput(i);
      this->GetPVRenderView()->RemovePVData(ac);
      }
    }    

  // Remove all of the outputs
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i])
      {
      this->PVOutputs[i]->UnRegister(this);
      this->PVOutputs[i] = NULL;
      }
    }
  
  if ( initialized )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  
  // This should delete this source.
  // "this" will no longer be valid after the call.
  window->RemovePVSource("Sources", this);
  window->SetCurrentPVSourceCallback(current);
}

//----------------------------------------------------------------------------
void vtkPVSource::VTKSourceModifiedMethod()
{
  if(!this->AcceptCallbackFlag)
    {
    this->UpdateParameterWidgets();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateParameterWidgets()
{
  vtkPVWidget *pvw;
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetObject());
    pvw->ForceReset();
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSource::RaiseSourcePage()
{
  this->Notebook->Raise("Source");
}

//----------------------------------------------------------------------------
// This should be apart of AcceptCallbackInternal.
void vtkPVSource::UpdateVTKSourceParameters()
{
  vtkPVWidget *pvw;
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetObject());
    pvw->Accept();
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
// Returns the number of consumers of either:
// 1. the first output if there is only one output,
// 2. the outputs of the consumers of
//    each output if there are more than one outputs.
// Method (2) is used for multiple outputs because,
// in this situation, there is always a dummy pvsource
// attached to each of the outputs.
int vtkPVSource::GetNumberOfPVConsumers()
{

 int numOutputs = this->GetNumberOfPVOutputs();
 if (numOutputs > 1)
   {
   int numConsumers = 0;
   for (int i=0; i<numOutputs; i++)
     {
     vtkPVData* output = this->GetPVOutput(i);
     if ( output )
       {
       int numCons2= output->GetNumberOfPVConsumers();
       for (int j=0; j<numCons2; j++)
         {
         vtkPVSource* consumer = output->GetPVConsumer(j);
         if ( consumer )
           {
           numConsumers += consumer->GetNumberOfPVConsumers();
           }
         }
       }
     }
   return numConsumers;
   }
 else
   {
   vtkPVData* output0 = this->GetPVOutput(0);
   if ( output0 )
     {
     return output0->GetNumberOfPVConsumers();
     }
   }

 return 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateProperties()
{
  // --------------------------------------
  // Change the state of the delete button based on if there are any users.
  // Only filters at the end of a pipeline can be deleted.
  if ( this->IsDeletable() )
      {
      this->Script("%s configure -state normal",
                   this->DeleteButton->GetWidgetName());
      }
    else
      {
      this->Script("%s configure -state disabled",
                   this->DeleteButton->GetWidgetName());
      }
  
  // I do not know why the inputs have to be updated.
  // I am changing it to output as an experiment.
  // The output might have been already updated elsewhere.
  if (this->GetPVOutput())
    {
    // Update the VTK data.
    this->GetPVOutput()->Update();
    }

  this->UpdateDescriptionFrame();
}

//----------------------------------------------------------------------------
int vtkPVSource::IsDeletable()
{
  if (this->IsPermanent || this->GetNumberOfPVConsumers() > 0 )
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetParametersParent(vtkKWWidget *parent)
{
  if (this->ParametersParent == parent)
    {
    return;
    }
  if (this->ParametersParent)
    {
    vtkErrorMacro("Cannot reparent properties.");
    return;
    }
  this->ParametersParent = parent;
  parent->Register(this);
}


  
typedef vtkPVData *vtkPVDataPointer;
//---------------------------------------------------------------------------
void vtkPVSource::SetNumberOfPVInputs(int num)
{
  int idx;
  vtkPVDataPointer *inputs;

  // in case nothing has changed.
  if (num == this->NumberOfPVInputs)
    {
    return;
    }
  
  // Allocate new arrays.
  inputs = new vtkPVDataPointer[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    inputs[idx] = NULL;
    }

  // Copy old inputs
  for (idx = 0; idx < num && idx < this->NumberOfPVInputs; ++idx)
    {
    inputs[idx] = this->PVInputs[idx];
    }
  
  // delete the previous arrays
  if (this->PVInputs)
    {
    delete [] this->PVInputs;
    this->PVInputs = NULL;
    this->NumberOfPVInputs = 0;
    }
  
  // Set the new array
  this->PVInputs = inputs;
  
  this->NumberOfPVInputs = num;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::SetNthPVInput(int idx, vtkPVData *pvd)
{
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthPVInput: " << idx << ", cannot set input. ");
    return;
    }
  
  // Expand array if necessary.
  if (idx >= this->NumberOfPVInputs)
    {
    this->SetNumberOfPVInputs(idx + 1);
    }
  
  // Does this change anything?  Yes, it keeps the object from being modified.
  if (pvd == this->PVInputs[idx])
    {
    return;
    }
  
  if (this->PVInputs[idx])
    {
    this->PVInputs[idx]->RemovePVConsumer(this);
    this->PVInputs[idx]->UnRegister(this);
    this->PVInputs[idx] = NULL;
    }
  
  if (pvd)
    {
    pvd->Register(this);
    pvd->AddPVConsumer(this);
    this->PVInputs[idx] = pvd;
    }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::RemoveAllPVInputs()
{
  if ( this->PVInputs )
    {
    for (int idx = 0; idx < this->NumberOfPVInputs; ++idx)
      {
      if ( this->PVInputs[idx] )
        {
        this->PVInputs[idx]->UnRegister(this);
        this->PVInputs[idx] = NULL;
        }
      }

    delete [] this->PVInputs;
    this->PVInputs = NULL;
    this->NumberOfPVInputs = 0;
    this->Modified();
    }
}

//---------------------------------------------------------------------------
vtkPVData *vtkPVSource::GetNthPVInput(int idx)
{
  if (idx >= this->NumberOfPVInputs)
    {
    return NULL;
    }
  
  return (vtkPVData *)(this->PVInputs[idx]);
}

//---------------------------------------------------------------------------
void vtkPVSource::SetNumberOfPVOutputs(int num)
{
  int idx;
  vtkPVDataPointer *outputs;

  // in case nothing has changed.
  if (num == this->NumberOfPVOutputs)
    {
    return;
    }
  
  // Allocate new arrays.
  outputs = new vtkPVDataPointer[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    outputs[idx] = NULL;
    }

  // Copy old outputs
  for (idx = 0; idx < num && idx < this->NumberOfPVOutputs; ++idx)
    {
    outputs[idx] = this->PVOutputs[idx];
    }
  
  // delete the previous arrays
  if (this->PVOutputs)
    {
    delete [] this->PVOutputs;
    this->PVOutputs = NULL;
    this->NumberOfPVOutputs = 0;
    }
  
  // Set the new array
  this->PVOutputs = outputs;
  
  this->NumberOfPVOutputs = num;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::SetNthPVOutput(int idx, vtkPVData *pvd)
{
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthPVOutput: " << idx << ", cannot set output. ");
    return;
    }
  
  if (this->NumberOfPVOutputs <= idx)
    {
    this->SetNumberOfPVOutputs(idx+1);
    }
  
  // Does this change anything?  Yes, it keeps the object from being modified.
  if (pvd == this->PVOutputs[idx])
    {
    return;
    }
  
  if (this->PVOutputs[idx])
    {
    // Manage backward pointer.
    this->PVOutputs[idx]->SetPVSource(this);
    this->PVOutputs[idx]->UnRegister(this);
    this->PVOutputs[idx] = NULL;
    }
  
  if (pvd)
    {
    pvd->Register(this);
    this->PVOutputs[idx] = pvd;
    // Manage backward pointer.
    pvd->SetPVSource(this);
    }

  this->Modified();
}

//---------------------------------------------------------------------------
vtkPVData *vtkPVSource::GetNthPVOutput(int idx)
{
  if (idx >= this->NumberOfPVOutputs)
    {
    return NULL;
    }
  
  return (vtkPVData *)(this->PVOutputs[idx]);
}

//----------------------------------------------------------------------------
void vtkPVSource::SaveInTclScript(ofstream *file, int interactiveFlag, 
                                  int vtkFlag)
{
  int i, numWidgets;
  vtkPVWidget *widget;

  // Detect special sources we do not handle yet.
  if (this->VTKSource == NULL)
    {
    return;
    }

  // This should not be needed, but We can check anyway.
  if (this->VisitedFlag == 2)
    {
    return;
    }

  // Special condition to detect loops in the pipeline.
  // Visited 1 means that source is in stack, 2 means that
  // the tcl script contains the source.
  if (this->VisitedFlag == 1)
    { // This source is already in in the stack, but there is a loop.
    *file << "\n" << this->VTKSource->GetClassName()
          << " " << this->VTKSourceTclName << "\n";
    this->VisitedFlag = 2;
    return;
    }

  // This is the recursive part.
  this->VisitedFlag = 1;
  // Loop through all of the inputs
  for (i = 0; i < this->NumberOfPVInputs; ++i)
    {
    if (this->PVInputs[i] && this->PVInputs[i]->GetPVSource()->GetVisitedFlag() != 2)
      {
      this->PVInputs[i]->GetPVSource()->SaveInTclScript(file, interactiveFlag, vtkFlag);
      }
    }
  
  // Save the object in the script.
  if (this->VisitedFlag != 2)
    {
    *file << "\n" << this->VTKSource->GetClassName()
          << " " << this->VTKSourceTclName << "\n";
    this->VisitedFlag = 2;
    }

  // Let the PVWidgets set up the object.
  numWidgets = this->Widgets->GetNumberOfItems();
  for (i = 0; i < numWidgets; i++)
    {
    widget = vtkPVWidget::SafeDownCast(this->Widgets->GetItemAsObject(i));
    if (widget)
      {
      widget->SaveInTclScript(file);
      }
    }

  // Add the mapper, actor, scalar bar actor ...
  this->GetPVOutput()->SaveInTclScript(file, interactiveFlag, vtkFlag);
}


//----------------------------------------------------------------------------
void vtkPVSource::AddPVWidget(vtkPVWidget *pvw)
{
  char str[512];
  this->Widgets->AddItem(pvw);  

  if (pvw->GetTraceName() == NULL)
    {
    vtkWarningMacro("TraceName not set. Widget class: " 
                    << pvw->GetClassName());
    return;
    }

  pvw->SetTraceReferenceObject(this);
  sprintf(str, "GetPVWidget {%s}", pvw->GetTraceName());
  pvw->SetTraceReferenceCommand(str);
  pvw->Select();
}

//----------------------------------------------------------------------------
void vtkPVSource::AddInputClassName(const char* classname)
{
  this->InputClassNames->AppendItem(classname);
}

//----------------------------------------------------------------------------
vtkIdType vtkPVSource::GetNumberOfInputClasses()
{
  return this->InputClassNames->GetNumberOfItems();
}

//----------------------------------------------------------------------------
int vtkPVSource::GetIsValidInput(vtkPVData *pvd)
{
  vtkDataObject *data;

  int match = 0;
  vtkIdType numItems = this->InputClassNames->GetNumberOfItems();
  for(int i=0; i<numItems; i++)
    {
    const char* item;
    if ( this->InputClassNames->GetItem(i, item) == VTK_OK )
      {
      data = pvd->GetVTKData();
      if ( data && data->IsA(item) )
        {
        match = 1;
        break;
        }
      }
    }

  return match;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetAcceptButtonColorToRed()
{
  this->Script("%s configure -background red1",
               this->AcceptButton->GetWidgetName());
}

void vtkPVSource::SetAcceptButtonColorToWhite()
{
#ifdef _WIN32
  this->Script("%s configure -background SystemButtonFace",
               this->AcceptButton->GetWidgetName());
#else
  this->Script("%s configure -background #d9d9d9",
               this->AcceptButton->GetWidgetName());
#endif
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVSource::GetPVWidget(const char *name)
{
  vtkObject *o;
  vtkPVWidget *pvw;
  this->Widgets->InitTraversal();

  while ( (o = this->Widgets->GetNextItemAsObject()) )
    {
    pvw = vtkPVWidget::SafeDownCast(o);
    if (pvw && pvw->GetTraceName() && strcmp(pvw->GetTraceName(), name) == 0)
      {
      return pvw;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVSource::GetPVRenderView()
{
  return vtkPVRenderView::SafeDownCast(this->View);
}

vtkPVInputMenu *vtkPVSource::AddInputMenu(char *label, char *inputName, 
                                          char *inputType, char *help, 
                                          vtkPVSourceCollection *sources)
{
  vtkPVInputMenu *inputMenu;

  inputMenu = vtkPVInputMenu::New();
  inputMenu->SetPVSource(this);
  inputMenu->SetSources(sources);
  inputMenu->SetParent(this->ParameterFrame->GetFrame());
  inputMenu->SetLabel(label);
  inputMenu->SetModifiedCommand(this->GetTclName(), 
                                "SetAcceptButtonColorToRed");
  inputMenu->Create(this->Application);
  inputMenu->SetInputName(inputName);
  inputMenu->SetInputType(inputType);
  inputMenu->SetBalloonHelpString(help);

  this->AddPVWidget(inputMenu);
  inputMenu->Delete();

  return inputMenu;
}

int vtkPVSource::ClonePrototype(int makeCurrent, vtkPVSource*& clone )
{
  return this->ClonePrototypeInternal(makeCurrent, clone);
}

int vtkPVSource::ClonePrototypeInternal(int makeCurrent, vtkPVSource*& clone)
{
  clone = 0;

  // Create instance
  vtkPVSource* pvs = this->NewInstance();
  // Copy properties
  pvs->SetApplication(this->Application);
  pvs->SetReplaceInput(this->ReplaceInput);
  pvs->SetParametersParent(this->ParametersParent);

  pvs->SetShortHelp(this->GetShortHelp());
  pvs->SetLongHelp(this->GetLongHelp());

  vtkIdType numItems = this->InputClassNames->GetNumberOfItems();
  vtkIdType id;
  for(id=0; id<numItems; id++)
    {
    const char* item;
    if ( this->InputClassNames->GetItem(id, item) == VTK_OK )
      {
      pvs->AddInputClassName(item);
      }
    }
  pvs->SetOutputClassName(this->OutputClassName);
  pvs->SetSourceClassName(this->SourceClassName);

  pvs->SetModuleName(this->ModuleName);

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(pvs->Application);
  if (!pvApp)
    {
    vtkErrorMacro("vtkPVApplication is not set properly. Aborting clone.");
    pvs->Delete();
    return VTK_ERROR;
    }

  vtkPVData *current = this->GetPVWindow()->GetCurrentPVData();

  // Create a (unique) name for the source.
  // Beware: If two prototypes have the same name, the name 
  // will not be unique.
  char tclName[1024];
  if (this->Name && this->Name[0] != '\0')
    { 
    sprintf(tclName, "%s%d", this->Name, this->PrototypeInstanceCount);
    }
  else
    {
    vtkErrorMacro("The prototype must have a name. Cloning aborted.");
    pvs->Delete();
    return VTK_ERROR;
    }
  pvs->SetName(tclName);

  // Create a vtkSource with the same name as the PVSource
  vtkSource* vtksource = 
    static_cast<vtkSource *>(pvApp->MakeTclObject(this->SourceClassName, 
                                                  tclName));
  if (vtksource == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    pvs->Delete();
    return VTK_ERROR;
    }

  pvs->SetVTKSource(vtksource, tclName);
  pvs->SetView(this->GetPVWindow()->GetMainView());
  // Set the input if necessary.
  if (pvs->InputClassNames->GetNumberOfItems() > 0)
    {
    pvs->SetPVInput(current);
    }
  // Create the properties frame etc.
  pvs->CreateProperties();

  // let's see if we can determine the output type.
  const char* outputDataType = this->GetOutputClassName();
  if (strcmp(outputDataType, "vtkDataSet") == 0)
    { // Output will be the same as the input.
    if (current == NULL)
      {
      pvs->Delete();
      vtkErrorMacro("Cannot determine output type.");
      return VTK_ERROR;
      }
    outputDataType = current->GetVTKData()->GetClassName();
    }
  if (strcmp(outputDataType, "vtkPointSet") == 0)
    { // Output will be the same as the input.
    if (current == NULL)
      {
      pvs->Delete();
      vtkErrorMacro("Cannot determine output type.");
      return VTK_ERROR;
      }
    outputDataType = current->GetVTKData()->GetClassName();
    }

  if (makeCurrent)
    {
    // This has to be here because if it is called
    // after the PVData is set we get errors. This 
    // should probably be fixed.
    this->GetPVWindow()->SetCurrentPVSourceCallback(pvs);
    this->GetPVWindow()->ShowCurrentSourceProperties();
    }

  // Create the output.
  char otherTclName[1024];
  vtkPVData* pvd = vtkPVData::New();
  pvd->SetPVApplication(pvApp);
  sprintf(otherTclName, "%sOutput", tclName);
  // Create the object through tcl on all processes.
  vtkDataSet* vtkdata = static_cast<vtkDataSet *>(pvApp->MakeTclObject(
    outputDataType, otherTclName));
  pvd->SetVTKData(vtkdata, otherTclName);

  // Connect the source and data.
  pvs->SetPVOutput(pvd);  
  
  // Relay the connection to the VTK objects.  
  pvApp->BroadcastScript("%s SetOutput %s", pvs->GetVTKSourceTclName(),
                         pvd->GetVTKDataTclName());   

  // Create the extent translator
  if (pvs->InputClassNames->GetNumberOfItems())
    {
    pvApp->BroadcastScript("%s SetExtentTranslator [%s GetExtentTranslator]",
                           pvd->GetVTKDataTclName(), 
                           current->GetVTKDataTclName());
    }
  else
    {
    sprintf(otherTclName, "%sTranslator", tclName);
    pvApp->BroadcastScript("vtkPVExtentTranslator %s", otherTclName);
    pvApp->BroadcastScript("%s SetOriginalSource [%s GetOutput]",
                           otherTclName, 
                           pvs->GetVTKSourceTclName());
    pvApp->BroadcastScript("%s SetExtentTranslator %s",
                           pvd->GetVTKDataTclName(), 
                           otherTclName);
    // Hold onto name so it can be deleted.
    pvs->SetExtentTranslatorTclName(otherTclName);
    }

  pvd->Delete();

  this->PrototypeInstanceCount++;

  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* widgetMap =
    vtkArrayMap<vtkPVWidget*, vtkPVWidget*>::New();

  // Copy all widgets
  vtkPVWidget *pvWidget, *clonedWidget;
  this->Widgets->InitTraversal();
  int i;
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvWidget = this->Widgets->GetNextPVWidget();
    clonedWidget = pvWidget->ClonePrototype(pvs, widgetMap);
    clonedWidget->SetParent(pvs->ParameterFrame->GetFrame());
    clonedWidget->Create(pvApp);
    pvApp->Script("pack %s -side top -fill x -expand t", 
                  clonedWidget->GetWidgetName());
    pvs->AddPVWidget(clonedWidget);
    clonedWidget->Delete();
    }
  widgetMap->Delete();
  
  // Accept button is always red when a source is first created.
  pvs->SetAcceptButtonColorToRed();
  
  clone = pvs;
  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkPVSource::SerializeSelf(ostream& os, vtkIndent indent)
{
  int cc;
  this->Superclass::SerializeSelf(os, indent);
  os << indent << "ModuleName " << this->ModuleName << endl;
  os << indent << "HideDisplayPage " << this->HideDisplayPage << endl;
  os << indent << "HideParametersPage " << this->HideParametersPage << endl;
  os << indent << "HideInformationPage " << this->HideInformationPage << endl;
  os << indent << "NumberOfPVInputs " << this->GetNumberOfPVInputs() << endl;
  for ( cc = 0; cc < this->GetNumberOfPVInputs(); cc ++ )
    {
    if ( this->GetNthPVInput(cc) && this->GetNthPVInput(cc)->GetPVSource() )
      {
      os << indent << "Input " << cc << " " 
         << this->GetNthPVInput(cc)->GetPVSource()->GetName() 
         << endl;
      }
    }
  os << indent << "Output ";
  this->GetPVOutput()->Serialize(os, indent);
  if ( this->Widgets )
    {
    os << indent << "Widgets " << endl;
    vtkCollectionIterator* it = this->Widgets->NewIterator();
    it->InitTraversal();
    vtkIndent indentp = indent.GetNextIndent();
    os << indentp << "{" << endl;
    int cc =0;
    while( !it->IsDoneWithTraversal() )
      {
      vtkPVWidget* widget = static_cast<vtkPVWidget*>( it->GetObject() );
      os << indentp << "Widget \"" << widget->GetTraceName() << "\" ";
      widget->Serialize(os, indentp);

      it->GoToNextItem();
      cc ++;
      }
    os << indentp << "}" << endl;
    it->Delete();
    }
}

//------------------------------------------------------------------------------
void vtkPVSource::SerializeToken(istream& is, const char token[1024])
{
  if ( vtkString::Equals(token, "HideDisplayPage") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->HideDisplayPage = cor;
    }
  else if ( vtkString::Equals(token, "HideParametersPage") )
    {
    int cor;
    cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->HideParametersPage = cor;
    }
  else if ( vtkString::Equals(token, "HideInformationPage") )
    {
    int cor;
    cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->HideInformationPage = cor;
    }
  else if ( vtkString::Equals(token, "ModuleName") )
    {
    char name[1024];
    name[0] = 0;
    if ( ! (is >> name ) )
      {
      vtkErrorMacro("Problem Parsing session file");
      return;
      }
    this->SetModuleName(name);
    }
  else if ( vtkString::Equals(token, "Output") )
    {
    vtkPVData* output = this->GetPVOutput();
    if (!output )
      {
      output = vtkPVData::New();
      output->SetPVApplication(
        vtkPVApplication::SafeDownCast(this->Application));
      this->SetPVOutput(output);
      output->Delete();
      }
    //this->AcceptCallback();
    this->Accept(0, 0);
    output->Serialize(is);
    }
  else if ( vtkString::Equals(token, "NumberOfPVInputs") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->SetNumberOfPVInputs(cor);
    }
  else if ( vtkString::Equals(token, "Input--does-not-work") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    //cout << "Reading input: " << cor << endl;
    char name[1024];
    name[0] = 0;
    if ( ! (is >> name ) )
      {
      vtkErrorMacro("Problem Parsing session file");
      return;
      }
    //cout << "Input " << cor << " is: " << name << endl;
    vtkPVApplication *application 
      = vtkPVApplication::SafeDownCast(this->Application);
    vtkPVSource* isource 
      = application->GetMainWindow()->GetSourceFromName(name);
    if ( !isource )
      {
      vtkErrorMacro("Cannot find input to this source");
      return;
      }
    this->SetNthPVInput(cor, isource->GetPVOutput());
    this->UpdateParameterWidgets();
    this->Accept(0,0);
    }
  else if ( vtkString::Equals(token, "Widgets") )
    {
    char ntoken[1024];
    int done = 0;;
    while(!done)
      {
      ntoken[0] = 0;
      if ( !( is >> ntoken ) )
        {
        vtkErrorMacro("Error when parsing widgets");
        return;
        }
      if ( vtkString::Equals(ntoken, "}") )
        {
        done = 1;
        }
      else if ( vtkString::Equals(ntoken, "Widget") )
        {
        char tracename[1024];
        vtkKWSerializer::GetNextToken(&is,tracename);
        vtkPVWidget* widget = this->GetPVWidget(tracename);
        if ( !widget )
          {
          vtkErrorMacro("Widget " << tracename << " does not exists in "
                        << this->GetModuleName());
          return;
          }
        //cout << "Widget: " << tracename << " (" << widget << ") " 
        //     << widget->GetClassName() << endl;
        widget->Serialize(is);
        }
      else 
        {
        //cout << "Unknown token: " << ntoken << endl;
        }
      }
    }
  else
    {
    //cout << "Unknown Token for " << this->GetClassName() << ": " 
    //     << token << endl;
    this->Superclass::SerializeToken(is,token);  
    }
}

//------------------------------------------------------------------------------
void vtkPVSource::SerializeRevision(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeRevision(os,indent);
  os << indent << "vtkPVSource ";
  this->ExtractRevision(os,"$Revision: 1.248.2.13 $");
}

//----------------------------------------------------------------------------
void vtkPVSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Initialized: " << (this->Initialized?"yes":"no") << endl;
  os << indent << "Name: " << (this->Name ? this->Name : "none") << endl;
  os << indent << "LongHelp: " << (this->LongHelp ? this->LongHelp : "none") 
     << endl;
  os << indent << "ShortHelp: " << (this->ShortHelp ? this->ShortHelp : "none") 
     << endl;
  os << indent << "Description: " 
     << (this->Label ? this->Label : "none") << endl;
  os << indent << "ModuleName: " << (this->ModuleName?this->ModuleName:"none")
     << endl;
  os << indent << "MenuName: " << (this->MenuName?this->MenuName:"none")
     << endl;
  os << indent << "AcceptButton: " << this->GetAcceptButton() << endl;
  os << indent << "DeleteButton: " << this->GetDeleteButton() << endl;
  os << indent << "ExtentTranslatorTclName: " 
     << (this->ExtentTranslatorTclName?this->ExtentTranslatorTclName:"null")
     << endl;
  os << indent << "MainParameterFrame: " << this->GetMainParameterFrame() 
     << endl;
  os << indent << "DescriptionFrame: " << this->DescriptionFrame 
     << endl;
  os << indent << "Notebook: " << this->GetNotebook() << endl;
  os << indent << "NumberOfPVInputs: " << this->GetNumberOfPVInputs() << endl;
  os << indent << "NumberOfPVOutputs: " << this->GetNumberOfPVOutputs() 
     << endl;
  os << indent << "ParameterFrame: " << this->GetParameterFrame() << endl;
  os << indent << "ParametersParent: " << this->GetParametersParent() << endl;
  os << indent << "ReplaceInput: " << this->GetReplaceInput() << endl;
  os << indent << "VTKSource: " << this->GetVTKSource() << endl;
  os << indent << "VTKSourceTclName: " 
     << (this->VTKSourceTclName?this->VTKSourceTclName:"none") << endl;
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "VisitedFlag: " << this->GetVisitedFlag() << endl;
  os << indent << "Widgets: " << this->GetWidgets() << endl;
  os << indent << "InputClassNames: " << this->InputClassNames << endl;
  os << indent << "IsPermanent: " << this->IsPermanent << endl;
  os << indent << "OutputClassName: " 
     << (this->OutputClassName?this->OutputClassName:"null") << endl;
  os << indent << "SourceClassName: " 
     << (this->SourceClassName?this->SourceClassName:"null") << endl;
  os << indent << "HideParametersPage: " << this->HideParametersPage << endl;
  os << indent << "HideDisplayPage: " << this->HideDisplayPage << endl;
  os << indent << "HideInformationPage: " << this->HideInformationPage << endl;
  os << indent << "ToolbarModule: " << this->ToolbarModule << endl;
}
