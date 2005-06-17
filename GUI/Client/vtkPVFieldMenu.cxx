/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFieldMenu.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVFieldMenu.h"

#include "vtkArrayMap.txx"
#include "vtkDataSet.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVInputProperty.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSource.h"
#include "vtkPVTraceHelper.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVFieldMenu);
vtkCxxRevisionMacro(vtkPVFieldMenu, "1.28");


vtkCxxSetObjectMacro(vtkPVFieldMenu, InputMenu, vtkPVInputMenu);


int vtkPVFieldMenuCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVFieldMenu::vtkPVFieldMenu()
{
  this->CommandFunction = vtkPVFieldMenuCommand;
  
  this->InputMenu = NULL;
  this->Label = vtkKWLabel::New();
  this->FieldMenu = vtkKWOptionMenu::New();
  this->Value = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  
}

//----------------------------------------------------------------------------
vtkPVFieldMenu::~vtkPVFieldMenu()
{
  this->Label->Delete();
  this->Label = NULL;
  this->FieldMenu->Delete();
  this->FieldMenu = NULL;

  this->SetInputMenu(NULL);
  
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
  if (this->Value == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    os << indent << "Value: Point Data. \n";
    }
  if (this->Value == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    os << indent << "Value: Cell Data. \n";
    }
}

//----------------------------------------------------------------------------
vtkPVInputProperty* vtkPVFieldMenu::GetInputProperty()
{
  if (this->PVSource == NULL)
    {
    return NULL;
    }

  // Should we get the input name from the input menu?
  return this->PVSource->GetInputProperty("Input");
}


//----------------------------------------------------------------------------
void vtkPVFieldMenu::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  this->Label->SetParent(this);
  this->Label->Create(app);
  this->Label->SetWidth(18);
  this->Label->SetJustificationToRight();
  this->Label->SetText("Attribute Mode");
  this->Label->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->FieldMenu->SetParent(this);
  this->FieldMenu->Create(app);
  this->FieldMenu->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->Script("pack %s -side left", this->FieldMenu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::SetValue(int field)
{
  if (field == this->Value)
    {
    return;
    }

  vtkSMProperty* prop = this->GetSMProperty();
  if (prop)
    {
    vtkSMEnumerationDomain* edom = vtkSMEnumerationDomain::SafeDownCast(
      prop->GetDomain("field_list"));
    if (edom)
      {
      unsigned int numEntries = edom->GetNumberOfEntries();
      for (unsigned int i=0; i<numEntries; i++)
        {
        if (field == edom->GetEntryValue(i))
          {
          this->FieldMenu->SetValue(edom->GetEntryText(i));
          }
        }
      }
    else
      {
      vtkErrorMacro("Required domain (field_list) could not be found");
      }
    }
  
  this->Value = field;
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::Accept()
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (svp)
    {
    ostrstream cmd_with_warning_C4701;
    cmd_with_warning_C4701 << this->Value << ends;
    svp->SetElement(3, cmd_with_warning_C4701.str());
    delete[] cmd_with_warning_C4701.str();
    }

  this->Superclass::Accept();
}

//---------------------------------------------------------------------------
void vtkPVFieldMenu::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetValue "
        << this->Value << endl;
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::Initialize()
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::ResetInternal()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (ivp)
    {
    this->SetValue(ivp->GetElement(0));
    }

  this->ModifiedFlag = 0;

  // Do we really need to update?
  // What causes dependent widgets like ArrayMenu to update?
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::UpdateProperty()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (ivp)
    {
    ivp->SetUncheckedElement(0, this->Value);
    ivp->UpdateDependentDomains();
    }
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::Update()
{
  vtkSMProperty* prop = this->GetSMProperty();
  if (prop)
    {
    vtkSMEnumerationDomain* edom = vtkSMEnumerationDomain::SafeDownCast(
      prop->GetDomain("field_list"));
    if (edom)
      {
      int valSet = 0;
      unsigned int numEntries = edom->GetNumberOfEntries();
      for (unsigned int i=0; i<numEntries; i++)
        {
        if (this->Value == edom->GetEntryValue(i))
          {
          valSet = 1;
          }
        }
      if (!valSet)
        {
        if (numEntries > 0)
          {
          this->Value = edom->GetEntryValue(0);
          }
        }
      }
    else
      {
      vtkErrorMacro("Required property (field_list) could not be found.");
      }
    }

  this->UpdateProperty();

  this->FieldMenu->DeleteAllEntries();
  if (prop)
    {
    vtkSMEnumerationDomain* edom = vtkSMEnumerationDomain::SafeDownCast(
      prop->GetDomain("field_list"));
    if (edom)
      {
      const char* valid = 0;
      unsigned int numEntries = edom->GetNumberOfEntries();
      for (unsigned int i=0; i<numEntries; i++)
        {
        ostrstream com;
        com << "SetValue " << edom->GetEntryValue(i) << ends;
        this->FieldMenu->AddEntryWithCommand(
          edom->GetEntryText(i), this, com.str());
        delete[] com.str();
        if (this->Value == edom->GetEntryValue(i))
          {
          valid = edom->GetEntryText(i);
          }
        }
      if (valid)
        {
        this->FieldMenu->SetCurrentEntry(valid);
        }
      }
    else
      {
      vtkErrorMacro("Required domain (field_list) could not be found.");
      }
    }

  // This updates any array menu dependent on this widget.
  this->Superclass::Update();
}


//----------------------------------------------------------------------------
vtkPVFieldMenu* vtkPVFieldMenu::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVFieldMenu::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
// It looks like I could leave this for the superclass.!!!!!!!!!!!!!!!
vtkPVWidget* vtkPVFieldMenu::ClonePrototypeInternal(vtkPVSource* pvSource,
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

    vtkPVFieldMenu* pvfm = vtkPVFieldMenu::SafeDownCast(pvWidget);
    if (!pvfm)
      {
      vtkErrorMacro("Internal error. Could not downcast pointer.");
      pvWidget->Delete();
      return 0;
      }
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

//----------------------------------------------------------------------------
void vtkPVFieldMenu::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  
  vtkPVFieldMenu* pvamm = vtkPVFieldMenu::SafeDownCast(clone);
  if (pvamm)
    {
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvamm->SetInputMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to vtkPVAttributeMenu.");
    }

}

//----------------------------------------------------------------------------
int vtkPVFieldMenu::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
    
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if(!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }
  
  vtkPVXMLElement* ime = element->LookupElement(input_menu);
  if (!ime)
    {
    vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
    return 0;
    }
  
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

//----------------------------------------------------------------------------
void vtkPVFieldMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->FieldMenu);
  this->PropagateEnableState(this->InputMenu);
}
