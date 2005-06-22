/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTraceFileDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTraceFileDialog.h"

#include "vtkKWFrame.h"
#include "vtkKWPushButton.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVTraceFileDialog );
vtkCxxRevisionMacro(vtkPVTraceFileDialog, "1.13");

//-----------------------------------------------------------------------------
vtkPVTraceFileDialog::vtkPVTraceFileDialog()
{
  this->SaveFrame = vtkKWFrame::New();
  this->SaveFrame->SetParent(this->ButtonFrame);
  this->SaveButton = vtkKWPushButton::New();
  this->SaveButton->SetParent(this->SaveFrame);

  this->RetraceFrame = vtkKWFrame::New();
  this->RetraceFrame->SetParent(this->ButtonFrame);
  this->RetraceButton = vtkKWPushButton::New();
  this->RetraceButton->SetParent(this->RetraceFrame);

  this->SetStyleToOkCancel();
  this->SetOptions(
    vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::Beep | 
    vtkKWMessageDialog::YesDefault );
  this->SetOKButtonText("Delete");
  this->SetCancelButtonText("Do Nothing");

}

//-----------------------------------------------------------------------------
vtkPVTraceFileDialog::~vtkPVTraceFileDialog()
{
  this->SaveFrame->Delete();
  this->SaveButton->Delete();
  this->RetraceFrame->Delete();
  this->RetraceButton->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVTraceFileDialog::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("TraceFileDialogx already created");
    return;
    }

  // invoke super method
  this->Superclass::Create(app);

  this->SaveFrame->Create(app);
  this->SaveFrame->SetBorderWidth(3);

  this->SaveButton->Create(app);
  this->SaveButton->SetText("Save");
  this->SaveButton->SetWidth(16);
  this->SaveButton->SetCommand(this, "Save");

  this->Script("pack %s -side left -expand yes",
               this->SaveButton->GetWidgetName());
  this->Script("pack %s -side left -padx 4 -expand yes",
               this->SaveFrame->GetWidgetName());

  if ( this->SaveButton->GetApplication() )
    {
    this->SaveButton->SetBind("<FocusIn>", this->SaveFrame->GetWidgetName(), 
                            "configure -relief groove");
    this->SaveButton->SetBind("<FocusOut>", this->SaveFrame->GetWidgetName(), 
                            "configure -relief flat");
    this->SaveButton->SetBind(this, "<Return>", "Save");
    }
  this->RetraceFrame->Create(app);
  this->SaveFrame->SetBorderWidth(3);

  this->RetraceButton->Create(app);
  this->RetraceButton->SetText("Recover");
  this->RetraceButton->SetWidth(16);
  this->RetraceButton->SetCommand(this, "Retrace");

  this->Script("pack %s -side left -expand yes",
               this->RetraceButton->GetWidgetName());
  this->Script("pack %s -side left -padx 4 -expand yes",
               this->RetraceFrame->GetWidgetName());

  if ( this->RetraceButton->GetApplication() )
    {
    this->RetraceButton->SetBind("<FocusIn>", this->RetraceFrame->GetWidgetName(), 
                            "configure -relief groove");
    this->RetraceButton->SetBind("<FocusOut>", this->RetraceFrame->GetWidgetName(), 
                            "configure -relief flat");
    this->RetraceButton->SetBind(this, "<Return>", "Retrace");
    }
}

//----------------------------------------------------------------------------
void vtkPVTraceFileDialog::Save()
{
  this->Withdraw();
  this->ReleaseGrab();
  this->Done = 3;  
}

//----------------------------------------------------------------------------
void vtkPVTraceFileDialog::Retrace()
{
  this->Withdraw();
  this->ReleaseGrab();
  this->Done = 4;  
}

//----------------------------------------------------------------------------
void vtkPVTraceFileDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
