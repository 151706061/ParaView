/*=========================================================================

  Program:   ParaView
  Module:    vtkXDMFReaderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXDMFReaderModule.h"

#include "vtkCollectionIterator.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVFileEntry.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkKWListBox.h"
#include "vtkKWPushButton.h"
#include "vtkPVColorMap.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMDisplayProxy.h"

#include <vtkstd/string>
#include <vtkstd/map>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXDMFReaderModule);
vtkCxxRevisionMacro(vtkXDMFReaderModule, "1.36");

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
  this->DomainGridFrame = 0;
  this->DomainMenu = 0;

  this->Domain = 0;

  this->GridSelection = 0;

  this->Internals = new vtkXDMFReaderModuleInternal;
}

//----------------------------------------------------------------------------
vtkXDMFReaderModule::~vtkXDMFReaderModule()
{
  this->SetDomain(0);
  delete this->Internals;
  if ( this->DomainMenu )
    {
    this->DomainMenu->Delete();
    this->DomainMenu = 0;
    }
  if ( this->GridSelection )
    {
    this->GridSelection->Delete();
    this->GridSelection = 0;
    }
  if ( this->DomainGridFrame )
    {
    this->DomainGridFrame->Delete();
    this->DomainGridFrame = 0;
    }
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

  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << clone->GetVTKSourceID(0) << "SetFileName" << fname
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
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

  vtkClientServerStream stream;

  if ( !this->Domain ||
    this->Internals->GridList.size() == 0 )
    {
    // Prompt user

    // Change the hardcoded "FileName" to something more elaborated
    stream << vtkClientServerStream::Invoke
           << this->GetVTKSourceID(0) << "UpdateInformation"
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

    vtkKWMessageDialog* dlg = vtkKWMessageDialog::New();
    dlg->SetTitle("Domain and Grids Selection");
    dlg->SetStyleToOkCancel();
    dlg->SetMasterWindow(this->GetPVWindow());
    dlg->Create(pvApp);
    dlg->SetText("Select Domain and Grids");

    this->DomainGridFrame = vtkKWFrameLabeled::New();
    this->DomainGridFrame->SetParent(dlg->GetMessageDialogFrame());
    this->DomainGridFrame->Create(pvApp);
    this->DomainGridFrame->SetLabelText("Domain and Grids Selection");

    this->DomainMenu = vtkKWOptionMenu::New();
    this->DomainMenu->SetParent(this->DomainGridFrame->GetFrame());
    this->DomainMenu->Create(pvApp);
    this->UpdateDomains();

    this->GridSelection = vtkKWListBox::New();
    this->GridSelection->SetParent(this->DomainGridFrame->GetFrame());
    this->GridSelection->ScrollbarOn();
    this->GridSelection->Create(pvApp);
    this->GridSelection->SetSelectionModeToExtended();
    this->GridSelection->SetHeight(0);
    this->UpdateGrids();


    this->Script("%s configure -height 1", this->DomainMenu->GetWidgetName());
    this->Script("pack %s -expand yes -fill x -side top -pady 2", 
      this->DomainMenu->GetWidgetName());
    this->Script("pack %s -expand yes -fill x -side top -pady 2", 

      this->GridSelection->GetWidgetName());

    if ( this->DomainMenu->GetNumberOfEntries() > 0 )
      {
      this->Script("pack %s -expand yes -fill x -side top -pady 2", 
        this->DomainGridFrame->GetWidgetName());
      if ( this->GridSelection->GetNumberOfItems() > 1 )
        {
        vtkKWPushButton* selectAllButton = vtkKWPushButton::New();
        selectAllButton->SetParent(this->DomainGridFrame->GetFrame());
        selectAllButton->SetText("Select All Grids");
        selectAllButton->Create(pvApp);
        selectAllButton->SetCommand(this, "EnableAllGrids");
        this->Script("pack %s -expand yes -fill x -side bottom -pady 2", 
          selectAllButton->GetWidgetName());
        selectAllButton->Delete();
        }
      }
    else
      {
      dlg->SetText("No domains found");
      dlg->GetOKButton()->EnabledOff();
      }

    int result = VTK_OK;

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
    else
      {
      result = VTK_ERROR;
      }

    this->DomainMenu->Delete();
    this->DomainMenu = 0;

    this->GridSelection->Delete();
    this->GridSelection = 0;

    this->DomainGridFrame->Delete();
    this->DomainGridFrame = 0;

    dlg->Delete();
    if ( result != VTK_OK )
      {
      return result;
      }
    }

  stream << vtkClientServerStream::Invoke
         << this->GetVTKSourceID(0) << "UpdateInformation"
         << vtkClientServerStream::End;
  if ( this->Domain )
    {
    stream << vtkClientServerStream::Invoke
           << this->GetVTKSourceID(0) << "SetDomainName" << this->Domain 
           << vtkClientServerStream::End;
    this->GetTraceHelper()->AddEntry("$kw(%s) SetDomain {%s}", 
      this->GetTclName(), 
      this->Domain);
    }
  stream << vtkClientServerStream::Invoke
         << this->GetVTKSourceID(0) << "UpdateInformation"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->GetVTKSourceID(0) << "DisableAllGrids"
         << vtkClientServerStream::End;

  vtkXDMFReaderModuleInternal::GridListType::iterator mit;
  for ( mit = this->Internals->GridList.begin(); 
    mit != this->Internals->GridList.end(); 
    ++mit )
    {
    stream << vtkClientServerStream::Invoke
           << this->GetVTKSourceID(0) << "EnableGrid" << mit->first.c_str() 
           << vtkClientServerStream::End;
    this->GetTraceHelper()->AddEntry(
      "$kw(%s) EnableGrid {%s}", this->GetTclName(), mit->first.c_str());
    }


  stream << vtkClientServerStream::Invoke
         << this->GetVTKSourceID(0) << "UpdateInformation"
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

  int retVal = this->InitializeClone(1);
  if (retVal != VTK_OK)
    {
    return retVal;
    }

  retVal =  this->Superclass::ReadFileInformation(fname);
  if (retVal != VTK_OK)
    {
    return retVal;
    }

  // Re-initialize widgets to get the information from the reader.
  this->InitializeWidgets();

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkXDMFReaderModule::Finalize(const char* fname)
{
  return this->Superclass::Finalize(fname);
}

//----------------------------------------------------------------------------
void vtkXDMFReaderModule::UpdateGrids()
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkClientServerStream stream;

  // Get the number of grids.
  stream << vtkClientServerStream::Invoke
         << this->GetVTKSourceID(0) << "UpdateInformation"
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
  stream << vtkClientServerStream::Invoke
         << this->GetVTKSourceID(0) << "GetNumberOfGrids"
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  int numGrids = 0;
  if(!pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &numGrids))
    {
    vtkErrorMacro("Error getting number of grids.");
    }

  // Fill the grid selection list with all the grid names.
  this->GridSelection->DeleteAll();
  for(int i = 0; i < numGrids; ++i)
    {
    stream << vtkClientServerStream::Invoke
           << this->GetVTKSourceID(0) << "GetGridName" << i
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
    const char* gname;
    if(pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &gname))
      {
      this->GridSelection->InsertEntry(i, gname);
      }
    else
      {
      vtkErrorMacro("Error getting name of grid " << i);
      }
    }

  // Set the default selection and enable the scrollbar if necessary.
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
void vtkXDMFReaderModule::UpdateDomains()
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkClientServerStream stream;

  // Get the number of domains.
  stream << vtkClientServerStream::Invoke
         << this->GetVTKSourceID(0) << "UpdateInformation"
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
  stream << vtkClientServerStream::Invoke
         << this->GetVTKSourceID(0) << "GetNumberOfDomains"
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  int numDomains = 0;
  if(!pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &numDomains))
    {
    vtkErrorMacro("Error getting number of grids.");
    }

  // Fill the domain menu with the name of each domain.
  this->DomainMenu->DeleteAllEntries();
  for(int i = 0; i < numDomains; ++i)
    {
    stream << vtkClientServerStream::Invoke
           << this->GetVTKSourceID(0) << "GetDomainName" << i
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
    const char* dname;
    if(pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &dname))
      {
      this->DomainMenu->AddEntryWithCommand(dname, this, "UpdateGrids");

      // Set the menu selection to the first entry.
      if(i == 0)
        {
        this->DomainMenu->SetValue(dname);
        }
      }
    else
      {
      vtkErrorMacro("Error getting name of grid " << i);
      }
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
    *file << "$kw(" << this->GetTclName() << ") EnableGrid {" << mit->first.c_str() << "}" << endl;
    }
  *file << "$kw(" << this->GetPVWindow()->GetTclName() << ") "
        << "ReadFileInformation $kw(" << this->GetTclName() << ") \""
        << this->FileEntry->GetValue() << "\"" << endl;
  *file << "$kw(" << this->GetPVWindow()->GetTclName() << ") "
        << "FinalizeRead $kw(" << this->GetTclName() << ") \""
        << this->FileEntry->GetValue() << "\"" << endl;

  // Let the PVWidgets set up the object.
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  
  int numWidgets = this->Widgets->GetNumberOfItems();
  for (int i = 0; i < numWidgets; i++)
    {
    vtkPVWidget* pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    pvw->SaveState(file);
    it->GoToNextItem();
    }
  it->Delete();

  // Call accept.
  *file << "$kw(" << this->GetTclName() << ") AcceptCallback" << endl;

  this->VisitedFlag = 1;
}

//----------------------------------------------------------------------------
void vtkXDMFReaderModule::EnableAllGrids()
{
  int cc;
  for ( cc = 0; cc < this->GridSelection->GetNumberOfItems(); cc ++ )
    {
    this->GridSelection->SetSelectState(cc, 1);
    }
}

//----------------------------------------------------------------------------
void vtkXDMFReaderModule::EnableGrid(const char* grid)
{
  this->Internals->GridList[grid] = 1;
}

//----------------------------------------------------------------------------
void vtkXDMFReaderModule::SaveInBatchScript(ofstream *file)
{
  // This should not be needed, but We can check anyway.
  if (this->VisitedFlag)
    {
    return;
    }
  this->SaveFilterInBatchScript(file);

  if ( this->Domain )
    {
    *file << "  [$pvTemp" << this->GetVTKSourceID(0) 
          << " GetProperty DomainName] SetElement 0 {"
          << this->Domain << "}" << endl;
    *file << "  $pvTemp" << this->GetVTKSourceID(0) << " UpdateVTKObjects" << endl;
    *file << "  $pvTemp" << this->GetVTKSourceID(0) << " UpdateInformation" << endl;
    }
  int numGrids=0;
  vtkXDMFReaderModuleInternal::GridListType::iterator mit;
  for ( mit = this->Internals->GridList.begin(); 
        mit != this->Internals->GridList.end(); 
        ++mit )
    {
    numGrids++;
    }
  *file << "  [$pvTemp" << this->GetVTKSourceID(0) 
        << " GetProperty EnableGrid] SetNumberOfElements "
        << numGrids << endl;
  
  numGrids = 0;
  for ( mit = this->Internals->GridList.begin(); 
        mit != this->Internals->GridList.end(); 
        ++mit )
    {
    *file << "  [$pvTemp" << this->GetVTKSourceID(0) 
          << " GetProperty EnableGrid] SetElement " << numGrids << " {" << mit->first.c_str() << "}" << endl;
    numGrids++;
    }
  *file << "  $pvTemp" << this->GetVTKSourceID(0) << " UpdateVTKObjects" << endl;

  // Add the mapper, actor, scalar bar actor ...
  if (this->GetVisibility())
    {
    if (this->PVColorMap)
      {
      this->PVColorMap->SaveInBatchScript(file);
      }
    
    vtkSMDisplayProxy* pDisp = this->GetDisplayProxy();
    if (pDisp)
      {
      *file << "#Display Proxy" << endl;
      pDisp->SaveInBatchScript(file);
      }
    }
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
