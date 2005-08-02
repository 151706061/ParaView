/*=========================================================================

  Module:    vtkPVCornerAnnotationEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCornerAnnotationEditor.h"

#include "vtkCornerAnnotation.h"
#include "vtkKWCheckButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWText.h"
#include "vtkKWTextWithLabel.h"
#include "vtkPVTextPropertyEditor.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkPVTraceHelper.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVCornerAnnotationEditor );
vtkCxxRevisionMacro(vtkPVCornerAnnotationEditor, "1.10");

//----------------------------------------------------------------------------
vtkPVCornerAnnotationEditor::vtkPVCornerAnnotationEditor()
{
  this->TraceHelper = vtkPVTraceHelper::New();
  this->TraceHelper->SetObject(this);

  this->InternalCornerAnnotation = NULL;

  this->View= NULL;

  // Delete the vtkKWTextPropertyEditor, use the traced one, vtkPVTextPropertyEditor

  if (this->TextPropertyWidget)
    {
    this->TextPropertyWidget->Delete();
    }
  
  this->TextPropertyWidget = vtkPVTextPropertyEditor::New();
  vtkPVTextPropertyEditor *pvtpropw = 
    vtkPVTextPropertyEditor::SafeDownCast(this->TextPropertyWidget);
  pvtpropw->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
  pvtpropw->GetTraceHelper()->SetReferenceCommand(
    "GetTextPropertyWidget");
}

//----------------------------------------------------------------------------
vtkPVCornerAnnotationEditor::~vtkPVCornerAnnotationEditor()
{
  this->SetView(NULL);

  if (this->TraceHelper)
    {
    this->TraceHelper->Delete();
    this->TraceHelper = NULL;
    }

  if (this->InternalCornerAnnotation)
    {
    this->InternalCornerAnnotation->Delete();
    this->InternalCornerAnnotation = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::SetView(vtkKWView *vw)
{ 
  vtkPVRenderView* rw = vtkPVRenderView::SafeDownCast(vw);

  if (this->View == rw) 
    {
    return;
    }

  if (this->View != NULL) 
    { 
    this->View->UnRegister(this); 
    }

  this->View = rw;

  // We are now in vtkKWView mode, create the corner prop and the composite

  if (this->View != NULL) 
    { 
    this->View->Register(this); 
    if (!this->InternalCornerAnnotation)
      {
      this->InternalCornerAnnotation = vtkCornerAnnotation::New();
      this->InternalCornerAnnotation->SetMaximumLineHeight(0.07);
      this->InternalCornerAnnotation->VisibilityOff();
      }
    this->CornerAnnotation = this->InternalCornerAnnotation;
    }
  else
    {
    this->CornerAnnotation = NULL;
    }

  this->Modified();

  // Update the GUI. Test if it is alive because we might be in the middle
  // of destructing the whole GUI

  if (this->IsAlive())
    {
    this->Update();
    }
} 

//----------------------------------------------------------------------------
int vtkPVCornerAnnotationEditor::GetVisibility() 
{
  // Note that the visibility here is based on the real visibility of the
  // annotation, not the state of the checkbutton

  return (this->CornerAnnotation &&
          this->CornerAnnotation->GetVisibility()) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::SetVisibility(int state)
{
  // In vtkKWView mode, add/remove the composite
  // In vtkKWRenderWidget mode, add/remove the prop

  int old_visibility = this->GetVisibility();

  if (this->CornerAnnotation)
    {
    if (state)
      {
      this->CornerAnnotation->VisibilityOn();
      if (this->View)
        {
        this->View->AddAnnotationProp(this);
        }
      }
    else
      {
      this->CornerAnnotation->VisibilityOff();
      if (this->View)
        {
        this->View->RemoveAnnotationProp(this);
        }
      }
    }

  if (old_visibility != this->GetVisibility())
    {
    this->Update();
    this->Render();
    this->SendChangedEvent();
    this->GetTraceHelper()->AddEntry("$kw(%s) SetVisibility %d", this->GetTclName(), state);
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::UpdateCornerText() 
{
  if (this->IsCreated())
    {
    for (int i = 0; i < 4; i++)
      {
      if (this->CornerText[i])
        {
        this->SetCornerTextInternal(
          this->CornerText[i]->GetWidget()->GetValue(), i);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::SetCornerTextInternal(const char* text, int corner) 
{
  if (this->CornerAnnotation &&
      (!this->GetCornerText(corner) ||
       strcmp(this->GetCornerText(corner), text)))
    {
#if 0
    this->CornerAnnotation->SetText(corner, text);
#else
    // Note: this is a special hack to allow Tcl commands to be
    // entered in the editor (Exodus reader for example) and
    // evaluated directly.
    this->CornerAnnotation->SetText(
      corner, this->Script("%s \"%s\"", "set pvCATemp", text));
#endif
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::CornerTextCallback(int i) 
{
  if (this->IsCreated() && this->CornerText[i])
    {
    char* text = this->CornerText[i]->GetWidget()->GetValue();
    this->SetCornerTextInternal(text, i);

    this->Update();

    if (this->GetVisibility())
      {
      this->Render();
      }

    this->SendChangedEvent();

    vtksys_stl::string escaped(
      this->ConvertInternalStringToTclString(
        text, vtkKWCoreWidget::ConvertStringEscapeInterpretable));
    
    this->GetTraceHelper()->AddEntry(
      "$kw(%s) SetCornerText \"%s\" %d", 
      this->GetTclName(), escaped.c_str(), i);
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::SetMaximumLineHeight(float v)
{
  this->Superclass::SetMaximumLineHeight(v);
  this->GetTraceHelper()->AddEntry(
    "$kw(%s) SetMaximumLineHeight %f", this->GetTclName(), v);
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::SetCornerText(const char *text, int corner) 
{
  char* oldValue = this->CornerText[corner]->GetWidget()->GetValue();
  if (this->CornerAnnotation && (strcmp(oldValue, text)))
    {
    this->CornerText[corner]->GetWidget()->SetValue(text);
    this->SetCornerTextInternal(text, corner);

    this->Update();

    if (this->GetVisibility())
      {
      this->Render();
      }

    this->SendChangedEvent();

    vtksys_stl::string escaped(
      this->ConvertInternalStringToTclString(
        text, vtkKWCoreWidget::ConvertStringEscapeInterpretable));
    
    this->GetTraceHelper()->AddEntry(
      "$kw(%s) SetCornerText \"%s\" %d", 
      this->GetTclName(), escaped.c_str(), corner);
    }
}


//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::Update() 
{
  // Maximum line height

  if (this->MaximumLineHeightScale && this->CornerAnnotation)
    {
    this->MaximumLineHeightScale->SetValue(
      this->CornerAnnotation->GetMaximumLineHeight());
    }

  // Text property

  if (this->TextPropertyWidget)
    {
    this->TextPropertyWidget->SetTextProperty(
      this->CornerAnnotation ? this->CornerAnnotation->GetTextProperty():NULL);
    this->TextPropertyWidget->SetActor2D(this->CornerAnnotation);
    this->TextPropertyWidget->Update();
    }

  if (this->CheckButton && this->CornerAnnotation)
    {
    this->CheckButton->SetSelectedState(this->CornerAnnotation->GetVisibility());
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::Render() 
{
  if (this->View)
    {
    this->View->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::SaveState(ofstream *file)
{
  *file << "$kw(" << this->GetTclName() << ") SetVisibility "
        << this->GetVisibility() << endl;
  
  int i;
  for (i = 0; i < 4; i++)
    {
    *file << "$kw(" << this->GetTclName() << ") SetCornerText {";
    if (this->CornerText[i]->GetWidget()->GetValue())
      {
      *file << this->CornerText[i]->GetWidget()->GetValue();
      }
    *file << "} " << i << endl;
    }
  
  *file << "$kw(" << this->GetTclName() << ") SetMaximumLineHeight "
        << this->GetCornerAnnotation()->GetMaximumLineHeight() << endl;
  
  *file << "set kw(" << this->TextPropertyWidget->GetTclName()
        << ") [$kw(" << this->GetTclName() << ") GetTextPropertyWidget]"
        << endl;
  char *tclName =
    new char[10 + strlen(this->TextPropertyWidget->GetTclName())];
  sprintf(tclName, "$kw(%s)", this->TextPropertyWidget->GetTclName());
  this->TextPropertyWidget->SaveInTclScript(file, tclName, 0);
  delete [] tclName;
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotationEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "TraceHelper: " << this->TraceHelper << endl;
}

