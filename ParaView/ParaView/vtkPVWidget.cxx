/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWidget.cxx
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
#include "vtkPVWidget.h"
#include "vtkPVSource.h"
#include "vtkPVApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkCollection.h"
#include "vtkArrayMap.txx"
#include "vtkLinkedList.txx"
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
vtkCxxRevisionMacro(vtkPVWidget, "1.32");

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
    this->AcceptInternal(this->PVSource->GetVTKSourceTclName(idx));
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

  // Suppress reset just allows the widget to set its own default value.
  // After accept is called, we want to enable the reset.
  this->SuppressReset = 0;
}

//-----------------------------------------------------------------------------
void vtkPVWidget::AcceptInternal(const char*)
{
  // Get rid of this eventually.  Display widgets (label) do not really
  // need to implement this method.
  //vtkErrorMacro("AcceptInternal not defined. " << this->GetClassName());
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
  this->ResetInternal(this->PVSource->GetVTKSourceTclName(0));
}

//-----------------------------------------------------------------------------
void vtkPVWidget::ResetInternal(const char*)
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
                  this->PVSource->GetVTKSourceTclName(sourceIdx));
    }
}

//-----------------------------------------------------------------------------
void vtkPVWidget::SaveState(ofstream *file)
{
  this->Trace(file);
}

//-----------------------------------------------------------------------------
void vtkPVWidget::SaveInBatchScriptForPart(ofstream*, const char*)
{
  // Either SaveInBatchScript or SaveInBatchScriptForPart
  // must be defined.
  vtkErrorMacro("Method not defined in subclass: " << this->GetClassName());
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
void vtkPVWidget::SerializeRevision(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeRevision(os,indent);
  os << indent << "vtkPVWidget ";
  this->ExtractRevision(os,"$Revision: 1.32 $");
}

//-----------------------------------------------------------------------------
void vtkPVWidget::SerializeSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkPVWidget* vtkPVWidget::GetPVWidgetFromParser(vtkPVXMLElement* element,
                                                vtkPVXMLPackageParser* parser)
{
  return parser->GetPVWidget(element);
}

//-----------------------------------------------------------------------------
vtkPVWindow* vtkPVWidget::GetPVWindowFormParser(vtkPVXMLPackageParser* parser)
{
  return parser->GetPVWindow();
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
}
