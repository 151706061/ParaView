/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkPVTempTessellatorEntry.h"

#include <vtkObjectFactory.h>

#include <vtkKWLabeledFrame.h>
#include <vtkKWFrame.h>
#include <vtkKWListBox.h>
#include <vtkKWCheckButton.h>
#include <vtkKWEntry.h>
#include <vtkKWLabel.h>
#include <vtkKWApplication.h>

#include <vtkPVSource.h>
#include <vtkPVInputMenu.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkPVDataInformation.h>
#include <vtkPVArrayInformation.h>
#include <vtkPVScalarListWidgetProperty.h>
#include <vtkPVXMLElement.h>

#define PLAIN "#007700"
#define EMPHS "#004400"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVTempTessellatorEntry);
vtkCxxRevisionMacro(vtkPVTempTessellatorEntry, "1.9");

//-----------------------------------------------------------------------------
class vtkTessellatorEntryData
{
public:
  vtkKWLabeledFrame* CriteriaFrame;
  vtkKWFrame* EditSubframe;

  vtkKWLabel* CriteriaInstructions;
  vtkKWListBox* ScalarFieldList;
  int LastSelectionIndex;
  vtkKWCheckButton* CriterionEnable;
  vtkKWEntry* CriterionValue;

  vtkPVInputMenu* InputMenu;
  vtkPVScalarListWidgetProperty* Property;
};

//-----------------------------------------------------------------------------
vtkPVTempTessellatorEntry::vtkPVTempTessellatorEntry()
{
  this->SetFieldCriterionCommand = NULL;
  this->ResetCriteriaCommand = NULL;
  this->InputMenu = NULL;

  this->Data = new vtkTessellatorEntryData;
  vtkTessellatorEntryData* d = this->Data;

  d->CriteriaFrame = vtkKWLabeledFrame::New();
  d->EditSubframe = vtkKWFrame::New();

  d->CriteriaInstructions = vtkKWLabel::New();
  d->ScalarFieldList = vtkKWListBox::New();
  d->LastSelectionIndex = -1;

  d->CriterionEnable = vtkKWCheckButton::New();
  d->CriterionValue = vtkKWEntry::New();

  d->Property = NULL;
}

vtkPVTempTessellatorEntry::~vtkPVTempTessellatorEntry()
{
  vtkTessellatorEntryData* d = this->Data;

  d->CriterionEnable->Delete();
  d->CriterionValue->Delete();
  d->ScalarFieldList->Delete();
  d->CriteriaInstructions->Delete();
  d->EditSubframe->Delete();
  d->CriteriaFrame->Delete();

  delete d;

  this->SetPVSource(NULL);
  this->SetInputMenu(NULL);

  if ( this->SetFieldCriterionCommand )
    {
    delete [] this->SetFieldCriterionCommand;
    }

  if ( this->ResetCriteriaCommand )
    {
    delete [] this->ResetCriteriaCommand;                                                                                              
    }
}

void vtkPVTempTessellatorEntry::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Data: " << this->Data << endl;
  os << indent << "SetFieldCriterionCommand: "
    << (this->SetFieldCriterionCommand?this->SetFieldCriterionCommand:"(null)") << endl;
  os << indent << "ResetCriteriaCommand: "
    << (this->ResetCriteriaCommand?this->ResetCriteriaCommand:"(null)") << endl;
  os << indent << "InputMenu: " << this->InputMenu << endl;
}

void vtkPVTempTessellatorEntry::Create( vtkKWApplication* app )
{
  if (this->IsCreated())
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  this->Script( "frame %s", this->GetWidgetName() );

  vtkTessellatorEntryData* d = this->Data;

  d->CriteriaFrame->SetParent( this );
  d->CriteriaFrame->SetLabel( "Tessellation Criteria" );
  d->CriteriaFrame->Create( app, "" );

  d->CriteriaInstructions->SetParent( d->CriteriaFrame->GetFrame() );
  d->CriteriaInstructions->Create( app, "" );
  d->CriteriaInstructions->SetLineType( vtkKWLabel::MultiLine );
  d->CriteriaInstructions->AdjustWrapLengthToWidthOn();
  d->CriteriaInstructions->SetLabel(
    "Select a point field from the list below. You may "
    "then alter whether the field is used to subdivide "
    "edges and, if so, what the maximum allowable error "
    "is at edge midpoints." );
  this->Script( "%s configure -anchor w", d->CriteriaInstructions->GetWidgetName() );

  d->ScalarFieldList->SetParent( d->CriteriaFrame->GetFrame() );
  d->ScalarFieldList->Create( app, "" );
  d->ScalarFieldList->SetHeight( 5 );
  d->ScalarFieldList->SetSingleClickCallback( this, "PointDataSelectedCallback" );
  d->LastSelectionIndex = -1;
  this->Script( "%s configure -font {Helvetica -12 bold}", d->ScalarFieldList->GetListbox()->GetWidgetName() );

  d->EditSubframe->SetParent( d->CriteriaFrame->GetFrame() );
  d->EditSubframe->Create( app, "" );

  d->CriterionEnable->SetParent( d->EditSubframe->GetFrame() );
  d->CriterionEnable->Create( app, "" );
  d->CriterionEnable->SetText( "" );
  d->CriterionEnable->SetEnabled( 0 );
  d->CriterionEnable->SetCommand( this, "ToggleCriterionCallback" );
  this->Script( "%s configure -anchor w", d->CriterionEnable->GetWidgetName() );

  d->CriterionValue->SetParent( d->EditSubframe->GetFrame() );
  d->CriterionValue->Create( app, "" );
  this->Script( "bind %s <KeyPress-Return> {+%s ChangeCriterionCallback }", d->CriterionValue->GetWidgetName(), this->GetTclName() );
  this->Script( "bind %s <KeyPress-Tab>    {+%s ChangeCriterionCallback }", d->CriterionValue->GetWidgetName(), this->GetTclName() );

  this->Script( "pack %s -expand yes -fill x", d->CriteriaFrame->GetWidgetName() );

  this->Script( "pack %s -expand t   -fill x", d->CriteriaInstructions->GetWidgetName() );
  this->Script( "pack %s -expand yes -fill x", d->ScalarFieldList->GetWidgetName() );
  this->Script( "pack %s -expand no  -fill x", d->EditSubframe->GetWidgetName() );

  this->Script( "pack %s -side left -expand f -fill y", d->CriterionEnable->GetWidgetName() );
  this->Script( "pack %s -side right -expand t -fill y", d->CriterionValue->GetWidgetName() );
}

void vtkPVTempTessellatorEntry::Update()
{
  vtkTessellatorEntryData* d = this->Data;

  if (this->GetApplication() == NULL || d->Property == NULL)
    return;

  d->ScalarFieldList->DeleteAll();
  d->LastSelectionIndex = -1;
  vtkPVDataSetAttributesInformation* pdi = this->GetPointDataInformation();
  d->CriterionEnable->SetEnabled( 0 );
  d->CriterionValue->SetEnabled( 0 );
  if ( (! pdi) || (pdi->GetNumberOfArrays() == 0) )
    {
    d->ScalarFieldList->SetEnabled( 0 );
    return;
    }
  d->ScalarFieldList->SetEnabled( 1 );

  int numberOfArrays = pdi->GetNumberOfArrays();
  int a;
  for ( a = 0; a < numberOfArrays; ++a )
    {
    const char *name = pdi->GetArrayInformation( a )->GetName();
    char *listEntry = new char[strlen(name) + 20];
    sprintf( listEntry, "%s: inactive", name );
    d->ScalarFieldList->AppendUnique( listEntry );
    this->Script( "%s itemconfigure %d -foreground " PLAIN , d->ScalarFieldList->GetListbox()->GetWidgetName(), a );
    delete[] listEntry;
    }
  
  if ( d->Property->GetNumberOfScalars()/2 != numberOfArrays )
    {
    float* scalars = new float[ 2*numberOfArrays ];
    for ( a = 0; a < numberOfArrays; ++a )
      {
      scalars[2*a  ] = a;
      scalars[2*a+1] = -1.;
      }
    d->Property->SetScalars( 2*numberOfArrays, scalars );
    }

 // that's all for now. Eventually, this should contact the
  // server and ask for each field's current max chord error or something
  this->Superclass::Update();
}

void vtkPVTempTessellatorEntry::ToggleCriterionCallback()
{
  int fnum = this->Data->ScalarFieldList->GetSelectionIndex();
  const char* field = this->Data->ScalarFieldList->GetSelection();
  if ( ! field )
    {
    if ( this->Data->LastSelectionIndex >= 0 )
      {
      fnum = this->Data->LastSelectionIndex;
      this->Data->ScalarFieldList->GetItem( fnum );
      }

    if ( ! field )
      {
      this->Data->CriterionEnable->SetState( 0 );
      this->Data->CriterionEnable->SetEnabled( 0 );
      this->Data->CriterionValue->SetEnabled( 0 );
      return;
      }
    }
  int fnl = strlen( field ) - 1;

  while ( fnl && field[ fnl ] != ':' )
    fnl--;

  if ( this->Data->CriterionEnable->GetState() == 1 )
    { // User just toggled it on, so put in a default value
    this->Data->CriterionValue->SetEnabled( 1 );
    this->Data->CriterionValue->SetValue( 1.e-5 );

    if ( field[fnl] == ':' )
      {
      char* listEntry = new char[ fnl + 25 ];
      strncpy( listEntry, field, fnl );
      sprintf( listEntry + fnl, ": %g", 1.e-5 );
      this->Data->ScalarFieldList->DeleteRange( fnum, fnum );
      this->Data->ScalarFieldList->InsertEntry( fnum, listEntry );
      this->Data->ScalarFieldList->SetSelectionIndex( fnum );
      delete [] listEntry;
      this->Script( "%s itemconfigure %d -foreground " EMPHS, this->Data->ScalarFieldList->GetListbox()->GetWidgetName(), fnum );
      }
    }
  else
    { // User just toggled it off, so mark it inactive
    this->Data->CriterionValue->SetEnabled( 0 );
    if ( field[fnl] == ':' )
      {
      char* listEntry = new char[ fnl + 25 ];
      strncpy( listEntry, field, fnl );
      sprintf( listEntry + fnl, ": inactive" );
      this->Data->ScalarFieldList->DeleteRange( fnum, fnum );
      this->Data->ScalarFieldList->InsertEntry( fnum, listEntry );
      this->Data->ScalarFieldList->SetSelectionIndex( fnum );
      delete [] listEntry;
      this->Script( "%s itemconfigure %d -foreground " PLAIN, this->Data->ScalarFieldList->GetListbox()->GetWidgetName(), fnum );
      }
    }
  this->ModifiedCallback();
}

void vtkPVTempTessellatorEntry::ChangeCriterionCallback()
{
  const char* field = this->Data->ScalarFieldList->GetSelection();
  int idx = this->Data->ScalarFieldList->GetSelectionIndex();
  if ( ! field )
    {
    if ( this->Data->LastSelectionIndex >= 0 )
      {
      idx = this->Data->LastSelectionIndex;
      field = this->Data->ScalarFieldList->GetItem( idx );
      }

    if ( ! field )
      {
      this->Data->CriterionEnable->SetState( 0 );
      this->Data->CriterionEnable->SetEnabled( 0 );
      this->Data->CriterionValue->SetEnabled( 0 );
      return;
      }
    }

  int fnl = strlen( field ) - 1;
  char* label = new char[ fnl + 64 /*FIXME: what's the longest a %g will print?*/ ];

  while ( fnl && field[ fnl ] != ':' )
    fnl--;

  if ( field[fnl] == ':' )
    {
    double value = this->Data->CriterionValue->GetValueAsFloat();
    if ( value <= 0.0 )
      {
      delete [] label;
      return; // don't accept zero or negative criteria
      }

    strncpy( label, field, fnl );
    sprintf( label + fnl, ": %g", value );
    this->Data->ScalarFieldList->DeleteRange( idx, idx );
    this->Data->ScalarFieldList->InsertEntry( idx, label );
    this->Data->ScalarFieldList->SetSelectionIndex( idx );
    this->Script( "%s itemconfigure %d -foreground " EMPHS, this->Data->ScalarFieldList->GetListbox()->GetWidgetName(), idx );
    }

  delete [] label;
  this->ModifiedCallback();
}

void vtkPVTempTessellatorEntry::PointDataSelectedCallback()
{
  const char* field = this->Data->ScalarFieldList->GetSelection();
  if ( ! field )
    {
    this->Data->CriterionEnable->SetEnabled( 0 );
    this->Data->CriterionValue->SetEnabled( 0 );
    return;
    }
  this->Data->LastSelectionIndex = this->Data->ScalarFieldList->GetSelectionIndex();

  int ftl = strlen( field );
  int fnl = ftl - 1;

  while ( fnl && field[ fnl ] != ':' )
    fnl--;

  if ( field[fnl] == ':' )
    {
    char* label = new char[ fnl + 1 ];
    char* value = new char[ ftl - fnl ];
    int active = strcmp( field + fnl + 2, "inactive" );

    strncpy( label, field, fnl );
    strncpy( value, field+fnl+2, ftl - fnl );
    label[fnl]= '\0';

    this->Data->CriterionEnable->SetText( label );
    this->Data->CriterionValue->SetValue( active ? value : "1.e-5" );
    delete [] label;
    delete [] value;

    this->Data->CriterionEnable->SetEnabled( 1 );
    this->Data->CriterionEnable->SetState( active ? 1 : 0 );
    this->Data->CriterionValue->SetEnabled( active ? 1 : 0 );
    }
}

void vtkPVTempTessellatorEntry::AddAnimationScriptsToMenu( 
  vtkKWMenu* , vtkPVAnimationInterfaceEntry* )
{
}

void vtkPVTempTessellatorEntry::SetLabel( const char* label )
{
  this->Data->CriteriaFrame->SetLabel( label );
  if ( label && label[0] &&
       (this->TraceNameState == vtkPVWidget::Uninitialized ||
        this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName( label );
    this->SetTraceNameState( vtkPVWidget::SelfInitialized );
    }
}

const char* vtkPVTempTessellatorEntry::GetLabel() const
{
  return this->Data->CriteriaFrame->GetLabel()->GetLabel();
}

char* vtkPVTempTessellatorEntry::GetLabel()
{
  return this->Data->CriteriaFrame->GetLabel()->GetLabel();
}

void vtkPVTempTessellatorEntry::ResetFieldCriteria()
{
  this->ModifiedCallback();

  vtkTessellatorEntryData* d = this->Data;

  d->ScalarFieldList->DeleteAll();
  d->LastSelectionIndex = -1;
  vtkPVDataSetAttributesInformation* pdi = this->GetPointDataInformation();
  d->CriterionEnable->SetEnabled( 0 );
  d->CriterionValue->SetEnabled( 0 );
  if ( (! pdi) || (pdi->GetNumberOfArrays() == 0) )
    {
    d->ScalarFieldList->SetEnabled( 0 );
    return;
    }
  d->ScalarFieldList->SetEnabled( 1 );

  int numberOfArrays = pdi->GetNumberOfArrays();
  char listEntry[512]; // FIXME: should be dynamically allocated
  int a;
  for ( a = 0; a < numberOfArrays; ++a )
    {
    sprintf( listEntry, "%s: inactive", pdi->GetArrayInformation( a )->GetName() );
    d->ScalarFieldList->AppendUnique( listEntry );
    this->Script( "%s itemconfigure %d -foreground " PLAIN, d->ScalarFieldList->GetListbox()->GetWidgetName(), a );
    }
}

void vtkPVTempTessellatorEntry::SetFieldCriterion( int fnum, float crit )
{
  vtkTessellatorEntryData* d = this->Data;
  vtkPVDataSetAttributesInformation* pdi = this->GetPointDataInformation();
  if ( !pdi || pdi->GetNumberOfArrays() <= fnum )
    return;

  const char* field = d->ScalarFieldList->GetItem(fnum);
  int flen = strlen(field);
  int want_active = crit <= 0.0;
  if ( (strcmp( field + flen - 8 /*=strlen("inactive")+1*/, "inactive" ) == 0) ^ want_active )
    {
    d->ScalarFieldList->SetSelectionIndex( fnum );
    d->CriterionEnable->SetState( want_active ? 1 : 0 );
    this->ToggleCriterionCallback();
    }

  d->CriterionValue->SetValue( crit );
  this->ChangeCriterionCallback();

  this->ModifiedCallback();
}

void vtkPVTempTessellatorEntry::AnimationMenuCallback( vtkPVAnimationInterfaceEntry* )
{
  cout << "AnimationMenuCallback called." << endl;
  this->ModifiedCallback();
}

void vtkPVTempTessellatorEntry::AcceptInternal( vtkClientServerID id )
{
  if (id.ID == 0)
    return;

  this->Superclass::AcceptInternal( id );

  this->UpdateProperty();

  int cmd;
  int ns = this->Data->Property->GetNumberOfScalars()/2;
  char** commands = new char*[ ns + 1 ];
  int*   nScalars = new int  [ ns + 1 ];

  commands[0] = this->ResetCriteriaCommand;
  nScalars[0] = 0;

  for ( cmd = 0; cmd < ns; ++cmd )
    {
    commands[cmd+1] = this->SetFieldCriterionCommand;
    nScalars[cmd+1] = 2;
    }

  this->Data->Property->SetVTKSourceID( id );
  this->Data->Property->SetVTKCommands( ns+1, commands, nScalars );
  this->Data->Property->AcceptInternal();

  delete [] commands;
}

void vtkPVTempTessellatorEntry::Trace( ofstream *file )
{
  if ( ! this->InitializeTrace(file) )
    return;

  int ns = this->Data->Property->GetNumberOfScalars()/2;
  float* scalars = this->Data->Property->GetScalars();
  (*file) << "$kw(" << this->GetTclName() << ") ResetFieldCriteria" << endl;
  for ( int s = 0; s < ns; ++s )
    (*file) << "  $kw(" << this->GetTclName() << ") SetFieldCriterion " << int(scalars[2*s]) << " " << scalars[2*s+1] << endl;
}

void vtkPVTempTessellatorEntry::ResetInternal()
{
  if ( ! this->PVSource )
    {
    vtkWarningMacro( "vtkPVTempTessellatorEntry::ResetInternal expects PVSource to be set" );
    return;
    }

  this->Superclass::ResetInternal();

  vtkTessellatorEntryData* d = this->Data;
  d->ScalarFieldList->DeleteAll();
  d->LastSelectionIndex = -1;

  vtkPVDataSetAttributesInformation* pdi = this->GetPointDataInformation();
  d->CriterionEnable->SetEnabled( 0 );
  d->CriterionValue->SetEnabled( 0 );
  if ( (! pdi) || (pdi->GetNumberOfArrays() == 0) )
    {
    d->ScalarFieldList->SetEnabled( 0 );
    return;
    }
  d->ScalarFieldList->SetEnabled( 1 );

  int numberOfArrays = pdi->GetNumberOfArrays();
  int a;
  float* scalars = d->Property->GetScalars();
  for ( a = 0; a < numberOfArrays; ++a )
    {
    float scalar = scalars[ 2*a + 1 ];
    int active = scalar > 0.;
    const char *name = pdi->GetArrayInformation( a )->GetName();
    char *listEntry = new char[strlen(name) + 20];
    if ( !active )
      sprintf( listEntry, "%s: inactive", name );
    else
      sprintf( listEntry, "%s: %g", name, scalars[ 2*a + 1 ] );

    d->ScalarFieldList->AppendUnique( listEntry );
    this->Script( "%s itemconfigure %d -foreground #%s", d->ScalarFieldList->GetListbox()->GetWidgetName(), a, active ? "006600" : "777744" );
    delete[] listEntry;
    }
}

void vtkPVTempTessellatorEntry::SetProperty( vtkPVWidgetProperty *prop )
{
  this->Data->Property = vtkPVScalarListWidgetProperty::SafeDownCast( prop );
}

const vtkPVWidgetProperty* vtkPVTempTessellatorEntry::GetProperty() const
{
  return this->Data->Property;
}

vtkPVWidgetProperty* vtkPVTempTessellatorEntry::GetProperty()
{
  return this->Data->Property;
}

vtkPVWidgetProperty* vtkPVTempTessellatorEntry::CreateAppropriateProperty()
{
  return vtkPVScalarListWidgetProperty::New();
}

void vtkPVTempTessellatorEntry::UpdateEnableState()
{
  vtkTessellatorEntryData* d = this->Data;

  this->Superclass::UpdateEnableState();

  this->PropagateEnableState( d->ScalarFieldList );
  this->PropagateEnableState( d->CriterionEnable );
  this->PropagateEnableState( d->CriterionValue );
}

vtkCxxSetObjectMacro(vtkPVTempTessellatorEntry,InputMenu,vtkPVInputMenu);

vtkPVDataSetAttributesInformation* vtkPVTempTessellatorEntry::GetPointDataInformation()
{
  if ( ! this->InputMenu )
    return 0;

  vtkPVSource* src = this->InputMenu->GetCurrentValue();
  if ( ! src )
    return 0;

  return src->GetDataInformation()->GetPointDataInformation();
}

void vtkPVTempTessellatorEntry::UpdateProperty()
{
  vtkTessellatorEntryData* d = this->Data;
  int numberOfArrays = d->Property->GetNumberOfScalars()/2;
  float* propScalars = new float[ 2*numberOfArrays ];
  for ( int a = 0; a < numberOfArrays; ++a )
    {
    const char* field = d->ScalarFieldList->GetItem(a);
    int flen = strlen(field);

    propScalars[2*a  ] = a;
    if ( strcmp( field + flen - 8 /*=strlen("inactive")+1*/, "inactive" ) != 0 )
      {
      int colon = flen;
      while ( colon && field[ colon ] != ':' )
        --colon;
      if ( ! colon )
        {
        vtkWarningMacro( "List item is screwy, couldn't find a colon" );
        continue;
        }

      propScalars[2*a+1] = atof( field + colon + 1 );
      }
    else
      {
      propScalars[2*a+1] = -1.0;
      }
    }
  d->Property->SetScalars( 2*numberOfArrays, propScalars );
  delete [] propScalars;
}

void vtkPVTempTessellatorEntry::CopyProperties( vtkPVWidget* clone, vtkPVSource* pvSource,
                                            vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map )
{
  this->Superclass::CopyProperties( clone, pvSource, map );

  vtkPVTempTessellatorEntry* dst = vtkPVTempTessellatorEntry::SafeDownCast( clone );
  if ( dst )
    {
    dst->SetSetFieldCriterionCommand( this->SetFieldCriterionCommand );
    dst->SetResetCriteriaCommand( this->ResetCriteriaCommand );
    if ( this->InputMenu )
      {
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype( pvSource, map );
      dst->SetInputMenu( im );
      im->Delete();
      }
    }
}

int vtkPVTempTessellatorEntry::ReadXMLAttributes( vtkPVXMLElement* element, vtkPVXMLPackageParser* parser )
{
  if ( !this->Superclass::ReadXMLAttributes(element, parser) )
    return 0;

  this->SetSetFieldCriterionCommand( element->GetAttribute("set_criterion_command") );
  this->SetResetCriteriaCommand( element->GetAttribute("reset_criteria_command") );

  const char* input_menu_id = element->GetAttribute("input_menu_id");
  if ( input_menu_id )
    {
    vtkPVXMLElement* im = element->LookupElement( input_menu_id );
    if (!im)
      {
      vtkErrorMacro("Couldn't find InputMenu element " << input_menu_id);
      return 0;
      }
    
    vtkPVWidget* w = this->GetPVWidgetFromParser( im, parser );
    vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast( w );
    if ( ! imw )
      {
      if ( w )
        w->Delete();
      vtkErrorMacro( "Menu with id \"" << input_menu_id << "\" could not be retrieved." );
      return 0;
      }
    imw->AddDependent( this ); // Whenever ModifiedCallback() or Update() is called on imw, we get an Update().
    this->SetInputMenu( imw );
    imw->Delete();
    }
  else
    {
    vtkErrorMacro( "TessellatorEntry requires that the input_menu_id attribute be set to\nthe name of a valid InputMenu widget." );
    return 0;
    }

  return 1;
}

void vtkPVTempTessellatorEntry::SaveInBatchScriptForPart( ofstream *file, vtkClientServerID id )
{
  vtkPVScalarListWidgetProperty* prop = this->Data->Property;
  int numScalars = prop->GetNumberOfScalars()/2;
  float* scalars = prop->GetScalars();

  (*file) << "pvTemp" << id << " ResetFieldCriteria" << endl;
  for ( int a=0; a<numScalars; ++a )
    (*file) << "pvTemp" << id << "   SetFieldCriterion " << scalars[2*a] << " " << scalars[2*a+1] << endl;
}

