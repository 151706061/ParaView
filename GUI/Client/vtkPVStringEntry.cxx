/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVStringEntry.h"

#include "vtkArrayMap.txx"
#include "vtkClientServerStream.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkSMStringVectorProperty.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVStringEntry);
vtkCxxRevisionMacro(vtkPVStringEntry, "1.35");

//----------------------------------------------------------------------------
vtkPVStringEntry::vtkPVStringEntry()
{
  this->LabelWidget = vtkKWLabel::New();
  this->LabelWidget->SetParent(this);
  this->Entry = vtkKWEntry::New();
  this->Entry->SetParent(this);
  this->EntryLabel = 0;

  this->InitSourceVariable = 0;
}

//----------------------------------------------------------------------------
vtkPVStringEntry::~vtkPVStringEntry()
{
  this->Entry->Delete();
  this->Entry = NULL;
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
  this->SetEntryLabel(0);
  this->SetInitSourceVariable(0);;
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetLabel(label);
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::SetBalloonHelpString(const char *str)
{

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->GetApplication() && !this->BalloonHelpInitialized )
    {
    this->LabelWidget->SetBalloonHelpString(this->BalloonHelpString);
    this->Entry->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::Create(vtkKWApplication *pvApp)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // For getting the widget in a script.
  if (this->EntryLabel && this->EntryLabel[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(this->EntryLabel);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
  
  // Now a label
  if (this->EntryLabel && this->EntryLabel[0] != '\0')
    {
    this->LabelWidget->Create(pvApp, "-width 18 -justify right");
    this->LabelWidget->SetLabel(this->EntryLabel);
    this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());
    }
  
  // Now the entry
  this->Entry->Create(pvApp, "");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->Entry->GetWidgetName(), this->GetTclName());
  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(this->BalloonHelpString);
    }
  this->Script("pack %s -side left -fill x -expand t",
               this->Entry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::SetValue(const char* fileName)
{
  const char *old;
  
  if (fileName == NULL)
    {
    fileName = "";
    }

  old = this->Entry->GetValue();
  if ( old && fileName )
    {
    if (strcmp(old, fileName) == 0)
      {
      return;
      }
    }

  this->Entry->SetValue(fileName); 

  this->ModifiedCallback();
}
  
//----------------------------------------------------------------------------
void vtkPVStringEntry::Accept()
{
  int modFlag = this->GetModifiedFlag();
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (svp)
    {
    svp->SetElement(0, this->GetValue());
    }
  
  this->ModifiedFlag = 0;
  
  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

  this->AcceptCalled = 1;
}

//---------------------------------------------------------------------------
void vtkPVStringEntry::Trace(ofstream *file)
{
  if (this->InitializeTrace(file))
    {
    *file << "$kw(" << this->GetTclName() << ") SetValue {"
          << this->GetValue() << "}" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if ( this->ObjectID.ID && this->InitSourceVariable && !this->AcceptCalled)
    {
    vtkstd::string method = "Get";
    method += this->InitSourceVariable;
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->ObjectID << method.c_str()
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT);
    const char* value;
    if(pm->GetLastServerResult().GetArgument(0, 0, &value))
      {
      this->SetValue(value);
      }
    else
      {
      vtkErrorMacro("Error getting \"" << this->InitSourceVariable
                    << "\" value from server.");
      }
    }

  if (svp)
    {
    this->SetValue(svp->GetElement(0));
    }
  
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//----------------------------------------------------------------------------
vtkPVStringEntry* vtkPVStringEntry::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVStringEntry::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::CopyProperties(vtkPVWidget* clone, 
                                      vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVStringEntry* pvse = vtkPVStringEntry::SafeDownCast(clone);
  if (pvse)
    {
    pvse->SetLabel(this->EntryLabel);
    pvse->SetInitSourceVariable(this->InitSourceVariable);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVStringEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVStringEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetLabel(label);
    }
  else
    {
    this->SetLabel(this->VariableName);
    }

  const char* init_source = element->GetAttribute("init_source");
  if ( init_source )
    {
    this->SetInitSourceVariable(init_source);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkPVStringEntry::GetValue() 
{
  return this->Entry->GetValue();
}

//-----------------------------------------------------------------------------
void vtkPVStringEntry::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  
  if (sourceID.ID == 0 || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }

  *file << "  [$pvTemp" << sourceID << " GetProperty " << this->SMPropertyName
        << "] SetElement 0 {" << this->GetValue() << "}" << endl;
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabelWidget);
  this->PropagateEnableState(this->Entry);
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
