/*=========================================================================

  Program:   ParaView
  Module:    vtkXDMFReaderModule.cxx
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
#include "vtkXDMFReaderModule.h"

#include "vtkCollectionIterator.h"
#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVFileEntry.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkString.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"
#include "vtkKWListBox.h"

#include <vtkstd/string>
#include <vtkstd/map>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXDMFReaderModule);
vtkCxxRevisionMacro(vtkXDMFReaderModule, "1.9");

int vtkXDMFReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

class vtkXDMFReaderModuleInternal
{
public:
  typedef vtkstd::map<vtkstd::string, int> GridListType;
  GridListType GridList;
};

//----------------------------------------------------------------------------
vtkXDMFReaderModule::vtkXDMFReaderModule()
{
  this->DomainMenu = 0;

  this->Domain = 0;

  this->GridSelection = 0;

  this->Internals = new vtkXDMFReaderModuleInternal;
}

//----------------------------------------------------------------------------
vtkXDMFReaderModule::~vtkXDMFReaderModule()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
int vtkXDMFReaderModule::Initialize(const char* fname, 
                                   vtkPVReaderModule*& clone)
{ 
  if (this->ClonePrototypeInternal(reinterpret_cast<vtkPVSource*&>(clone)) 
      != VTK_OK)
    {
    vtkErrorMacro("Error creating reader " << this->GetClassName()
                  << endl);
    clone = 0;
    return VTK_ERROR;
    }
  
  const char* res = clone->GetVTKSourceTclName();
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->ServerScript("%s Set%s {%s}", res, "FileName", fname);

  this->Internals->GridList.erase(
    this->Internals->GridList.begin(),
    this->Internals->GridList.end());

  this->SetDomain(0);

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkXDMFReaderModule::ReadFileInformation(const char* fname)
{
  int cc;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkPVApplication* pvApp = this->GetPVApplication();
  const char* res = this->GetVTKSourceTclName();

  if ( !this->Domain ||
    this->Internals->GridList.size() == 0 )
    {
    // Prompt user

    // Change the hardcoded "FileName" to something more elaborated
    pm->ServerScript("%s UpdateInformation", res);

    vtkKWMessageDialog* dlg = vtkKWMessageDialog::New();
    dlg->SetTitle("Domain and Grids Selection");
    dlg->SetMasterWindow(this->GetPVWindow());
    dlg->Create(pvApp,0);
    dlg->SetText("Select Domain and Grids");
    vtkKWLabeledFrame* frame = vtkKWLabeledFrame::New();
    frame->SetParent(dlg->GetMessageDialogFrame());
    frame->Create(pvApp, 0);
    frame->SetLabel("Domain and Grids Selection");

    this->DomainMenu = vtkKWOptionMenu::New();
    this->DomainMenu->SetParent(frame->GetFrame());
    this->DomainMenu->Create(pvApp, 0);
    this->UpdateDomains(res);


    this->GridSelection = vtkKWListBox::New();
    this->GridSelection->SetParent(frame->GetFrame());
    this->GridSelection->ScrollbarOn();
    this->GridSelection->Create(pvApp, "-selectmode extended");
    this->GridSelection->SetHeight(0);
    this->UpdateGrids(res);

    this->Script("%s configure -height 1", this->DomainMenu->GetWidgetName());
    this->Script("pack %s -expand yes -fill x -side top -pady 2", 
      this->DomainMenu->GetWidgetName());
    this->Script("pack %s -expand yes -fill x -side top -pady 2", 
      this->GridSelection->GetWidgetName());
    this->Script("pack %s -expand yes -fill x -side bottom -pady 2", 
      frame->GetWidgetName());

    if ( dlg->Invoke() )
      {
      this->SetDomain(this->DomainMenu->GetValue());

      for ( cc = 0; cc < this->GridSelection->GetNumberOfItems(); cc ++ )
        {
        if ( this->GridSelection->GetSelectState(cc) )
          {
          this->Internals->GridList[this->GridSelection->GetItem(cc)] = 1;
          }
        }
      }

    this->DomainMenu->Delete();
    this->DomainMenu = 0;


    this->GridSelection->Delete();
    this->GridSelection = 0;
    frame->Delete();
    dlg->Delete();

    }

  pm->ServerScript("%s UpdateInformation", res);
  if ( this->Domain )
    {
    pm->ServerScript("%s SetDomainName \"%s\"", 
      res, 
      this->Domain);
    pvApp->AddTraceEntry("$kw(%s) SetDomain {%s}", 
      this->GetTclName(), 
      this->Domain);
    }

  pm->ServerScript(
    "%s UpdateInformation\n"
    "%s DisableAllGrids", res, res);
  vtkXDMFReaderModuleInternal::GridListType::iterator mit;
  for ( mit = this->Internals->GridList.begin(); 
    mit != this->Internals->GridList.end(); 
    ++mit )
    {
    pm->ServerScript("%s EnableGrid {%s}", res, mit->first.c_str());
    pvApp->AddTraceEntry("$kw(%s) EnableGrid {%s}", this->GetTclName(), mit->first.c_str());
    }

  int retVal = VTK_OK;

  pm->ServerScript("%s UpdateInformation", res);
  retVal = this->InitializeClone(0, 1);

  if (retVal != VTK_OK)
    {
    return retVal;
    }

  retVal =  this->Superclass::ReadFileInformation(fname);
  if (retVal != VTK_OK)
    {
    return retVal;
    }

  // We called UpdateInformation, we need to update the widgets.
  vtkCollectionIterator* it = this->GetWidgets()->NewIterator();
  for ( it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkPVWidget *pvw = static_cast<vtkPVWidget*>(it->GetObject());
    pvw->ModifiedCallback();
    }
  it->Delete();
  this->UpdateParameterWidgets();
  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkXDMFReaderModule::Finalize(const char* fname)
{
  return this->Superclass::Finalize(fname);
}

//----------------------------------------------------------------------------
void vtkXDMFReaderModule::UpdateGrids(const char* ob)
{
  int cc;
  int num;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->ServerScript("%s UpdateInformation", ob);
  pm->RootScript("%s GetNumberOfGrids", ob);
  num = atoi(pm->GetRootResult());
  this->GridSelection->DeleteAll();
  for ( cc = 0; cc < num; cc ++ )
    {
    pm->RootScript("%s GetGridName %d", ob, cc);
    char* gname = vtkString::Duplicate(pm->GetRootResult());
    this->GridSelection->InsertEntry(cc, gname);
    delete [] gname;
    }
  this->GridSelection->SetSelectState(0, 1);
  if ( this->GridSelection->GetNumberOfItems() < 6 )
    {
    this->GridSelection->SetHeight(this->GridSelection->GetNumberOfItems());
    this->GridSelection->ScrollbarOff();
    }
  else
    {
    this->GridSelection->SetHeight(6);
    this->GridSelection->ScrollbarOn();
    }
}

//----------------------------------------------------------------------------
void vtkXDMFReaderModule::UpdateDomains(const char* ob)
{
  int cc;
  int num;
  char buffer[1024];

  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->ServerScript("%s UpdateInformation", ob);
  pm->RootScript("%s GetNumberOfDomains", ob);
  num = atoi(pm->GetRootResult());
  this->DomainMenu->ClearEntries();
  for ( cc = 0; cc < num; cc ++ )
    {
    pm->RootScript("%s GetDomainName %d", ob, cc);
    char* dname = vtkString::Duplicate(pm->GetRootResult());
    sprintf(buffer, "UpdateGrids %s", ob);
    this->DomainMenu->AddEntryWithCommand(dname, this, buffer);
    if ( !cc )
      {
      this->DomainMenu->SetValue(dname);
      }
    delete [] dname;
    }
 
}

//----------------------------------------------------------------------------
void vtkXDMFReaderModule::SaveState(ofstream *file)
{
  if (this->VisitedFlag)
    {
    return;
    }
  
  *file << "set kw(" << this->GetTclName() << ") [$kw("
        << this->GetPVWindow()->GetTclName() << ") InitializeReadCustom \""
        << this->GetModuleName() << "\" \"" << this->FileEntry->GetValue() 
        << "\"]" << endl;
  if ( this->Domain )
    {
    *file << "$kw(" << this->GetTclName() << ") SetDomain " << this->Domain
          << endl;
    }
  vtkXDMFReaderModuleInternal::GridListType::iterator mit;
  for ( mit = this->Internals->GridList.begin(); 
    mit != this->Internals->GridList.end(); 
    ++mit )
    {
    *file << "$kw(" << this->GetTclName() << ") EnableGrid " << mit->first.c_str() << endl;
    }
  *file << "$kw(" << this->GetPVWindow()->GetTclName() << ") "
        << "ReadFileInformation $kw(" << this->GetTclName() << ") \""
        << this->FileEntry->GetValue() << "\"" << endl;
  *file << "$kw(" << this->GetPVWindow()->GetTclName() << ") "
        << "FinalizeRead $kw(" << this->GetTclName() << ") \""
        << this->FileEntry->GetValue() << "\"" << endl;

  // Let the PVWidgets set up the object.
  int numWidgets = this->Widgets->GetNumberOfItems();
  for (int i = 0; i < numWidgets; i++)
    {
    vtkPVWidget* widget = 
      vtkPVWidget::SafeDownCast(this->Widgets->GetItemAsObject(i));
    if (widget)
      {
      widget->SaveState(file);
      }
    }

  // Call accept.
  *file << "$kw(" << this->GetTclName() << ") AcceptCallback" << endl;

  this->VisitedFlag = 1;
}

//----------------------------------------------------------------------------
void vtkXDMFReaderModule::EnableGrid(const char* grid)
{
  this->Internals->GridList[grid] = 1;
}

//----------------------------------------------------------------------------
void vtkXDMFReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Domain: " << (this->Domain?this->Domain:"(none)") << endl;
  vtkXDMFReaderModuleInternal::GridListType::iterator mit;
  int cc = 0;
  for ( mit = this->Internals->GridList.begin(); 
    mit != this->Internals->GridList.end(); 
    ++mit )
    {
    os << indent << "Enabled grid " << cc << " " << mit->first.c_str() << endl;
    cc ++;
    }
}
