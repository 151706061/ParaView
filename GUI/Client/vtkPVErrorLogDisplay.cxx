/*=========================================================================

  Program:   ParaView
  Module:    vtkPVErrorLogDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVErrorLogDisplay.h"

#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWText.h"
#include "vtkKWTextWithScrollbars.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkTimerLog.h"
#include "vtkVector.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVErrorLogDisplay );
vtkCxxRevisionMacro(vtkPVErrorLogDisplay, "1.11");

int vtkPVErrorLogDisplayCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVErrorLogDisplay::vtkPVErrorLogDisplay()
{
  this->ErrorMessages = 0;
}

//----------------------------------------------------------------------------
vtkPVErrorLogDisplay::~vtkPVErrorLogDisplay()
{
  if ( this->ErrorMessages )
    {
    this->ErrorMessages->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::AppendError(const char* msg)
{
  if ( !this->ErrorMessages )
    {
    this->ErrorMessages = vtkVector<const char*>::New();
    }
  this->ErrorMessages->AppendItem(msg);
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::Clear()
{
  if ( this->ErrorMessages )
    {
    this->ErrorMessages->RemoveAllItems();
    }

  vtkPVApplication *app = 
    vtkPVApplication::SafeDownCast(this->GetApplication());
  if (app)
    {
    vtkKWWindow *window = app->GetMainWindow();
    if (window)
      {
      window->SetErrorIconToNone();
      }
    }
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVErrorLogDisplay already created");
    return;
    }

  this->Superclass::Create(app);

  this->Script("pack forget  %s %s %s %s",
               this->ThresholdLabel->GetWidgetName(),
               this->ThresholdMenu->GetWidgetName(),
               this->EnableLabel->GetWidgetName(),
               this->EnableCheck->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::Update()
{
  int cc;
  this->DisplayText->GetWidget()->SetValue("");
  if ( this->ErrorMessages )
    {
    for ( cc = 0; cc < this->ErrorMessages->GetNumberOfItems(); cc ++ )
      {
      const char* item = 0;
      if ( this->ErrorMessages->GetItem(cc, item) == VTK_OK && item )
        {
        this->Append(item);
        }
      }
    }
  else
    {
    this->DisplayText->GetWidget()->SetValue("");
    this->Append("No errors");
    }
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::Save(const char *fileName)
{
  ofstream *fptr;
 
  fptr = new ofstream(fileName);

  if (fptr->fail())
    {
    vtkErrorMacro(<< "Could not open" << fileName);
    delete fptr;
    return;
    }

  int cc;
  if ( this->ErrorMessages )
    {
    for ( cc = 0; cc < this->ErrorMessages->GetNumberOfItems(); cc ++ )
      {
      const char* item = 0;
      if ( this->ErrorMessages->GetItem(cc, item) == VTK_OK && item )
        {
        *fptr << item << endl;
        }
      }
    }
  else
    {
    *fptr << "No errors" << endl;
    }
  fptr->close();
  delete fptr;
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << endl;
  os << indent << "Threshold: " << this->Threshold << endl;
}
