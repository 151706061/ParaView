/*=========================================================================

  Program:   ParaView
  Module:    vtkKWLookmarkFolder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/


#include "vtkKWLookmarkFolder.h"

#include "vtkCollectionIterator.h"
#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWDragAndDropTargetSet.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWLabel.h"
#include "vtkKWLookmark.h"
#include "vtkKWText.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLookmarkFolder );
vtkCxxRevisionMacro( vtkKWLookmarkFolder, "1.11");

int vtkKWLookmarkFolderCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLookmarkFolder::vtkKWLookmarkFolder()
{
  this->CommandFunction = vtkKWLookmarkFolderCommand;

  this->MainFrame = vtkKWFrame::New();
  this->LabelFrame= vtkKWFrameLabeled::New();
  this->SeparatorFrame = vtkKWFrame::New();
  this->NestedSeparatorFrame = vtkKWFrame::New();
  this->NameField = vtkKWText::New();
  this->Checkbox = vtkKWCheckButton::New();
}

//----------------------------------------------------------------------------
vtkKWLookmarkFolder::~vtkKWLookmarkFolder()
{
  this->RemoveFolder();
}


void vtkKWLookmarkFolder::RemoveFolder()
{
  if(this->NameField)
    {
    this->NameField->Delete();
    this->NameField = NULL;
    }

  if(this->Checkbox)
    {
    this->Checkbox->Delete();
    this->Checkbox = NULL;
    }

  if(this->MainFrame)
    {
    this->MainFrame->Delete();
    this->MainFrame = NULL;
    }

  if(this->LabelFrame)
    {
    this->LabelFrame->Delete();
    this->LabelFrame= NULL;
    }
  if(this->SeparatorFrame)
    {
    this->SeparatorFrame->Delete();
    this->SeparatorFrame = 0;
    }
  if(this->NestedSeparatorFrame)
    {
    this->NestedSeparatorFrame->Delete();
    this->NestedSeparatorFrame = 0;
    }
}


//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  this->MainFrame->SetParent(this);
  this->MainFrame->Create(app);
  this->Script("pack %s -fill x -expand t -anchor nw", this->MainFrame->GetWidgetName());

  this->Checkbox->SetParent(this->MainFrame);
  this->Checkbox->SetIndicator(1);
  this->Checkbox->Create(app);
  this->Checkbox->SetState(0);
  this->Checkbox->SetCommand(this, "SelectCallback");
  this->Script("pack %s -anchor nw -side left", this->Checkbox->GetWidgetName());

  this->LabelFrame->SetParent(this->MainFrame);
  this->LabelFrame->ShowHideFrameOn();
  this->LabelFrame->Create(app);
  this->LabelFrame->SetLabelText("Folder");
//  this->LabelFrame->GetLabel()->SetBind(this, "<Double-1>", "EditCallback");
  this->Script("pack %s -fill x -expand t -side left", this->LabelFrame->GetWidgetName());

  this->GetDragAndDropTargetSet()->SetStartCommand(
    this, "DragAndDropStartCallback");
  this->GetDragAndDropTargetSet()->SetEndCommand(
    this, "DragAndDropEndCallback");
  this->GetDragAndDropTargetSet()->SetSourceAnchor(
    this->LabelFrame->GetLabel());

  this->SeparatorFrame->SetParent(this);
  this->SeparatorFrame->Create(app);
  this->Script("pack %s -anchor nw -expand t -fill x",
                 this->SeparatorFrame->GetWidgetName());
  this->Script("%s configure -height 12",this->SeparatorFrame->GetWidgetName());

  this->NestedSeparatorFrame->SetParent(this->LabelFrame->GetFrame());
  this->NestedSeparatorFrame->Create(app);
  this->Script("pack %s -anchor nw -expand t -fill x",
                 this->NestedSeparatorFrame->GetWidgetName());
  this->Script("%s configure -height 12",this->NestedSeparatorFrame->GetWidgetName()); 

//  this->LabelFrame->GetLabel()->SetBind(this, "<Double-1>", "EditCallback");

  this->NameField->SetParent(this->LabelFrame->GetLabelFrame());
  this->NameField->Create(app);

  this->UpdateEnableState();

}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::DragAndDropPerformCommand(int x, int y, vtkKWWidget *vtkNotUsed(widget), vtkKWWidget *vtkNotUsed(anchor))
{
  if (  vtkKWTkUtilities::ContainsCoordinates(
        this->GetApplication()->GetMainInterp(),
        this->SeparatorFrame->GetWidgetName(),
        x, y))
    {
    this->Script("%s configure -bd 2 -relief groove", this->SeparatorFrame->GetWidgetName());
    this->Script("%s configure -bd 0 -relief flat", this->NestedSeparatorFrame->GetWidgetName());
    }
  else if (  vtkKWTkUtilities::ContainsCoordinates(
        this->GetApplication()->GetMainInterp(),
        this->NestedSeparatorFrame->GetWidgetName(),
        x, y))
    {
    this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
    this->Script("%s configure -bd 2 -relief groove", this->NestedSeparatorFrame->GetWidgetName());
    }
  else if (  vtkKWTkUtilities::ContainsCoordinates(
        this->GetApplication()->GetMainInterp(),
        this->LabelFrame->GetLabel()->GetWidgetName(),
        x, y))
    {
    this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
    this->Script("%s configure -bd 2 -relief groove", this->NestedSeparatorFrame->GetWidgetName());
    }
  else 
    {
    this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
    this->Script("%s configure -bd 0 -relief flat", this->NestedSeparatorFrame->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::RemoveDragAndDropTargetCues()
{
  this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
  this->Script("%s configure -bd 0 -relief flat", this->NestedSeparatorFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::SetFolderName(const char *name)
{
  this->LabelFrame->SetLabelText(name);
}


//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::EditCallback()
{
  char *temp = new char[100];

  strcpy(temp,this->LabelFrame->GetLabel()->GetText());
  this->LabelFrame->SetLabelText("");
  this->Script("pack %s", this->NameField->GetWidgetName());
  this->Script("%s configure -bg white -height 1 -width %d -wrap none", this->NameField->GetTextWidget()->GetWidgetName(), strlen(temp));
  if(this->NameField)
    this->NameField->SetValue(temp);
  this->NameField->GetTextWidget()->SetBind(this, "<KeyPress-Return>", "ChangeName");

  delete [] temp;
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::ChangeName()
{
  char *lmkName = new char[100];

  strcpy(lmkName,this->NameField->GetValue());
  this->NameField->Unpack();
  this->Script("pack %s -side left -fill x -expand t -padx 2", this->LabelFrame->GetLabel()->GetWidgetName());
  this->LabelFrame->SetLabelText(lmkName);

  this->ToggleNestedCheckBoxes(this->LabelFrame, 0);

  delete [] lmkName;
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::Pack()
{
  // Unpack everything
  this->MainFrame->Unpack();
  this->SeparatorFrame->Unpack();

  this->Script("pack %s -anchor nw -side left", this->Checkbox->GetWidgetName());
  this->Script("pack %s -anchor nw -expand t -fill x -side top", 
              this->NestedSeparatorFrame->GetWidgetName());
  this->Script("%s configure -height 12",
              this->NestedSeparatorFrame->GetWidgetName()); 
  this->Script("pack %s -fill x -expand t -side left", 
              this->LabelFrame->GetWidgetName());
  this->Script("pack %s -anchor w -fill x -expand t", 
              this->MainFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -expand t -fill x", 
              this->SeparatorFrame->GetWidgetName());
  this->Script("%s configure -height 12",
              this->SeparatorFrame->GetWidgetName());

}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::DragAndDropStartCallback(int, int )
{
  this->ToggleNestedLabels(this->LabelFrame,1);

}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::DragAndDropEndCallback(int, int)
{
  this->ToggleNestedLabels(this->LabelFrame,0);
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::SetSelectionState(int flag)
{
  this->ToggleNestedCheckBoxes(this, flag);
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::SelectCallback()
{
  if(this->Checkbox->GetState())
    {
    this->ToggleNestedCheckBoxes(this->LabelFrame, 1);
    }
  else
    {
    this->ToggleNestedCheckBoxes(this->LabelFrame, 0);
    }
}

//----------------------------------------------------------------------------
int vtkKWLookmarkFolder::GetSelectionState()
{
  return this->Checkbox->GetState();
}


void vtkKWLookmarkFolder::ToggleNestedCheckBoxes(vtkKWWidget *parent, int onoff)
{
  int nb_children = parent->GetNumberOfChildren();
  for (int i = 0; i < nb_children; i++)
    {
    vtkKWWidget *widget = parent->GetNthChild(i);
    if(widget->IsA("vtkKWCheckButton") && widget->IsPacked())
      {
      vtkKWCheckButton *checkBox = vtkKWCheckButton::SafeDownCast(widget);
      if(checkBox)
        {
        checkBox->SetState(onoff);
        }
      //else cleanup
      }
    else if(!widget->IsA("vtkKWCheckButtonLabeled"))
      {
        this->ToggleNestedCheckBoxes(widget, onoff);
      }
    }
}


//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::ToggleNestedLabels(vtkKWWidget *widget, int onoff)
{
  vtkKWWidget *anchor;
  vtkKWLookmark *lmkWidget;
  vtkKWLookmarkFolder *lmkFolder;

 
  if(!widget)
    {
    return;
    }

  if( widget->IsA("vtkKWLookmark") && widget->IsPacked())
    {
    lmkWidget = vtkKWLookmark::SafeDownCast(widget);
    if(lmkWidget)
      {
//      lmkWidget->SetLookmarkState(onoff);
      double fr, fg, fb, br, bg, bb;
      anchor = lmkWidget->GetDragAndDropTargetSet()->GetSourceAnchor();
      anchor->GetForegroundColor(&fr, &fg, &fb);
      anchor->GetBackgroundColor(&br, &bg, &bb);
      anchor->SetForegroundColor(br, bg, bb);
      anchor->SetBackgroundColor(fr, fg, fb);
      }
    }
  else if( widget->IsA("vtkKWLookmarkFolder") && widget->IsPacked())
    {
    lmkFolder = vtkKWLookmarkFolder::SafeDownCast(widget);
    if(lmkFolder)
      {
//      if(lmkFolder->GetSelectionFlag() != onoff)
//        {
        double fr, fg, fb, br, bg, bb;
        anchor = lmkFolder->GetDragAndDropTargetSet()->GetSourceAnchor();
        anchor->GetForegroundColor(&fr, &fg, &fb);
        anchor->GetBackgroundColor(&br, &bg, &bb);
        anchor->SetForegroundColor(br, bg, bb);
        anchor->SetBackgroundColor(fr, fg, fb);

//        lmkFolder->SetSelectionFlag(onoff);
//        }
      lmkFolder->ToggleNestedLabels(lmkFolder->GetLabelFrame()->GetFrame(), onoff);
      }
    }
  else
    {
    int nb_children = widget->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->ToggleNestedLabels(widget->GetNthChild(i), onoff);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

//  this->PropagateEnableState(this->MainFrame);
  this->PropagateEnableState(this->LabelFrame);
//  this->PropagateEnableState(this->CheckBox);
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Location: " << this->Location << endl;
  os << indent << "LabelFrame: " << this->LabelFrame << endl;
  os << indent << "SeparatorFrame: " << this->SeparatorFrame << endl;
  os << indent << "NestedSeparatorFrame: " << 
    this->NestedSeparatorFrame << endl;
}
