/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSaveBatchScriptDialog.cxx
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
#include "vtkPVSaveBatchScriptDialog.h"

#include "vtkKWApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkKWPushButton.h"
#include "vtkKWText.h"
#include "vtkKWWindow.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkString.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVSaveBatchScriptDialog );
vtkCxxRevisionMacro(vtkPVSaveBatchScriptDialog, "1.5.2.1");

int vtkPVSaveBatchScriptDialogCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSaveBatchScriptDialog::vtkPVSaveBatchScriptDialog()
{
  this->CommandFunction = vtkPVSaveBatchScriptDialogCommand;
  
  this->FilePath = NULL;
  this->FileRoot = NULL;

  this->ButtonFrame = vtkKWWidget::New();
  this->AcceptButton = vtkKWPushButton::New();
  this->CancelButton = vtkKWPushButton::New();
  
  this->OffScreenCheck = vtkKWCheckButton::New();

  this->SaveImagesCheck = vtkKWCheckButton::New();
  this->ImageFileNameFrame = vtkKWWidget::New();
  this->ImageFileNameEntry = vtkKWEntry::New();
  this->ImageFileNameBrowseButton = vtkKWPushButton::New();

  this->SaveGeometryCheck = vtkKWCheckButton::New();
  this->GeometryFileNameFrame = vtkKWWidget::New();
  this->GeometryFileNameEntry = vtkKWEntry::New();
  this->GeometryFileNameBrowseButton = vtkKWPushButton::New();

  this->Title = NULL;
  this->SetTitle("Batch File Elements");
  
  this->MasterWindow = 0;

  this->Exit = 1;
  this->AcceptedFlag = 1;
}

//----------------------------------------------------------------------------
vtkPVSaveBatchScriptDialog::~vtkPVSaveBatchScriptDialog()
{
  this->SetFilePath(NULL);
  this->SetFileRoot(NULL);

  this->OffScreenCheck->Delete();
  this->OffScreenCheck = NULL;

  this->SaveImagesCheck->Delete();
  this->SaveImagesCheck = NULL;
  this->ImageFileNameFrame->Delete();
  this->ImageFileNameFrame = NULL;
  this->ImageFileNameEntry->Delete();
  this->ImageFileNameEntry = NULL;
  this->ImageFileNameBrowseButton->Delete();
  this->ImageFileNameBrowseButton= NULL;

  this->SaveGeometryCheck->Delete();
  this->SaveGeometryCheck = NULL;
  this->GeometryFileNameFrame->Delete();
  this->GeometryFileNameFrame = NULL;
  this->GeometryFileNameEntry->Delete();
  this->GeometryFileNameEntry = NULL;
  this->GeometryFileNameBrowseButton->Delete();
  this->GeometryFileNameBrowseButton = NULL;

  this->AcceptButton->Delete();
  this->AcceptButton = NULL;
  this->CancelButton->Delete();
  this->CancelButton = NULL;
  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
  
  this->SetTitle(NULL);
  this->SetMasterWindow(0);
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::SetMasterWindow(vtkKWWindow* win)
{
  if (this->MasterWindow != win) 
    { 
    if (this->MasterWindow) 
      { 
      this->MasterWindow->UnRegister(this); 
      }
    this->MasterWindow = win; 
    if (this->MasterWindow) 
      { 
      this->MasterWindow->Register(this); 
      if (this->Application)
        {
        this->Script("wm transient %s %s", this->GetWidgetName(), 
                     this->MasterWindow->GetWidgetName());
        }
      } 
    this->Modified(); 
    } 
  
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::Create(vtkKWApplication *app)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Interactor already created");
    return;
    }
  
  this->SetApplication(app);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("toplevel %s", wname);
  this->Script("wm title %s \"%s\"", wname, this->Title);
  this->Script("wm iconname %s \"vtk\"", wname);
  this->Script("wm geometry %s +%d+%d", this->GetWidgetName(),
               400, 250);


  if (this->MasterWindow)
    {
    this->Script("wm transient %s %s", wname, 
                 this->MasterWindow->GetWidgetName());
    }
  else
    {
    int sw, sh;
    this->Script("concat [winfo screenwidth %s] [winfo screenheight %s]",
                 this->GetWidgetName(), this->GetWidgetName());
    sscanf(app->GetMainInterp()->result, "%d %d", &sw, &sh);

    int ww, wh;
    this->Script("concat [winfo reqwidth %s] [winfo reqheight %s]",
                 this->GetWidgetName(), this->GetWidgetName());
    sscanf(app->GetMainInterp()->result, "%d %d", &ww, &wh);
    this->Script("wm geometry %s +%d+%d", this->GetWidgetName(), 
                 (sw-ww)/2, (sh-wh)/2);
    }

  this->OffScreenCheck->SetParent(this);
  this->OffScreenCheck->Create(app, 0);
  this->OffScreenCheck->SetText("Offscreen");

  this->SaveImagesCheck->SetParent(this);
  this->SaveImagesCheck->Create(app, 0);
  this->SaveImagesCheck->SetState(1);
  this->SaveImagesCheck->SetText("Save Images");
  this->SaveImagesCheck->SetCommand(this, "SaveImagesCheckCallback");
  this->ImageFileNameFrame->SetParent(this);
  this->ImageFileNameFrame->Create(app, "frame", 0);

  this->SaveGeometryCheck->SetParent(this);
  this->SaveGeometryCheck->Create(app, 0);
  this->SaveGeometryCheck->SetState(0);
  this->SaveGeometryCheck->SetText("Save Geometry");
  this->SaveGeometryCheck->SetCommand(this, "SaveGeometryCheckCallback");
  this->GeometryFileNameFrame->SetParent(this);
  this->GeometryFileNameFrame->Create(app, "frame", 0);

  this->Script("pack %s %s -side top -padx 2 -anchor w",
               this->OffScreenCheck->GetWidgetName(),
               this->SaveImagesCheck->GetWidgetName());
  this->Script("pack %s -side top -expand 1 -fill x -padx 2",
               this->ImageFileNameFrame->GetWidgetName());
  this->Script("pack %s -side top -expand 0 -padx 2 -anchor w",
               this->SaveGeometryCheck->GetWidgetName());
  this->Script("pack %s -side top -expand 1 -fill x -padx 2",
               this->GeometryFileNameFrame->GetWidgetName());

  char* fileName = NULL;
  if (this->FilePath && this->FileRoot)
    {
    fileName = new char[strlen(this->FilePath)+strlen(this->FileRoot)+64];
    }
   
  this->ImageFileNameEntry->SetParent(this->ImageFileNameFrame);
  this->ImageFileNameEntry->Create(app, 0);
  if (fileName)
    {
    sprintf(fileName, "%s/%s.jpg", this->FilePath, this->FileRoot);
    this->ImageFileNameEntry->SetValue(fileName);
    }
  this->ImageFileNameBrowseButton->SetParent(this->ImageFileNameFrame);
  this->ImageFileNameBrowseButton->Create(app, 0);
  this->ImageFileNameBrowseButton->SetLabel("Browse");
  this->ImageFileNameBrowseButton->SetCommand(this, "ImageFileNameBrowseButtonCallback");
  this->Script("pack %s -side right -expand 0 -padx 2",
               this->ImageFileNameBrowseButton->GetWidgetName());
  this->Script("pack %s -side right -expand 1 -fill x -padx 2",
               this->ImageFileNameEntry->GetWidgetName());


  this->GeometryFileNameEntry->SetParent(this->GeometryFileNameFrame);
  this->GeometryFileNameEntry->Create(app, 0);
  if (fileName)
    {
    sprintf(fileName, "%s/%s.vtp", this->FilePath, this->FileRoot);
    this->GeometryFileNameEntry->SetValue(fileName);
    }
  this->GeometryFileNameBrowseButton->SetParent(this->GeometryFileNameFrame);
  this->GeometryFileNameBrowseButton->Create(app, 0);
  this->GeometryFileNameBrowseButton->SetLabel("Browse");
  this->GeometryFileNameBrowseButton->SetCommand(this, "GeometryFileNameBrowseButtonCallback");

  this->GeometryFileNameEntry->SetEnabled(0);
  this->GeometryFileNameBrowseButton->SetEnabled(0);

  this->Script("pack %s -side right -expand 0 -padx 2",
               this->GeometryFileNameBrowseButton->GetWidgetName());
  this->Script("pack %s -side right -expand 1 -fill x -padx 2",
               this->GeometryFileNameEntry->GetWidgetName());



  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(app, "frame", "");
  this->Script("pack %s -side bottom -fill both -expand 0 -pady 2m",
               this->ButtonFrame->GetWidgetName());
  this->AcceptButton->SetParent(this->ButtonFrame);
  this->AcceptButton->Create(app, "");
  this->AcceptButton->SetCommand(this, "Accept");
  this->AcceptButton->SetLabel("Accept");
  this->CancelButton->SetParent(this->ButtonFrame);
  this->CancelButton->Create(app, "");
  this->CancelButton->SetCommand(this, "Cancel");
  this->CancelButton->SetLabel("Cancel");
  this->Script("pack %s %s -side left -expand 1 -fill x -padx 2",
               this->AcceptButton->GetWidgetName(),
               this->CancelButton->GetWidgetName());

  this->Script("wm protocol %s WM_DELETE_WINDOW {wm withdraw %s}",
               wname, wname);

  this->Script("wm withdraw %s", wname);

  this->Script("wm protocol %s WM_DELETE_WINDOW { %s Cancel}",
               this->GetWidgetName(), this->GetTclName());
}

//----------------------------------------------------------------------------
int vtkPVSaveBatchScriptDialog::GetOffScreen()
{
  return this->OffScreenCheck->GetState();
}

//----------------------------------------------------------------------------
const char* vtkPVSaveBatchScriptDialog::GetImagesFileName()
{
  if ( ! this->SaveImagesCheck->GetState())
    {
    return NULL;
    }

  return this->ImageFileNameEntry->GetValue();
}

//----------------------------------------------------------------------------
const char* vtkPVSaveBatchScriptDialog::GetGeometryFileName()
{
  if ( ! this->SaveGeometryCheck->GetState())
    {
    return NULL;
    }

  return this->GeometryFileNameEntry->GetValue();
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::SaveImagesCheckCallback()
{
  if (this->SaveImagesCheck->GetState())
    {
    this->ImageFileNameEntry->SetEnabled(1);
    this->ImageFileNameBrowseButton->SetEnabled(1);
    }
  else
    {
    this->ImageFileNameEntry->SetEnabled(0);
    this->ImageFileNameBrowseButton->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::SaveGeometryCheckCallback()
{
  if (this->SaveGeometryCheck->GetState())
    {
    this->GeometryFileNameEntry->SetEnabled(1);
    this->GeometryFileNameBrowseButton->SetEnabled(1);
    }
  else
    {
    this->GeometryFileNameEntry->SetEnabled(0);
    this->GeometryFileNameBrowseButton->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::ImageFileNameBrowseButtonCallback()
{
  ostrstream str;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkKWLoadSaveDialog* loadDialog = pm->NewLoadSaveDialog();
  loadDialog->Create(this->GetPVApplication(), 0);
  loadDialog->SetTitle("Select File Pattern");

  // Look for the current extension.
  char *fileName = this->ImageFileNameEntry->GetValue();
  char *ptr;
  char *ext = NULL;

  ptr = fileName;
  while (*ptr != '\0')
    {
    if (*ptr == '.')
      {
      ext = ptr;
      }
    ++ptr;
    }

  if (ext == NULL || ext[1] == '\0')
    {
    loadDialog->SetDefaultExtension("jpg");
    }
  else
    {
    loadDialog->SetDefaultExtension(ext);
    }
  str << "{{} {.jpg}} {{} {.tif}} {{} {.png}} ";
  str << "{{All files} {*}}" << ends;  
  loadDialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);  
  if(loadDialog->Invoke())
    {
    this->ImageFileNameEntry->SetValue(loadDialog->GetFileName());
    }

  loadDialog->Delete();
}







//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::GeometryFileNameBrowseButtonCallback()
{
  ostrstream str;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkKWLoadSaveDialog* loadDialog = pm->NewLoadSaveDialog();
  loadDialog->Create(this->GetPVApplication(), 0);
  loadDialog->SetTitle("Select Geometry File Pattern");

  // Look for the current extension.
  char *fileName = this->GeometryFileNameEntry->GetValue();
  char *ptr;
  char *ext = NULL;

  ptr = fileName;
  while (*ptr != '\0')
    {
    if (*ptr == '.')
      {
      ext = ptr;
      }
    ++ptr;
    }

  if (ext == NULL || ext[1] == '\0')
    {
    loadDialog->SetDefaultExtension("vtk");
    }
  else
    {
    loadDialog->SetDefaultExtension(ext);
    }
  str << "{{} {.vtk}} ";
  str << "{{All files} {*}}" << ends;  
  loadDialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);  
  if(loadDialog->Invoke())
    {
    this->GeometryFileNameEntry->SetValue(loadDialog->GetFileName());
    }

  loadDialog->Delete();
}



//----------------------------------------------------------------------------
int vtkPVSaveBatchScriptDialog::Invoke()
{   
  this->Script("wm deiconify %s", this->GetWidgetName());
  this->Script("grab %s", this->GetWidgetName());

  this->Exit = 0;
  this->AcceptedFlag = 0;
  while (this->Exit == 0)
    {
    // I assume the update will process multiple events.
    this->Script("update");
    if (this->Exit == 0)
      {
      this->Script("after 100");
      }
    }

  this->Script("grab release %s", this->GetWidgetName());
  this->Script("wm withdraw %s", this->GetWidgetName());

  return this->AcceptedFlag;
}



//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::Accept()
{
  this->Exit = 1;
  this->AcceptedFlag = 1;
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::Cancel()
{
  this->Exit = 1;
  this->AcceptedFlag = 0;
}

//----------------------------------------------------------------------------
vtkPVApplication *vtkPVSaveBatchScriptDialog::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->Application);
}


//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << endl;
  os << indent << "FilePath: " << (this->FilePath ? this->FilePath : "(none)") << endl;
  os << indent << "FileRoot: " << (this->FileRoot ? this->FileRoot : "(none)") << endl;
}
