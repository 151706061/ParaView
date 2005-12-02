/*=========================================================================

  Module:    vtkKWScaleWithEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWScaleWithEntry.h"

#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWTopLevel.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/string>

//---------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWScaleWithEntry );
vtkCxxRevisionMacro(vtkKWScaleWithEntry, "1.5");

/* 
 * This part was generated by ImageConvert from image:
 *    arrow.png (zlib, base64)
 */
#define image_arrow_width         4
#define image_arrow_height        7
#define image_arrow_pixel_size    4
#define image_arrow_length 40

static unsigned char image_arrow[] = 
  "eNpjYGD4z4AK/jOgiv1HE/uPB+PSDwcAlQUP8Q==";

//---------------------------------------------------------------------------
vtkKWScaleWithEntry::vtkKWScaleWithEntry()
{
  this->EntryCommand    = NULL;
  this->EntryVisibility = 1;
  this->Entry           = NULL;
  this->EntryPosition   = vtkKWScaleWithEntry::EntryPositionDefault;
  this->ExpandEntry     = 0;

  this->TopLevel        = NULL;
  this->PopupPushButton = NULL;
  this->PopupMode      = 0;

  this->RangeMinLabel   = NULL;
  this->RangeMaxLabel   = NULL;
  this->RangeVisibility = 0;
}

//---------------------------------------------------------------------------
vtkKWScaleWithEntry::~vtkKWScaleWithEntry()
{
  if (this->IsAlive())
    {
    this->UnBind();
    }

  if (this->EntryCommand)
    {
    delete [] this->EntryCommand;
    this->EntryCommand = NULL;
    }

  if (this->Entry)
    {
    this->Entry->Delete();
    this->Entry = NULL;
    }

  if (this->RangeMinLabel)
    {
    this->RangeMinLabel->Delete();
    this->RangeMinLabel = NULL;
    }
  
  if (this->RangeMaxLabel)
    {
    this->RangeMaxLabel->Delete();
    this->RangeMaxLabel = NULL;
    }
  
  if (this->TopLevel)
    {
    this->TopLevel->Delete();
    this->TopLevel = NULL;
    }

  if (this->PopupPushButton)
    {
    this->PopupPushButton->Delete();
    this->PopupPushButton = NULL;
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // If we need the scale to popup, create the top level window accordingly
  // and its push button. We need to set the parent of the scale right now
  // before it is created by the superclass as a child of the composite frame

  if (this->PopupMode)
    {
    this->TopLevel = vtkKWTopLevel::New();
    this->TopLevel->SetApplication(this->GetApplication());
    this->TopLevel->Create();
    this->TopLevel->SetBackgroundColor(0.0, 0.0, 0.0);
    this->TopLevel->SetBorderWidth(2);
    this->TopLevel->SetReliefToFlat();
    this->TopLevel->HideDecorationOn();
    this->TopLevel->Withdraw();
    this->TopLevel->SetMasterWindow(this);

    if (this->GetScale())
      {
      this->GetScale()->SetParent(this->TopLevel);
      }
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  // Popup

  if (this->PopupMode)
    {
    this->PopupPushButton = vtkKWPushButton::New();
    this->PopupPushButton->SetParent(this);
    this->PopupPushButton->Create();
    this->PopupPushButton->SetPadX(0);
    this->PopupPushButton->SetPadY(0);
    
    this->PopupPushButton->SetImageToPixels(
      image_arrow, 
      image_arrow_width, image_arrow_height, image_arrow_pixel_size, 
      image_arrow_length);
    }

  // Create the entry subwidget now if it has to be shown now

  if (this->EntryVisibility)
    {
    this->CreateEntry();
    }

  // Pack and bind

  this->Pack();
  this->Bind();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetLabelText(const char *text)
{
  this->Superclass::SetLabelText(text);

  // Call pack to pack/unpack the label if its content is an empty string

  this->Pack();
}

//----------------------------------------------------------------------------
vtkKWEntry* vtkKWScaleWithEntry::GetEntry()
{
  // Lazy evaluation. Create the entry only when it is needed

  if (!this->Entry)
    {
    this->Entry = vtkKWEntry::New();
    this->PropagateEnableState(this->Entry);
    }

  return this->Entry;
}

//----------------------------------------------------------------------------
int vtkKWScaleWithEntry::HasEntry()
{
  return this->Entry ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::CreateEntry()
{
  // Create the entry. If the parent has been set before (i.e. by the subclass)
  // do not set it.
  // Note that GetEntry() will allocate the entry on the fly
  
  if (this->HasEntry() && this->GetEntry()->IsCreated())
    {
    return;
    }

  vtkKWEntry *entry = this->GetEntry();
  if (!entry->GetParent())
    {
    entry->SetParent(this);
    }

  entry->Create();
  entry->SetBalloonHelpString(this->GetBalloonHelpString());
  entry->SetWidth(11);
  entry->SetValueAsDouble(this->GetValue());

  // Since we have just created the entry on the fly, it is likely that 
  // it needs to be displayed somehow, which is usually Pack()'s job

  this->Pack();
  this->Bind();
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetEntryPosition(int arg)
{
  if (arg < vtkKWScaleWithEntry::EntryPositionDefault)
    {
    arg = vtkKWScaleWithEntry::EntryPositionDefault;
    }
  else if (arg > vtkKWScaleWithEntry::EntryPositionRight)
    {
    arg = vtkKWScaleWithEntry::EntryPositionRight;
    }

  if (this->EntryPosition == arg)
    {
    return;
    }

  this->EntryPosition = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetLabelAndEntryPositionToTop()
{
  this->SetLabelPositionToTop();
  this->SetEntryPositionToTop();
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetEntryWidth(int width)
{
  this->GetEntry()->SetWidth(width);
}

//----------------------------------------------------------------------------
int vtkKWScaleWithEntry::GetEntryWidth()
{
  return this->GetEntry()->GetWidth();
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetEntryVisibility(int _arg)
{
  if (this->EntryVisibility == _arg)
    {
    return;
    }
  this->EntryVisibility = _arg;
  this->Modified();

  // Make sure that if the entry has to be show, we create it on the fly if
  // needed

  if (this->EntryVisibility && this->IsCreated())
    {
    this->CreateEntry();
    }

  this->Pack();
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetExpandEntry(int arg)
{
  if (this->ExpandEntry == arg)
    {
    return;
    }

  this->ExpandEntry = arg;
  this->Modified();

  this->Pack();
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  if (this->Widget)
    {
    this->Widget->UnpackSiblings();
    }

  this->UnpackChildren();

  // Repack everything

  ostrstream tk_cmd;

  int i;
  int is_horiz = 1;

  if (this->GetScale() && 
      this->GetScale()->IsCreated() &&
      this->GetScale()->GetOrientation() == 
      vtkKWTkOptions::OrientationVertical)
    {
    is_horiz = 0;
    }

  int row, col, row_span, col_span, c_padx = 0, c_pady = 0;
  const char *anchor, *sticky;

  /*
           0 1 2  3         4 5  6 7       0 1 2
         +--------------------------    +---------
        0|        L         E           0|   L 
        1| L E R0 [---------] R1 L E    1|   E
        2|        L         E           2|   R0
                                        3| L ^ L
                                         |   | 
                                         |   | 
                                         |   |
                                        4| E v E
                                        5|   R1
                                        6|   L
                                        7|   E
  */

  // Label
  // For convenience purposes, an empty label is not displayed

  if (this->LabelVisibility && 
      this->HasLabel() && 
      this->GetLabel()->IsCreated() &&
      this->GetLabel()->GetText() &&
      *this->GetLabel()->GetText())
    {
    if (this->PopupMode)
      {
      col = 0; row = 1; sticky = "nsw"; anchor = "w";
      }
    else
      {
      if (is_horiz)
        {
        switch (this->LabelPosition)
          {
          case vtkKWWidgetWithLabel::LabelPositionLeft:
          case vtkKWWidgetWithLabel::LabelPositionDefault:
          default:
            col = 0; row = 1; sticky = "nsw"; anchor = "w";
            break;
          case vtkKWWidgetWithLabel::LabelPositionRight:
            col = 6; row = 1; sticky = "nsw"; anchor = "w";
            break;
          case vtkKWWidgetWithLabel::LabelPositionBottom:
            col = 3; row = 2; sticky = "w"; anchor = "w";
            break;
          case vtkKWWidgetWithLabel::LabelPositionTop:
            col = 3; row = 0; sticky = "w"; anchor = "w";
            break;
          }
        }
      else 
        {
        switch (this->LabelPosition)
          {
          case vtkKWWidgetWithLabel::LabelPositionLeft:
            col = 0; row = 3; sticky = "nw"; anchor = "w";
            break;
          case vtkKWWidgetWithLabel::LabelPositionRight:
          case vtkKWWidgetWithLabel::LabelPositionDefault:
          default:
            col = 2; row = 3; sticky = "nw"; anchor = "w";
            break;
          case vtkKWWidgetWithLabel::LabelPositionBottom:
            col = 1; row = 6; sticky = "w"; anchor = "w";
            break;
          case vtkKWWidgetWithLabel::LabelPositionTop:
            col = 1; row = 0; sticky = "w"; anchor = "w";
            break;
          }
        }
      }
    tk_cmd << "grid " << this->GetLabel()->GetWidgetName() 
           << " -row " << row << " -column " << col 
           << " -sticky " << sticky << endl;
    this->GetLabel()->SetConfigurationOption("-anchor", anchor);
    }

  // Reset resizing

  for (i = 0; i <= 7; i++)
    {
    tk_cmd << "grid " << (is_horiz ? "columnconfigure " : "rowconfigure ")
           << this->GetWidgetName() << " " << i << " -weight 0" << endl;
    }

  // Entry

  if (this->EntryVisibility && this->Entry && this->Entry->IsCreated())
    {
    if (this->PopupMode)
      {
      col = 1; row = 1; c_padx = 1;
      if (this->ExpandEntry)
        {
        sticky = "news";
        tk_cmd << "grid columnconfigure "
               << " " << this->GetEntry()->GetParent()->GetWidgetName() 
               << " " << col << " -weight 1" << endl;
        }
      else
        {
        sticky = "nsw";
        }
      }
    else
      {
      if (is_horiz)
        {
        switch (this->EntryPosition)
          {
          case vtkKWScaleWithEntry::EntryPositionLeft:
            col = 1; row = 1; sticky = "nsw"; c_padx = 1;
            break;
          case vtkKWScaleWithEntry::EntryPositionRight:
          case vtkKWScaleWithEntry::EntryPositionDefault:
          default:
            col = 7; row = 1; sticky = "nsw"; c_padx = 1;
            break;
          case vtkKWScaleWithEntry::EntryPositionBottom:
            col = 4; row = 2; sticky = "e"; c_pady = 1;
            break;
          case vtkKWScaleWithEntry::EntryPositionTop:
            col = 4; row = 0; sticky = "e"; c_pady = 1;
            break;
          }
        }
      else 
        {
        switch (this->EntryPosition)
          {
          case vtkKWScaleWithEntry::EntryPositionLeft:
            col = 0; row = 4; sticky = "nw"; c_padx = 1;
            break;
          case vtkKWScaleWithEntry::EntryPositionRight:
          case vtkKWScaleWithEntry::EntryPositionDefault:
          default:
            col = 2; row = 4; sticky = "nw"; c_padx = 1;
            break;
          case vtkKWScaleWithEntry::EntryPositionBottom:
            col = 1; row = 7; sticky = "w"; c_pady = 1;
            break;
          case vtkKWScaleWithEntry::EntryPositionTop:
            col = 1; row = 1; sticky = "w"; c_pady = 1;
            break;
          }
        }
      }
    tk_cmd << "grid " << this->Entry->GetWidgetName()
           << " -row " << row << " -column " << col 
           << " -sticky " << sticky << endl;
    }

  // Scale

  if (this->GetScale() && this->GetScale()->IsCreated())
    {
    if (is_horiz || this->PopupMode) 
      { 
      col = 3; row = 1; col_span = 2; row_span = 1; sticky = "ew";
      }
    else 
      { 
      col = 1; row = 3; col_span = 1; row_span = 2; sticky = "ns";
      }
    tk_cmd << "grid " 
           << (this->PopupMode && 
               this->PopupPushButton && this->PopupPushButton->IsCreated() ?
               this->PopupPushButton->GetWidgetName() :
               this->GetScale()->GetWidgetName())
           << " -row " << row << " -column " << col 
           << " -rowspan " << row_span << " -columnspan " << col_span
           << " -sticky " << (this->PopupMode ? "ns" : sticky)
      //           << " -padx " << c_padx * 2 << " -pady " << c_pady * 2
           << " -ipadx " << (this->PopupMode ? 1 : 0)
           << endl;

    if (this->PopupMode)
      {
      tk_cmd << "pack " << this->GetScale()->GetWidgetName()
             << " -side " << (is_horiz ? "left" : "top") 
             << " -expand y -fill both -pady 0 -padx 0" << endl;
      }
    else
      {
      // Make sure it will resize properly
      
      for (i = 3; i < 3 + (is_horiz ? col_span : row_span); i++)
        {
        tk_cmd << "grid " << (is_horiz ? "columnconfigure " : "rowconfigure ")
               << this->GetScale()->GetParent()->GetWidgetName() 
               << " " << i << " -weight 1" << endl;
        }
      }
 
    // Range

    if (this->RangeVisibility)
      {
      if (this->RangeMinLabel && this->RangeMinLabel->IsCreated())
        {
        if (this->PopupMode)
          {
          tk_cmd << "pack " << this->RangeMinLabel->GetWidgetName()
                 << " -expand n -fill both -pady 0 -padx 0 "
                 << " -side " << (is_horiz ? "left" : "top") 
                 << " -before " << this->GetScale()->GetWidgetName() << endl;
          }
        else
          {
          tk_cmd << "grid " << this->RangeMinLabel->GetWidgetName()
                 << " -row " << (is_horiz ? 1 : 2) 
                 << " -column " << (is_horiz ? 2 : 1) 
                 << " -sticky " << (is_horiz ? "nse" : "new")  << endl;
          }
        }
      if (this->RangeMaxLabel && this->RangeMaxLabel->IsCreated())
        {
        if (this->PopupMode)
          {
          tk_cmd << "pack " << this->RangeMaxLabel->GetWidgetName()
                 << " -expand n -fill both -pady 0 -padx 0 " 
                 << " -side " << (is_horiz ? "right" : "bottom") 
                 << " -after " << this->GetScale()->GetWidgetName() << endl;
          }
        else
          {
          tk_cmd << "grid " << this->RangeMaxLabel->GetWidgetName()
                 << " -row " << (is_horiz ? 1 : 5) 
                 << " -column " << (is_horiz ? 5 : 1) 
                 << " -sticky " << (is_horiz ? "nsw" : "new")  << endl;
          }
        }
      }
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::Bind()
{
  if (this->GetScale())
    {
      this->GetScale()->Bind();
      
    // If in popup mode and the mouse is leaving the top level window, 
    // then withdraw it, unless the user is interacting with the scale.

    if (this->PopupMode &&
        this->TopLevel && this->TopLevel->IsCreated())
      {
      this->TopLevel->SetBinding(
        "<Leave>", this, "WithdrawPopupModeCallback");

      vtksys_stl::string callback;

      this->GetScale()->AddBinding(
        "<ButtonPress>", this->TopLevel, "RemoveBinding <Leave>");

      callback = "SetBinding ";
      callback += " <Leave> ";
      callback += this->GetTclName();
      callback += " WithdrawPopupModeCallback";

      this->GetScale()->AddBinding(
        "<ButtonRelease>", this->TopLevel, callback.c_str());
      }
    
    char *command = NULL;
    this->SetObjectMethodCommand(&command, this, "ScaleValueCallback");
    this->GetScale()->SetConfigurationOption("-command", command);
    delete [] command;
    }

  if (this->Entry && this->Entry->IsCreated())
    {
    this->Entry->SetBinding("<Return>", this, "EntryValueCallback");
    this->Entry->SetBinding("<FocusOut>", this, "EntryValueCallback");
    }

  if (this->PopupMode && 
      this->PopupPushButton && this->PopupPushButton->IsCreated())
    {
    this->PopupPushButton->SetBinding(
      "<ButtonPress>", this, "DisplayPopupModeCallback");
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::UnBind()
{
  if (this->GetScale())
    {
    this->GetScale()->UnBind();
    }

  if (this->Entry && this->Entry->IsCreated())
    {
    this->Entry->RemoveBinding("<Return>");
    this->Entry->RemoveBinding("<FocusOut>");
    }
  
  if (this->PopupMode && 
      this->PopupPushButton && this->PopupPushButton->IsCreated())
    {
    this->PopupPushButton->RemoveBinding("<ButtonPress>");
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::DisplayPopupModeCallback()
{
  if (!this->PopupMode ||
      !this->PopupPushButton || !this->PopupPushButton->IsCreated() ||
      !this->TopLevel || !this->TopLevel->IsCreated() ||
      !this->GetScale() || !this->GetScale()->IsCreated())
    {
    return;
    }

  // Get the position of the mouse, the position and size of the push button,
  // the size of the scale.

  int x, y, py, ph, scx, scy, sx, sy;

  vtkKWTkUtilities::GetMousePointerCoordinates(this, &x, &y);
  vtkKWTkUtilities::GetWidgetCoordinates(this->PopupPushButton, NULL, &py);
  vtkKWTkUtilities::GetWidgetSize(this->PopupPushButton, NULL, &ph);
  vtkKWTkUtilities::GetWidgetRelativeCoordinates(this->GetScale(), &sx, &sy);

  sscanf(this->Script(
           "%s coords %g", this->GetScale()->GetWidgetName(),this->GetValue()),
         "%d %d", &scx, &scy);
 
  // Place the scale so that the slider is coincident with the x mouse position
  // and just below the push button
  
  x -= sx + scx;

  if (py <= y && y <= (py + ph -1))
    {
    y = py + ph - 3;
    }
  else
    {
    y -= sy + scy;
    }

  this->TopLevel->SetPosition(x, y);
  vtkKWTkUtilities::ProcessPendingEvents(this->GetApplication());
  this->TopLevel->DeIconify();
  this->TopLevel->Raise();

  this->UpdateValue();
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::WithdrawPopupModeCallback()
{
  if (!this->TopLevel)
    {
    return;
    }
  
  // Withdraw the popup
  
  this->TopLevel->Withdraw();
  
  this->UpdateValue();
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::ScaleValueCallback(double num)
{
  if (this->GetScale() && this->GetScale()->GetDisableScaleValueCallback())
    {
    return;
    }

  this->SetValue(num);
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetValue(double num)
{
  // Update entry first since the scale is the one that will trigger
  // a user-callback, and he may try to retrieve the value using the
  // vtkKWEntry API, not our API

  if (this->GetValue() != num)
    {
    this->SetEntryValue(num);
    }

  if (this->GetScale())
    {
    this->GetScale()->SetValue(num);
    }
}

//---------------------------------------------------------------------------
double vtkKWScaleWithEntry::GetValue()
{
  if (this->GetScale())
    {
    return this->GetScale()->GetValue();
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::UpdateValue()
{
  this->SetEntryValue(this->GetValue());

  if (this->GetScale())
    {
    this->GetScale()->UpdateValue();
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetEntryValue(double num)
{
  if (this->Entry && this->Entry->IsCreated())
    {
    this->Entry->SetValueAsDouble(num);
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetResolution(double r)
{
  if (this->GetScale())
    {
    this->GetScale()->SetResolution(r);
    }
}

//---------------------------------------------------------------------------
double vtkKWScaleWithEntry::GetResolution()
{
  if (this->GetScale())
    {
    return this->GetScale()->GetResolution();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetOrientation(int orientation)
{
  int old_orient = this->GetOrientation();
  if (this->GetScale())
    {
    this->GetScale()->SetOrientation(orientation);
    }
  if (old_orient != this->GetOrientation())
    {
    this->Pack();
    }
}

//----------------------------------------------------------------------------
int vtkKWScaleWithEntry::GetOrientation()
{
  if (this->GetScale())
    {
    return this->GetScale()->GetOrientation();
    }
  return vtkKWTkOptions::OrientationUnknown;
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetLength(int length)
{
  if (this->GetScale())
    {
    this->GetScale()->SetLength(length);
    }
}

//----------------------------------------------------------------------------
int vtkKWScaleWithEntry::GetLength()
{
  if (this->GetScale())
    {
    return this->GetScale()->GetLength();
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetRange(double min, double max)
{
  double old_min, old_max;
  this->GetRange(old_min, old_max);

  if (this->GetScale())
    {
    this->GetScale()->SetRange(min, max);
    }

  this->GetRange(min, max);

  if (old_min != min || old_max != max)
    {
    this->UpdateRange();
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::UpdateRange()
{
  char label_text[100];
  
  if (this->RangeMinLabel && this->RangeMinLabel->IsCreated())
    {
    sprintf(label_text, "(%g)", this->GetRangeMin());
    this->RangeMinLabel->SetText(label_text);
    }

  if (this->RangeMaxLabel && this->RangeMaxLabel->IsCreated())
    {
    sprintf(label_text, "(%g)", this->GetRangeMax());
    this->RangeMaxLabel->SetText(label_text);
    }
}

//---------------------------------------------------------------------------
double* vtkKWScaleWithEntry::GetRange()
{
  if (this->GetScale())
    {
    return this->GetScale()->GetRange();
    }
  return NULL;
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::GetRange(double &min, double &max)
{
  if (this->GetScale())
    {
    this->GetScale()->GetRange(min, max);
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetRangeVisibility(int flag)
{
  if (this->RangeVisibility == flag)
    {
    return;
    }
  
  this->RangeVisibility = flag;

  if (!this->RangeMinLabel)
    {
    this->RangeMinLabel = vtkKWLabel::New();
    if (this->PopupMode)
      {
      this->RangeMinLabel->SetParent(this->TopLevel);
      }
    else
      {
      this->RangeMinLabel->SetParent(this);
      }
    this->PropagateEnableState(this->RangeMinLabel);
    }
  if (!this->RangeMinLabel->IsCreated())
    {
    this->RangeMinLabel->Create();
    }

  if (!this->RangeMaxLabel)
    {
    this->RangeMaxLabel = vtkKWLabel::New();
    if (this->PopupMode)
      {
      this->RangeMaxLabel->SetParent(this->TopLevel);
      }
    else
      {
      this->RangeMaxLabel->SetParent(this);
      }
    this->PropagateEnableState(this->RangeMaxLabel);
    }
  if (!this->RangeMaxLabel->IsCreated())
    {
    this->RangeMaxLabel->Create();
    }

  this->UpdateRange();
  
  this->Modified();
  this->Pack();
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::EntryValueCallback()
{
  if (this->Entry)
    {
    double value = this->Entry->GetValueAsDouble();
    double old_value = this->GetValue();
    this->SetValue(value);

    if (value != old_value)
      {
      this->InvokeEntryCommand();
      }
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::InvokeEntryCommand()
{
  if (this->EntryCommand && *this->EntryCommand && !this->GetDisableCommands()
      && this->IsCreated())
    {
    this->Script("eval %s", this->EntryCommand);
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetEntryCommand(vtkObject *object, const char * method)
{
  this->SetObjectMethodCommand(
    &this->EntryCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetCommand(vtkObject *object, const char *method)
{
  if (this->GetScale())
    {
    this->GetScale()->SetCommand(object, method);
    }
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetStartCommand(vtkObject *object, const char * method)
{
  if (this->GetScale())
    {
    this->GetScale()->SetStartCommand(object, method);
    }
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetEndCommand(vtkObject *object, const char * method)
{
  if (this->GetScale())
    {
    this->GetScale()->SetEndCommand(object, method);
    }
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetDisableCommands(int val)
{
  if (this->GetScale())
    {
    this->GetScale()->SetDisableCommands(val);
    }
}

//----------------------------------------------------------------------------
int vtkKWScaleWithEntry::GetDisableCommands()
{
  if (this->GetScale())
    {
    return this->GetScale()->GetDisableCommands();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetClampValue(int val)
{
  if (this->GetScale())
    {
    this->GetScale()->SetClampValue(val);
    }
}

//----------------------------------------------------------------------------
int vtkKWScaleWithEntry::GetClampValue()
{
  if (this->GetScale())
    {
    return this->GetScale()->GetClampValue();
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  // Do not use GetEntry() here, otherwise the entry will be created 
  // on the fly, and we do not want this. Once the entry gets created when
  // there is a real need for it, its Enabled state will be set correctly
  // anyway.

  if (this->Entry)
    {
    this->Entry->SetBalloonHelpString(string);
    }

  if (this->PopupMode && this->PopupPushButton)
    {
    vtksys_stl::string temp(string);
    temp  += " (press this button to display the scale)";
    this->PopupPushButton->SetBalloonHelpString(temp.c_str());
    }
}

//---------------------------------------------------------------------------
void vtkKWScaleWithEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Do not use GetEntry() here, otherwise the entry will be created 
  // on the fly, and we do not want this. Once the entry gets created when
  // there is a real need for it, its Enabled state will be set correctly
  // anyway.

  this->PropagateEnableState(this->Entry);

  this->PropagateEnableState(this->PopupPushButton);
}

//----------------------------------------------------------------------------
void vtkKWScaleWithEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "EntryVisibility: " 
     << (this->EntryVisibility ? "On" : "Off") << endl;

  os << indent << "EntryPosition: " << this->EntryPosition << endl;

  os << indent << "Entry: ";
  if (this->Entry)
    {
    os << endl;
    this->Entry->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "PopupPushButton: " << this->PopupPushButton << endl;
  os << indent << "PopupMode: " 
     << (this->PopupMode ? "On" : "Off") << endl;
  os << indent << "ExpandEntry: " 
     << (this->ExpandEntry ? "On" : "Off") << endl;
  os << indent << "RangeVisibility: "
     << (this->RangeVisibility ? "On" : "Off") << endl;
}

