/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCutEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCutEntry.h"

#include "vtkPVInputMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCutEntry);
vtkCxxRevisionMacro(vtkPVCutEntry, "1.4");

vtkCxxSetObjectMacro(vtkPVCutEntry, InputMenu, vtkPVInputMenu);

//-----------------------------------------------------------------------------
int vtkPVCutEntryCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);


//-----------------------------------------------------------------------------
vtkPVCutEntry::vtkPVCutEntry()
{
  this->CommandFunction = vtkPVCutEntryCommand;
  this->InputMenu = NULL;
}

//-----------------------------------------------------------------------------
vtkPVCutEntry::~vtkPVCutEntry()
{
  this->SetInputMenu(NULL);
}

//-----------------------------------------------------------------------------
int vtkPVCutEntry::ComputeWidgetRange()
{
  if (!this->InputMenu)
    {
    return 0;
    }
  vtkPVSource* input = this->InputMenu->GetCurrentValue();
  if (!input)
    {
    return 0;
    }
  
  double bounds[6];
  input->GetDataInformation()->GetBounds(bounds);
  float length = sqrt(
    static_cast<double>(
      (bounds[1]-bounds[0])*(bounds[1]-bounds[0])+
      (bounds[3]-bounds[2])*(bounds[3]-bounds[2])+
      (bounds[5]-bounds[4])*(bounds[5]-bounds[4])));
  
  this->WidgetRange[0] = -length;
  this->WidgetRange[1] =  length;
  this->UseWidgetRange = 1;
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVCutEntry::CopyProperties(
  vtkPVWidget* clone, 
  vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVCutEntry* pvce = vtkPVCutEntry::SafeDownCast(clone);
  if (pvce)
    {
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvce->SetInputMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVCutEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVCutEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if (!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }

  vtkPVXMLElement* ime = element->LookupElement(input_menu);
  vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
  vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
  if(!imw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
    return 0;
    }
  imw->AddDependent(this);
  this->SetInputMenu(imw);
  imw->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVCutEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
}
