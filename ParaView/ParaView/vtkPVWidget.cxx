/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVWidget.h"
#include "vtkPVSource.h"
#include "vtkPVApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkCollection.h"
#include "vtkArrayMap.txx"
#include "vtkLinkedList.txx"
#include "vtkPVWidgetProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLPackageParser.h"

#include <ctype.h>

#ifndef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION

template class VTK_EXPORT vtkAbstractList<void*>;
template class VTK_EXPORT vtkLinkedList<void*>;
template class VTK_EXPORT vtkAbstractIterator<vtkIdType,void*>;
template class VTK_EXPORT vtkLinkedListIterator<void*>;
template class VTK_EXPORT vtkAbstractMap<vtkPVWidget*, vtkPVWidget*>;
template class VTK_EXPORT vtkArrayMap<vtkPVWidget*, vtkPVWidget*>;
template class VTK_EXPORT vtkAbstractIterator<vtkPVWidget*, vtkPVWidget*>;
template class VTK_EXPORT vtkArrayMapIterator<vtkPVWidget*, vtkPVWidget*>;

#endif

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPVWidget, "1.43");

//-----------------------------------------------------------------------------
vtkPVWidget::vtkPVWidget()
{
  this->ModifiedCommandObjectTclName = NULL;
  this->ModifiedCommandMethod = NULL;

  this->AcceptedCommandObjectTclName = NULL;
  this->AcceptedCommandMethod = NULL;

  // Start modified because empty widgets do not match their variables.
  this->ModifiedFlag = 1;

  this->Dependents = vtkLinkedList<void*>::New();
  this->PVSource = 0;

  this->TraceNameState = vtkPVWidget::Uninitialized;
  this->SuppressReset = 0;
  this->AcceptCalled = 0;
  
  this->UseWidgetRange = 0;
  this->WidgetRange[0] = 0;
  this->WidgetRange[1] = 0;
}

//-----------------------------------------------------------------------------
vtkPVWidget::~vtkPVWidget()
{
  this->SetModifiedCommandObjectTclName(NULL);
  this->SetModifiedCommandMethod(NULL);
  this->SetAcceptedCommandObjectTclName(NULL);
  this->SetAcceptedCommandMethod(NULL);

  this->Dependents->Delete();
  this->Dependents = NULL;
}


//-----------------------------------------------------------------------------
void vtkPVWidget::SetModifiedCommand(const char* cmdObject, 
                                     const char* methodAndArgs)
{
  this->SetModifiedCommandObjectTclName(cmdObject);
  this->SetModifiedCommandMethod(methodAndArgs);
}

//-----------------------------------------------------------------------------
void vtkPVWidget::SetAcceptedCommand(const char* cmdObject, 
                                     const char* methodAndArgs)
{
  this->SetAcceptedCommandObjectTclName(cmdObject);
  this->SetAcceptedCommandMethod(methodAndArgs);
}


//-----------------------------------------------------------------------------
void vtkPVWidget::Accept()
{
  int num, idx;
  int modFlag = this->GetModifiedFlag();


  if (this->PVSource == NULL)
    {
    vtkErrorMacro("Superclass expect PVSource to be set. " 
                  << this->GetClassName());
    return;
    }

  num = this->PVSource->GetNumberOfVTKSources();
  for (idx = 0; idx < num; ++idx)
    {
    this->AcceptInternal(this->PVSource->GetVTKSourceID(idx));
    }

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

//-----------------------------------------------------------------------------
void vtkPVWidget::AcceptInternal(vtkClientServerID)
{
  // Suppress reset just allows the widget to set its own default value.
  // After accept is called, we want to enable the reset.
  this->SuppressReset = 0;
}

//-----------------------------------------------------------------------------
// I have to think how to get rid of this method.
void vtkPVWidget::Reset()
{
  if (this->SuppressReset)
    {
    return;
    }

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("Superclass expect PVSource to be set. " 
                  << this->GetClassName());
    return;
    }

  if (this->PVSource->GetNumberOfVTKSources() < 1)
    {
    return;
    }
  this->ResetInternal();
}

//-----------------------------------------------------------------------------
void vtkPVWidget::ResetInternal()
{
  // Get rid of this eventually.  Display widgets (label) do not really
  // need to implement this method.
  //vtkErrorMacro("ResetInternal not defined. " << this->GetClassName());
}






//-----------------------------------------------------------------------------
void vtkPVWidget::Update()
{
  vtkLinkedListIterator<void*>* it = this->Dependents->NewIterator();
  while ( !it->IsDoneWithTraversal() )
    {
    void* pvwp = 0;
    if (it->GetData(pvwp) == VTK_OK && pvwp)
      {
      static_cast<vtkPVWidget*>(pvwp)->Update();
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVWidget::AddDependent(vtkPVWidget *pvw)
{
  this->Dependents->AppendItem(pvw);  
}

void vtkPVWidget::RemoveDependent(vtkPVWidget *pvw)
{
  vtkIdType index = 0;

  if ( this->Dependents->FindItem(pvw, index) == VTK_OK )
    {
    this->Dependents->RemoveItem(index);
    }
}

void vtkPVWidget::RemoveAllDependents()
{
  this->Dependents->RemoveAllItems();
}

//-----------------------------------------------------------------------------
void vtkPVWidget::ModifiedCallback()
{
  this->ModifiedFlag = 1;
  
  if (this->ModifiedCommandObjectTclName && this->ModifiedCommandMethod &&
      this->Application)
    {
    this->Script("%s %s", this->ModifiedCommandObjectTclName,
                 this->ModifiedCommandMethod);
    }
}

//-----------------------------------------------------------------------------
void vtkPVWidget::AcceptedCallback()
{
  if (this->AcceptedCommandObjectTclName && this->AcceptedCommandMethod)
    {
    this->Script("%s %s", this->AcceptedCommandObjectTclName,
                 this->AcceptedCommandMethod);
    }
}

vtkPVApplication *vtkPVWidget::GetPVApplication() 
{
  return vtkPVApplication::SafeDownCast(this->Application);
}

//-----------------------------------------------------------------------------
void vtkPVWidget::SaveInBatchScript(ofstream *file)
{
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("SaveInBatchScript requires a PVSource.")
    return;
    }

  int numSources, sourceIdx;
  numSources = this->PVSource->GetNumberOfVTKSources();
  for (sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
    {
    this->SaveInBatchScriptForPart(file, 
                                   this->PVSource->GetVTKSourceID(sourceIdx));
    }
}

//-----------------------------------------------------------------------------
void vtkPVWidget::SaveState(ofstream *file)
{
  this->Trace(file);
}

//-----------------------------------------------------------------------------
void vtkPVWidget::SaveInBatchScriptForPart(ofstream*, vtkClientServerID)
{
  // Either SaveInBatchScript or SaveInBatchScriptForPart
  // must be defined.
  vtkErrorMacro("Method SaveInBatchScriptForPart not defined in subclass: " 
    << this->GetClassName());
}


//-----------------------------------------------------------------------------
vtkPVWidget* vtkPVWidget::ClonePrototypeInternal(vtkPVSource* pvSource,
                                vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* pvWidget = 0;
  // Check if a clone of this widget has already been created
  if ( map->GetItem(this, pvWidget) != VTK_OK )
    {
    // If not, create one and add it to the map
    pvWidget = this->NewInstance();
    map->SetItem(this, pvWidget);
    // Now copy all the properties
    this->CopyProperties(pvWidget, pvSource, map);
    }
  else
    {
    // Increment the reference count. This is necessary
    // to make the behavior same whether a widget is created
    // or returned from the map. Always call Delete() after
    // cloning.
    pvWidget->Register(this);
    }
  return pvWidget;
}

//-----------------------------------------------------------------------------
void vtkPVWidget::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  // Copy the tracename and help (note that SetBalloonHelpString
  // is virtual and is redefined by subclasses when necessary).
  clone->SetTraceName(this->GetTraceName());
  clone->SetTraceNameState(this->TraceNameState);
  clone->SetBalloonHelpString(this->GetBalloonHelpString());
  clone->SetDebug(this->GetDebug());
  clone->SuppressReset = this->SuppressReset;
  
  // Now copy the dependencies
  vtkPVWidget* dep;
  vtkPVWidget* clonedep;

  vtkLinkedListIterator<void*>* it = this->Dependents->NewIterator();
  while ( !it->IsDoneWithTraversal() )
    {
    void* pvwp = 0;
    if (it->GetData(pvwp) == VTK_OK && pvwp)
      {
      dep = static_cast<vtkPVWidget*>(pvwp);
      
      // We call clone. If there is already a copy of this
      // widget, it will be returned. Otherwise, a new one will
      // be created.
      clonedep = dep->ClonePrototype(pvSource, map);
      clone->Dependents->AppendItem(clonedep);
      // Although we are not registering this widget when
      // adding it to the dependent list, we still call Delete
      // because ClonePrototype() always increases the reference
      // count by one. There is not danger of the widget being
      // deleted even if it was created in ClonePrototype because
      // the reference count will be 2 (1 when it is first created,
      // 2 when it is added to the map). A problem may arise if
      // the widget is not referenced by another container before
      // the end of the cloning process. If the map is deleted and
      // the widget is not referenced by any other object, it will
      // be deleted and the pointer in the Dependents list will be
      // invalid.
      clonedep->Delete();
      }
    it->GoToNextItem();
    }
  it->Delete();

  clone->SetPVSource(pvSource);
  clone->SetModifiedCommand(pvSource->GetTclName(), 
                            "SetAcceptButtonColorToRed");
}

//-----------------------------------------------------------------------------
static int vtkPVWidgetIsSpace(char c)
{
  return isspace(c);
}

//-----------------------------------------------------------------------------
int vtkPVWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                   vtkPVXMLPackageParser* parser)
{
  const char* help = element->GetAttribute("help");
  if(help) { this->SetBalloonHelpString(help); }
  
  const char* trace_name = element->GetAttribute("trace_name");
  if (trace_name) 
    { 
    this->SetTraceName(trace_name); 
    this->SetTraceNameState(vtkPVWidget::XMLInitialized);
    }
  else
    {
    vtkErrorMacro("Widget is missing required trace_name attribute."
                  << " Source name: " << this->PVSource->GetName()
                  << ", widget class: " << this->GetClassName());
    // For now, do not raise error. Otherwise, old configuration
    // files might not work.
    // return 0;
    }

  if(!element->GetScalarAttribute("suppress_reset", &this->SuppressReset))
    {
    // Just use the widgets default if XML does not specify.
    //this->SuppressReset = 0;
    }

  const char* deps = element->GetAttribute("dependents");
  if(deps)
    {
    const char* start = deps;
    const char* end = 0;
    
    // Parse the space-separated list.
    while(*start)
      {
      while(*start && vtkPVWidgetIsSpace(*start)) { ++start; }
      end = start;
      while(*end && !vtkPVWidgetIsSpace(*end)) { ++end; }
      int length = end-start;
      if(length)
        {
        char* entry = new char[length+1];
        strncpy(entry, start, length);
        entry[length] = '\0';
        vtkPVXMLElement* depElement = element->LookupElement(entry);
        vtkPVWidget* dep = this->GetPVWidgetFromParser(depElement, parser);
        if(!dep)
          {
          vtkErrorMacro("Couldn't add dependent " << entry);
          delete [] entry;
          return 0;
          }
        delete [] entry;
        this->Dependents->AppendItem(dep);
        dep->Delete();
        }
      start = end;
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
vtkPVWidget* vtkPVWidget::GetPVWidgetFromParser(vtkPVXMLElement* element,
                                                vtkPVXMLPackageParser* parser)
{
  return parser->GetPVWidget(element, 0, 1);
}

//-----------------------------------------------------------------------------
vtkPVWindow* vtkPVWidget::GetPVWindowFormParser(vtkPVXMLPackageParser* parser)
{
  return parser->GetPVWindow();
}

//-----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVWidget::CreateAppropriateProperty()
{
  return vtkPVWidgetProperty::New();
}

//-----------------------------------------------------------------------------
void vtkPVWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ModifiedFlag: " << this->GetModifiedFlag() << endl;
  os << indent << "ModifiedCommandMethod: " 
     << (this->ModifiedCommandMethod?this->ModifiedCommandMethod:"(none)") 
     << endl;
  os << indent << "ModifiedCommandObjectTclName: " 
     << (this->ModifiedCommandObjectTclName?
         this->ModifiedCommandObjectTclName:"(none)") << endl;
  os << indent << "TraceNameState: " << this->TraceNameState << endl;
  os << indent << "UseWidgetRange: " << this->UseWidgetRange << endl;
  os << indent << "WidgetRange: " << this->WidgetRange[0] << " "
     << this->WidgetRange[1] << endl;
}
