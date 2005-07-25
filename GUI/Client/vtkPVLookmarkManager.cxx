/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLookmarkManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkPVLookmarkManager.h"

//#include "vtkToolkits.h" // Needed for vtkPVGeneratedModules
//#include "vtkPVConfig.h" // Needed for vtkPVGeneratedModules
//#include "vtkPVGeneratedModules.h"
#include "vtkPVWidgetCollection.h"
#include "vtkSMDisplayProxy.h"
#include "vtkPVInteractorStyleCenterOfRotation.h"
#include "vtkPVSelectTimeSet.h"
#include "vtkPVStringEntry.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVFileEntry.h"
#include "vtkPVLookmark.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVSelectionList.h"
#include "vtkPVScale.h"
#include "vtkPVReaderModule.h"
#include "vtkPVInputMenu.h"
#include "vtkPVArraySelection.h"
#include "vtkPVSelectWidget.h"
#include "vtkPVCameraIcon.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVMinMax.h"
#ifdef PARAVIEW_USE_EXODUS
#include "vtkPVBasicDSPFilterWidget.h"
#endif
#include "vtkPVGUIClientOptions.h"

#include "vtkKWMenuButton.h"
#include "vtkKWLookmark.h"
#include "vtkKWLookmarkFolder.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWIcon.h"
#include "vtkKWText.h"
#include "vtkKWTextWithScrollbars.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWidget.h"
#include "vtkKWWindow.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWDragAndDropTargetSet.h"
#include "vtkKWToolbar.h"

#include "vtkXMLUtilities.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLLookmarkElement.h"

#include "vtkBase64Utilities.h"
#include "vtkRenderWindow.h"
#include "vtkCollectionIterator.h"
#include "vtkImageReader2.h"
#include "vtkJPEGWriter.h"
#include "vtkWindowToImageFilter.h"
#include "vtkJPEGReader.h"
#include "vtkImageResample.h"
#include "vtkIndent.h"
#include "vtkCamera.h"
#include "vtkVector.txx"
#include "vtkCollection.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkVectorIterator.txx"
#include "vtkImageClip.h"
#include "vtkStdString.h"
#include "vtkPVTraceHelper.h"

#include "vtkCamera.h"
#include "vtkPVProcessModule.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkPVXMLParser.h"
#include "vtkPVXMLElement.h"

#include "vtkSMDisplayProxy.h"

#ifndef _WIN32
  #include <sys/wait.h>
  #include <unistd.h>
#endif
#include <vtkstd/vector>
#include <vtkstd/string>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLookmarkManager);
vtkCxxRevisionMacro(vtkPVLookmarkManager, "1.54");

//----------------------------------------------------------------------------
vtkPVLookmarkManager::vtkPVLookmarkManager()
{
  this->PVLookmarks = vtkVector<vtkPVLookmark*>::New();
  this->LmkFolderWidgets = vtkVector<vtkKWLookmarkFolder*>::New();
  this->MacroExamples = vtkVector<vtkPVLookmark*>::New();

  this->LmkPanelFrame = vtkKWFrame::New();
  this->LmkScrollFrame = vtkKWFrameWithScrollbar::New();
  this->SeparatorFrame = vtkKWFrame::New();
  this->TopDragAndDropTarget = vtkKWFrame::New();
  this->BottomDragAndDropTarget= vtkKWFrame::New();
  this->CreateLmkButton = vtkKWPushButton::New();

  this->MenuFile = vtkKWMenu::New();
  this->MenuEdit = vtkKWMenu::New();
  this->MenuImport = vtkKWMenu::New();
  this->MenuHelp = vtkKWMenu::New();
  this->MenuExamples = vtkKWMenu::New();

  this->QuickStartGuideDialog = 0;
  this->QuickStartGuideTxt = 0;
  this->UsersTutorialDialog = 0;
  this->UsersTutorialTxt = 0;

  this->TraceHelper = vtkPVTraceHelper::New();
  this->TraceHelper->SetObject(this);

  this->SetTitle("Lookmark Manager");
}

//----------------------------------------------------------------------------
vtkPVLookmarkManager::~vtkPVLookmarkManager()
{
  this->Checkpoint();

  this->TraceHelper->Delete();
  this->TraceHelper = 0;

  this->CreateLmkButton->Delete();
  this->CreateLmkButton= NULL;
  this->SeparatorFrame->Delete();
  this->SeparatorFrame= NULL;
  this->TopDragAndDropTarget->Delete();
  this->TopDragAndDropTarget= NULL;
  this->BottomDragAndDropTarget->Delete();
  this->BottomDragAndDropTarget= NULL;

  this->MenuEdit->Delete();
  this->MenuImport->Delete();
  this->MenuFile->Delete();
  this->MenuHelp->Delete();
  this->MenuExamples->Delete();

  if (this->QuickStartGuideTxt)
    {
    this->QuickStartGuideTxt->Delete();
    this->QuickStartGuideTxt = NULL;
    }

  if (this->QuickStartGuideDialog)
    {
    this->QuickStartGuideDialog->Delete();
    this->QuickStartGuideDialog = NULL;
    }

  if (this->UsersTutorialTxt)
    {
    this->UsersTutorialTxt->Delete();
    this->UsersTutorialTxt = NULL;
    }

  if (this->UsersTutorialDialog)
    {
    this->UsersTutorialDialog->Delete();
    this->UsersTutorialDialog = NULL;
    }

  if(this->MacroExamples)
    {
    vtkVectorIterator<vtkPVLookmark *> *it = this->MacroExamples->NewIterator();
    while (!it->IsDoneWithTraversal())
      {
      vtkPVLookmark *lmk = 0;
      if (it->GetData(lmk) == VTK_OK && lmk)
        {
        lmk->Delete();
        }
      it->GoToNextItem();
      }
    it->Delete();
    this->MacroExamples->Delete();
    this->MacroExamples = 0;
    }

  if(this->PVLookmarks)
    {
    vtkVectorIterator<vtkPVLookmark *> *it = this->PVLookmarks->NewIterator();
    while (!it->IsDoneWithTraversal())
      {
      vtkPVLookmark *lmk = 0;
      if (it->GetData(lmk) == VTK_OK && lmk)
        {
        lmk->Delete();
        }
      it->GoToNextItem();
      }
    it->Delete();
    this->PVLookmarks->Delete();
    this->PVLookmarks = 0;
    }

  if(this->LmkFolderWidgets)
    {
    vtkVectorIterator<vtkKWLookmarkFolder *> *it = this->LmkFolderWidgets->NewIterator();
    while (!it->IsDoneWithTraversal())
      {
      vtkKWLookmarkFolder *lmkFolder = 0;
      if (it->GetData(lmkFolder) == VTK_OK && lmkFolder)
        {
        lmkFolder->Delete();
        }
      it->GoToNextItem();
      }
    it->Delete();
    this->LmkFolderWidgets->Delete();
    this->LmkFolderWidgets = 0;
    }

  this->LmkScrollFrame->Delete();
  this->LmkScrollFrame = NULL;
  this->LmkPanelFrame->Delete();
  this->LmkPanelFrame= NULL;
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVLookmarkManager::GetPVRenderView()
{
  return this->GetPVApplication()->GetMainView();
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVLookmarkManager::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}



//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Create(vtkKWApplication *app)
{
  // Check if already created
  char methodAndArgs[100];

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  this->SetGeometry("380x700+0+0");

  // Menu : File

  vtkKWMenu *root_menu = this->GetMenu();

  this->MenuFile->SetParent(root_menu);
  this->MenuFile->SetTearOff(0);
  this->MenuFile->Create(app);

  this->MenuImport->SetParent(this->MenuFile);
  this->MenuImport->SetTearOff(0);
  this->MenuImport->Create(app);
  char* rbv = 
    this->MenuImport->CreateRadioButtonVariable(this, "Import");
  this->Script( "set %s 0", rbv );
  this->MenuImport->AddRadioButton(0, "Replace", rbv, this, "ImportCallback", 0);
  this->MenuImport->AddRadioButton(1, "Append", rbv, this, "ImportCallback", 1);
  delete [] rbv;

  root_menu->AddCascade("File", this->MenuFile, 0);
  this->MenuFile->AddCascade("Import", this->MenuImport,0);
  this->MenuFile->AddCommand("Save As", this, "SaveAllCallback");
  this->MenuFile->AddCommand("Export Folder", this, "ExportFolderCallback");
  this->MenuFile->AddCommand("Close", this, "Withdraw");

  // Menu : Edit

  this->MenuEdit->SetParent(root_menu);
  this->MenuEdit->SetTearOff(0);
  this->MenuEdit->Create(app);
  root_menu->AddCascade("Edit", this->MenuEdit, 0);
  this->MenuEdit->AddCommand("Undo", this, "UndoCallback");
  this->MenuEdit->AddCommand("Redo", this, "RedoCallback");
  this->MenuEdit->AddSeparator();

  this->MenuExamples->SetParent(this->MenuEdit);
  this->MenuExamples->SetTearOff(0);
  this->MenuExamples->Create(app);
  this->MenuEdit->AddCascade("Add Existing Macro", this->MenuExamples,0);
  sprintf(methodAndArgs,"CreateLookmarkCallback 1");
  this->MenuEdit->AddCommand("Create Macro", this, methodAndArgs);
  this->MenuEdit->AddSeparator();
//  char* cbv = 
//    this->MenuExamples->CreateCheckButtonVariable(this, "MacroExamples");
//  this->Script( "set %s 0", cbv );
  this->ImportMacroExamplesCallback();
//  delete [] cbv;

  sprintf(methodAndArgs,"CreateLookmarkCallback 0");
  this->MenuEdit->AddCommand("Create Lookmark", this, methodAndArgs);
  this->MenuEdit->AddCommand("Update Lookmark", this, "UpdateLookmarkCallback");
  this->MenuEdit->AddCommand("Rename Lookmark", this, "RenameLookmarkCallback");
  this->MenuEdit->AddSeparator();
  this->MenuEdit->AddCommand("Create Folder", this, "CreateFolderCallback");
  this->MenuEdit->AddCommand("Rename Folder", this, "RenameFolderCallback");
  this->MenuEdit->AddSeparator();
  this->MenuEdit->AddCommand("Remove Item(s)", this, "RemoveCallback");
  this->MenuEdit->AddSeparator();
  this->MenuEdit->AddCommand("Select All", this, "AllOnOffCallback 1");
  this->MenuEdit->AddCommand("Clear All", this, "AllOnOffCallback 0");
  this->MenuEdit->SetItemState("Undo", vtkKWTkOptions::StateDisabled);
  this->MenuEdit->SetItemState("Redo", vtkKWTkOptions::StateDisabled);

  // Menu : Help

  this->MenuHelp->SetParent(root_menu);
  this->MenuHelp->SetTearOff(0);
  this->MenuHelp->Create(app);
  root_menu->AddCascade("Help", this->MenuHelp, 0);
  this->MenuHelp->AddCommand("Quick Start Guide", this, "DisplayQuickStartGuide");
  this->MenuHelp->AddCommand("User's Tutorial", this, "DisplayUsersTutorial");

  this->LmkPanelFrame->SetParent(this);
  this->LmkPanelFrame->Create(this->GetPVApplication());

  this->LmkScrollFrame->SetParent(this->LmkPanelFrame);
  this->LmkScrollFrame->Create(this->GetPVApplication());

  this->SeparatorFrame->SetParent(this->LmkPanelFrame);
  this->SeparatorFrame->Create(this->GetPVApplication());
  this->SeparatorFrame->SetBorderWidth(2);
  this->SeparatorFrame->SetReliefToGroove();

  this->CreateLmkButton->SetParent(this->LmkPanelFrame);
  this->CreateLmkButton->Create(this->GetPVApplication());
  this->CreateLmkButton->SetText("Create Lookmark");
  sprintf(methodAndArgs,"CreateLookmarkCallback 0");
  this->CreateLmkButton->SetCommand(this,methodAndArgs);

  this->TopDragAndDropTarget->SetParent(this->LmkScrollFrame->GetFrame());
  this->TopDragAndDropTarget->Create(this->GetPVApplication());

  this->BottomDragAndDropTarget->SetParent(this->LmkScrollFrame->GetFrame());
  this->BottomDragAndDropTarget->Create(this->GetPVApplication());

  this->Script("pack %s -padx 2 -pady 4 -expand t", 
                this->CreateLmkButton->GetWidgetName());
  this->Script("pack %s -ipady 1 -pady 2 -anchor nw -expand t -fill x",
                 this->SeparatorFrame->GetWidgetName());

  this->Script("pack %s -anchor w -fill both -side top",
                 this->TopDragAndDropTarget->GetWidgetName());
  this->Script("%s configure -height 12",
                 this->TopDragAndDropTarget->GetWidgetName());
  this->Script("pack %s -anchor w -fill x -pady 12 -side top",
                 this->LmkScrollFrame->GetWidgetName());
  this->Script("pack %s -anchor n -side top -fill x -expand t",
                this->LmkPanelFrame->GetWidgetName());

  this->Script("set commandList \"\"");

  // Import the lookmark file stored in the user's home directory if there is one
  ostrstream str;
  #ifndef _WIN32
  if ( !getenv("HOME") )
    {
    return;
    }
  str << getenv("HOME") << "/.ParaViewlmk" << ends;
  #else
  if ( !getenv("HOMEPATH") )
    {
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#ParaViewlmk#" << ends;
  #endif
  ifstream infile(str.str());
  if ( !infile.fail())
    {
    this->Import(str.str(),0);
    }

  vtkKWLookmarkFolder *folder = this->GetMacrosFolder();
  if(!folder)
    {
    vtkKWLookmarkFolder *macroFolder = vtkKWLookmarkFolder::New();
    macroFolder->SetParent(this->LmkScrollFrame->GetFrame());
    macroFolder->SetMacroFlag(1);
    macroFolder->Create(this->GetPVApplication());
    this->Script("pack %s -fill both -expand yes -padx 8",macroFolder->GetWidgetName());
    this->Script("%s configure -height 8",macroFolder->GetLabelFrame()->GetFrame()->GetWidgetName());
    macroFolder->SetFolderName("Macros");
    macroFolder->SetLocation(this->GetNumberOfChildLmkItems(this->LmkScrollFrame->GetFrame()));
    this->LmkFolderWidgets->InsertItem(this->LmkFolderWidgets->GetNumberOfItems(),macroFolder);
    this->DragAndDropWidget(macroFolder,this->TopDragAndDropTarget);
    this->PackChildrenBasedOnLocation(macroFolder->GetParent());
    this->ResetDragAndDropTargetSetAndCallbacks();

    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::AddMacroExampleCallback(int index)
{
  vtkPVLookmark *lookmarkWidget;
  this->MacroExamples->GetItem(index,lookmarkWidget);
  vtkPVLookmark *newLookmark = vtkPVLookmark::New();
  newLookmark->SetName(lookmarkWidget->GetName());
  newLookmark->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
  ostrstream s;
  s << "GetPVLookmark \"" << newLookmark->GetName() << "\"" << ends;
  newLookmark->GetTraceHelper()->SetReferenceCommand(s.str());
  s.rdbuf()->freeze(0);
  newLookmark->SetStateScript(lookmarkWidget->GetStateScript());
  newLookmark->SetName(lookmarkWidget->GetName());
  newLookmark->SetComments(lookmarkWidget->GetComments());
//ds
  newLookmark->SetDataset(lookmarkWidget->GetDataset());
  newLookmark->CreateDatasetList();
  newLookmark->SetImageData(lookmarkWidget->GetImageData());
  newLookmark->SetPixelSize(lookmarkWidget->GetPixelSize());
  newLookmark->SetMacroFlag(1);

  newLookmark->SetApplication(this->GetApplication());
  newLookmark->SetParent(this->GetMacrosFolder()->GetLabelFrame()->GetFrame());
  newLookmark->Create(this->GetPVApplication());
  char methodAndArg[200];
  sprintf(methodAndArg,"SelectItemCallback %s",newLookmark->GetWidgetName());
  newLookmark->GetCheckbox()->SetCommand(this,methodAndArg);
  newLookmark->UpdateWidgetValues();
  newLookmark->CommentsModifiedCallback();
  this->Script("pack %s -fill both -expand yes -padx 8",newLookmark->GetWidgetName());
  newLookmark->CreateIconFromImageData();
  newLookmark->SetLocation(this->GetNumberOfChildLmkItems(this->GetMacrosFolder()->GetLabelFrame()->GetFrame()));
//  this->MacroExamples->RemoveItem(index);
  this->PVLookmarks->InsertItem(this->PVLookmarks->GetNumberOfItems(),newLookmark);
  this->ResetDragAndDropTargetSetAndCallbacks();
}
/*
//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SetLookmarkMacros(char *string)
{
  this->LookmarkMacros = new char[strlen(string)];
  strcpy(this->LookmarkMacros,string);
}
*/

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SelectItemCallback(char *widgetName)
{
  // loop thru till you find the widget that this name belongs to 
  // if it has been selected and it is a lookmark, return, if it is a folder, call selectcallback and return
  // loop thru folders, checking whether given widget is inside of it
  // if it is, and the folder is selected, deselect it
  // if widget is a folder, call selectcallback

  vtkKWWidget *widget = NULL;
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType numberOfLookmarkWidgets, numberOfLookmarkFolders;
  numberOfLookmarkWidgets = this->PVLookmarks->GetNumberOfItems();
  numberOfLookmarkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  int i = 0;
  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->PVLookmarks->GetItem(i,lookmarkWidget);
    if(!strcmp(lookmarkWidget->GetWidgetName(),widgetName))
      {
      widget = lookmarkWidget;
      break;
      }
    }
  if(!widget)
    {
    for(i=numberOfLookmarkFolders-1;i>=0;i--)
      {
      this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
      if(!strcmp(lmkFolderWidget->GetWidgetName(),widgetName))
        {
        widget = lmkFolderWidget;
        break;
        }
      }
    }
  if(!widget)
    {
    return;
    }

  lookmarkWidget = vtkPVLookmark::SafeDownCast(widget);
  vtkKWLookmarkFolder *folderWidget = vtkKWLookmarkFolder::SafeDownCast(widget);
  if(lookmarkWidget)
    {
    if(lookmarkWidget->GetSelectionState())
      {
      return;
      }
    for(i=numberOfLookmarkFolders-1;i>=0;i--)
      {
      this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
      if(this->IsWidgetInsideFolder(lookmarkWidget,lmkFolderWidget) && lmkFolderWidget->GetSelectionState())
        {
        lmkFolderWidget->SetSelectionState(0);
        }
      }
    }
  else if(folderWidget)
    {
    if(folderWidget->GetSelectionState())
      {
      folderWidget->SelectCallback();
      return;
      }
    for(i=numberOfLookmarkFolders-1;i>=0;i--)
      {
      this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
      if(this->IsWidgetInsideFolder(folderWidget,lmkFolderWidget) && lmkFolderWidget->GetSelectionState())
        {
        lmkFolderWidget->SetSelectionState(0);
        }
      }
    folderWidget->SelectCallback();
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ImportMacroExamplesCallback()
{
  vtkXMLDataParser *parser;
  vtkXMLDataElement *root;
  vtkPVLookmark *lookmarkWidget;
  int j, numLmks, retval;
  char msg[500];

  if(this->GetPVApplication()->GetGUIClientOptions()->GetDisableRegistry())
    {
    return;
    }

//  char* init_string;
//  init_string = vtkPVDefaultModulesLookmarkMacrosGetInterfaces();

  ostrstream str;

  #ifdef _WIN32
  if ( !getenv("HOMEPATH") )
    {
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#LookmarkMacros#" << ends;
  #else
  if ( !getenv("HOME") )
    {
    return;
    }
  str << getenv("HOME") << "/.LookmarkMacros" << ends;
  #endif

  ifstream infile(str.str());
  if ( infile.fail())
    {
    return;
    }

  //parse the .lmk xml file and get the root node for traversing
  parser = vtkXMLDataParser::New();
  parser->SetStream(&infile);
  //parser->Parse(this->LookmarkMacros);
  retval = parser->Parse();
  if(retval==0)
    {
    sprintf(msg,"Error parsing lookmark file in %s.",str.str());
    this->GetPVApplication()->GetMainWindow()->ErrorMessage(msg);
    parser->Delete();
    return;
    } 
  root = parser->GetRootElement();

  vtkXMLDataElement *lmkElement;
  for(j=0; j<root->GetNumberOfNestedElements(); j++)
    {
    lmkElement = root->GetNestedElement(j);

    // this uses a vtkXMLLookmarkElement to create a vtkPVLookmark object
    // create lookmark widget
    lookmarkWidget = this->GetPVLookmark(lmkElement);
    lookmarkWidget->SetMacroFlag(1);
    numLmks = this->PVLookmarks->GetNumberOfItems();
    this->MacroExamples->InsertItem(j,lookmarkWidget);
    ostrstream checkCommand;
    checkCommand << "AddMacroExampleCallback " << j << ends;
//    this->MenuExamples->AddCascade(lookmarkWidget->GetName(), buttonVar, this, checkCommand.str());
    this->MenuExamples->AddCommand(lookmarkWidget->GetName(), this, checkCommand.str());

    checkCommand.rdbuf()->freeze(0);
    }

  parser->Delete();
//  delete [] init_string;

}


//----------------------------------------------------------------------------
vtkKWLookmarkFolder* vtkPVLookmarkManager::GetMacrosFolder()
{
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(int i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(!strcmp(lmkFolderWidget->GetFolderName(),"Macros"))
      {
      return lmkFolderWidget;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Withdraw()
{
  this->Superclass::Withdraw();

  this->GetTraceHelper()->AddEntry("$kw(%s) Withdraw",
                      this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Display()
{
  this->Superclass::Display();

  this->GetTraceHelper()->AddEntry("$kw(%s) Display",
                      this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DisplayQuickStartGuide()
{
  if (!this->QuickStartGuideDialog)
    {
    this->QuickStartGuideDialog = vtkKWMessageDialog::New();
    }

  if (!this->QuickStartGuideDialog->IsCreated())
    {
    this->QuickStartGuideDialog->SetMasterWindow(this->MasterWindow);
    this->QuickStartGuideDialog->Create(this->GetPVApplication());
    this->QuickStartGuideDialog->SetReliefToSolid();
    this->QuickStartGuideDialog->SetBorderWidth(1);
    this->QuickStartGuideDialog->SetModal(0);
    }

  this->ConfigureQuickStartGuide();

  this->QuickStartGuideDialog->Invoke();

  this->Focus();
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ConfigureQuickStartGuide()
{
  vtkPVApplication *app = this->GetPVApplication();

  if (!this->QuickStartGuideTxt)
    {
    this->QuickStartGuideTxt = vtkKWTextWithScrollbars::New();
    }

  if (!this->QuickStartGuideTxt->IsCreated())
    {
    this->QuickStartGuideTxt->SetParent(this->QuickStartGuideDialog->GetBottomFrame());
    this->QuickStartGuideTxt->Create(app);
    this->QuickStartGuideTxt->VerticalScrollbarVisibilityOn();

    vtkKWText *text = this->QuickStartGuideTxt->GetWidget();
    text->ResizeToGridOn();
    text->SetWidth(60);
    text->SetHeight(20);
    text->SetWrapToWord();
    text->ReadOnlyOn();
    double r, g, b;
    vtkKWCoreWidget *parent = vtkKWCoreWidget::SafeDownCast(text->GetParent());
    parent->GetBackgroundColor(&r, &g, &b);
    text->SetBackgroundColor(r, g, b);
    }

  this->Script("pack %s -side left -padx 2 -expand 1 -fill both",
                this->QuickStartGuideTxt->GetWidgetName());
  this->Script("pack %s -side bottom",  // -expand 1 -fill both
                this->QuickStartGuideDialog->GetMessageDialogFrame()->GetWidgetName());

  this->QuickStartGuideDialog->SetTitle("Lookmarks Quick-Start Guide");

  ostrstream str;

  str << "A Quick Start Guide for Lookmarks in ParaView" << endl << endl;
  str << "Step 1:" << endl << endl;
  str << "Open your dataset." << endl << endl;
  str << "Step 2:" << endl << endl;
  str << "Visit some feature of interest, set the view parameters as desired." << endl << endl;
  str << "Step 3:" << endl << endl;
  str << "In the Lookmark Manager, press \"Create Lookmark\". Note that a lookmark entry has appeared." << endl << endl;
  str << "Step 4:" << endl << endl;
  str << "Visit some other feature of interest, set the view parameters as desired." << endl << endl;
  str << "Step 5:" << endl << endl;
  str << "In the Lookmark Manager, press \"Create Lookmark\". Note that another lookmark entry has appeared." << endl << endl;
  str << "Step 6:" << endl << endl;
  str << "Click the thumbnail of the first lookmark. Note that ParaView returns to those view parameters and then hands control over to you." << endl << endl;
  str << "Step 7:" << endl << endl;
  str << "Click the thumbnail of the second lookmark. Note the same behavior." << endl << endl;
  str << "Step 8:" << endl << endl;
  str << "Read the User's Tutorial also available in the Help menu and explore the Lookmark Manager interface, to learn how to:" << endl << endl;
  str << "- Organize and edit lookmarks." << endl << endl;
  str << "- Save and import lookmarks to and from disk." << endl << endl;
  str << "- Apply a lookmark to a dataset different from the one with which it was created." << endl << endl;
  str << ends;
  this->QuickStartGuideTxt->GetWidget()->SetValue( str.str() );
  str.rdbuf()->freeze(0);
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DisplayUsersTutorial()
{
  if (!this->UsersTutorialDialog)
    {
    this->UsersTutorialDialog = vtkKWMessageDialog::New();
    }

  if (!this->UsersTutorialDialog->IsCreated())
    {
    this->UsersTutorialDialog->SetMasterWindow(this->MasterWindow);
    this->UsersTutorialDialog->Create(this->GetPVApplication());
    this->UsersTutorialDialog->SetReliefToSolid();
    this->UsersTutorialDialog->SetBorderWidth(1);
    this->UsersTutorialDialog->SetModal(0);
    }

  this->ConfigureUsersTutorial();

  this->UsersTutorialDialog->Invoke();

  this->Focus();
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ConfigureUsersTutorial()
{
  vtkPVApplication *app = this->GetPVApplication();

  if (!this->UsersTutorialTxt)
    {
    this->UsersTutorialTxt = vtkKWTextWithScrollbars::New();
    }
  if (!this->UsersTutorialTxt->IsCreated())
    {
    this->UsersTutorialTxt->SetParent(this->UsersTutorialDialog->GetBottomFrame());
    this->UsersTutorialTxt->Create(app);
    this->UsersTutorialTxt->VerticalScrollbarVisibilityOn();

    vtkKWText *text = this->UsersTutorialTxt->GetWidget();
    text->ResizeToGridOn();
    text->SetWidth(60);
    text->SetHeight(20);
    text->SetWrapToWord();
    text->ReadOnlyOn();
    double r, g, b;
    vtkKWCoreWidget *parent = vtkKWCoreWidget::SafeDownCast(text->GetParent());
    parent->GetBackgroundColor(&r, &g, &b);
    text->SetBackgroundColor(r, g, b);
    }

  this->Script("pack %s -side left -padx 2 -expand 1 -fill both",
                this->UsersTutorialTxt->GetWidgetName());
  this->Script("pack %s -side bottom",  // -expand 1 -fill both
                this->UsersTutorialDialog->GetMessageDialogFrame()->GetWidgetName());

  this->UsersTutorialDialog->SetTitle("Lookmarks' User's Manual");

  ostrstream str;

  str << "A User's Manual for Lookmarks in ParaView" << endl << endl;

  str << "Introduction:" << endl << endl;
  str << "Lookmarks provide a way to save and manage views of what you consider to ";
  str << "be the important regions of your dataset in a fashion analogous to how bookmarks ";
  str << "are used by a web browser, all within ParaView. They automate the mundane task of ";
  str << "recreating complex filter trees, making it possible to easily toggle back and ";
  str << "forth between views. They enable more effective data comparison because they can ";
  str << "be applied to different datasets with similar geometry. They can be saved to a single ";
  str << "file and then imported in a later ParaView session or shared with co-workers for ";
  str << "collaboration purposes. A lookmark is a time-saving tool that automates the recreation ";
  str << "of a complex view of data." << endl << endl;

  str << "Feedback:" << endl << endl;
  str << "Lookmarks in ParaView are still in an early stage of development. Any feedback you ";
  str << "have would be of great value. To report errors, suggest features or changes, or comment ";
  str << "on the functionality, please send an email to sdm-vs@ca.sandia.gov or call Eric Stanton ";
  str << "at 505-284-4422." << endl << endl;

  str << "Terminology:" << endl << endl;
  str << "Lookmark Manager - It is here that you import lookmarks into ParaView, toggle from ";
  str << "one lookmark to another, save and remove lookmarks as you deem appropriate, and create ";
  str << "new lookmarks of the data. In addition, the Lookmark Manager is hierarchical so that you ";
  str << "can organize the lookmarks into nested folders." << endl << endl;

  str << "Lookmark Widget - You interact with lookmarks through the lookmark widgets displayed in ";
  str << "the Lookmark Manager. Each widget contains a thumbnail preview of the lookmark, a ";
  str << "collapsible comments area, its default dataset name, and the name of the lookmark itself. ";
  str << "A single-click of the thumbnail generates that lookmark in the ParaView window. A checkbox ";
  str << "placed in front of each widget enables single or multiple lookmark selections for removing, ";
  str << "renaming, and updating lookmarks. A lookmark widget can also be dragged and dropped to other ";
  str << "parts of the Lookmark Manager by grabbing its label." << endl << endl;

  str << "Lookmark File - The contents of the Lookmark Manager or a sub-folder can be saved to a ";
  str << "lookmark file, a text-based, XML-structured file that stores the state information of all ";
  str << "lookmarks in the Lookmark Manager. This file can be loaded into any ParaView session and is ";
  str << "not tied to a particular dataset. It is this file that can easily be shared with co-workers." << endl << endl;

  str << "How To:" << endl << endl;
  str << "Display the Lookmark Manager window - Select \"Window\" >> \"Lookmark Manager\" in the ";
  str << "top ParaView menu. The window that appears is detached from the main ParaView window. Note that you ";
  str << "can interact with the main ParaView window and the Lookmark Manager window remains in the foreground. ";
  str << "The Lookmark Manager window can be closed and reopened without affecting the contents of the Lookmark ";
  str << "Manager." << endl << endl;

  str << "Create a new lookmark - Press the \"Create Lookmark\" button or select it in the \"Edit\" ";
  str << "menu. Note that the Lookmark Manager will momentarily be moved behind the main ParaView window. This ";
  str << "is normal and necessary to generate the thumbnail of the current view. The state of the applicable ";
  str << "filters is saved with the lookmark. It is assigned an initial name of the form Lookmark#. A lookmark ";
  str << "widget is then appended to the bottom of the Lookmark Manager." << endl << endl;

  str << "View a lookmark - Click on the thumbnail of the lookmark you wish to view. You will then ";
  str << "witness the appropriate filters being created in the main ParaView window. Note the lookmark name ";
  str << "has been appended to the filter name of each filter belonging to this lookmark.  Clicking this same ";
  str << "lookmark again will cause these filters to be deleted if possible (i.e. if they have not been set as ";
  str << "inputs to other filters) and the saved filters will be regenerated. See also How to change the dataset ";
  str << "to which lookmarks are applied. " << endl << endl;

  str << "Update an existing lookmark - Select the lookmark to be updated and then press \"Edit\" >> ";
  str << "\"Update Lookmark\". This stores the state of all filters that contribute to the current view in that ";
  str << "lookmark. The lookmark's thumbnail is also replaced to reflect the current view. All other attributes ";
  str << "of the lookmark widget (name, comments, etc.) remain unchanged." << endl << endl;

  str << "Save the contents of the Lookmark Manager - Press \"File\" >> \"Save As\". You will be asked ";
  str << "to select a new or pre-existing lookmark file (with a .lmk extension) to which to save. All information ";
  str << "needed to recreate the current state of the Lookmark Manager is written to this file. This file can be ";
  str << "opened and edited in a text editor." << endl << endl;

  str << "Export the contents of a folder - Press \"File\" >> \"Export Folder\". You will be asked to ";
  str << "select a new or pre-existing lookmark file. All information needed to recreate the lookmarks and/or ";
  str << "folders nested within the selected folder is written to this file." << endl << endl;

  str << "Import a lookmark file - Press \"File\" >> \"Import\" >> and either \"Append\" or \"Replace\" ";
  str << "in its cascaded menu. The first will append the contents of the imported lookmark file to the existing ";
  str << "contents of the Lookmark Manager. The latter will first remove the contents of the Lookmark Manager and ";
  str << "then import the new lookmark file." << endl << endl;

  str << "Automatic saving and loading of the contents of the Lookmark Manager - Any time you modify ";
  str << "the Lookmark Manager in some way (create or update a lookmark, move, rename, or remove items, import ";
  str << "or export lookmarks), after the modification takes place a lookmark file by the name of \"ParaViewlmk\" ";
  str << "is written to the user's home directory containing the state of the Lookmark Manager at that point in ";
  str << "time. This file is automatically imported into ParaView at the start of the session. It can be used to ";
  str << "recover your lookmarks in the event of a ParaView crash." << endl << endl;

  str << "Use a lookmark on a different dataset - A permanent folder titled \"Macros\" is located at ";
  str << "the top of the Lookmark Manager. A lookmark can be placed in here, or created manually using the \"Edit\" ";
  str << ">> \"Create Macro\" option. When invoked, this lookmark macro will be used on the currently selected dataset ";
  str << "instead of the one from which it was originally created. It will maintain your existing camera angle and timestep. ";
  str << "but recreate all other operations associated with that lookmark macro. This will work primarily only on datasets ";
  str << "with similar properties." << endl << endl;

  str << "Add existing lookmark macros to the \"Macros\" folder - Press \"Edit\" --> \"Add Existing Macro\" and select ";
  str << "from the available macros (see the following section on where ParaView gets these from). You will then see a lookmark widget ";
  str << "appear in the Macros folder by that name." << endl << endl;

  str << "Distribute pre-defined lookmark macros with ParaView - While a lookmark macro can be created in ";
  str << "the Lookmark Manager by the user, it can also be loaded automatically into the \"Edit\" --> \"Add Existing ";
  str << "Macro\" menu. This can be useful if you want to make available to users a set of \"canned\" views ";
  str << "of their data. Simply save a lookmark file of the desire lookmarks (using either \"Save As\" or \"Export Folder\") ";
  str << "to a file in your home directory named \"./LookmarkMacros\" on UNIX or \"#LookmarkMacros\" on Windows. Then, ";
  str << "when ParaView is launched, it will read from this file to populate the macros menu." << endl << endl;

  str << "Create a folder - Press \"Edit\" >> \"Create Folder\". This appends to the end of the Lookmark ";
  str << "Manager an empty folder named \"New Folder\". You can now drag lookmarks into this folder (see How to move ";
  str << "lookmarks and/or folders)." << endl << endl;

  str << "Move lookmarks and/or folders - A lookmark or folder can be moved in between any other lookmark ";
  str << "or folder in the Lookmark Manager. Simply move your mouse over the name of the item you wish to relocate ";
  str << "and hold the left mouse button down. Then drag the widget to the desired location (a box will appear under ";
  str << "your mouse if the location is a valid drop point) then release the left mouse button. Releasing over the ";
  str << "label of a folder will drop the item in the first nested entry of that folder." << endl << endl;

  str << "Remove lookmarks and/or folders - Select any combination of lookmarks and/or folders and then press ";
  str << "\"Edit\" >> \"Remove Item(s)\" button. You will be asked to verify that you wish to delete these. This prompt ";
  str << "may be turned off. " << endl << endl;

  str << "Rename a lookmark or folder - Select the lookmark or folder you wish to rename and press \"Edit\" >> ";
  str << "\"Rename Lookmark\" or \"Edit\" >> \"Rename Folder\".  This will replace the name with an editable text field ";
  str << "containing the old name. Modify the name and press the Enter/Return key. This will remove the text field and ";
  str << "replace it with a label containing the new name." << endl << endl;

  str << "Comment on a lookmark - When the contents of the Lookmark Manager are saved to a lookmark file, any ";
  str << "text you have typed in the comments area of a lookmark widget will also be saved. By default the comments frame ";
  str << "is initially collapsed (see How to expand/collapse lookmarks, folders, and the comments field)." << endl << endl;

  str << "Select lookmarks and/or folders - To select a lookmark or folder to be removed, renamed, or updated ";
  str << "(lookmarks only), checkboxes have been placed in front of each item in the Lookmark Manager. Checking a folder ";
  str << "will by default check all nested lookmarks and folders as well." << endl << endl;

  str << "Expand/collapse lookmarks, folders, and the comments field - The frames that encapsulate lookmarks, ";
  str << "folders, and the comments field can be expanded or collapsed by clicking on the \"x\" or the \"v\", respectively, ";
  str << "in the upper right hand corner of the frame. " << endl << endl;

  str << "Undo a change to the Lookmark Manager - Press \"Edit\" >> \"Undo\". This will return the Lookmark ";
  str << "Manager's contents to its state before the previous action was performed." << endl << endl;

  str << "Redo a change to the Lookmark Manager - Press \"Edit\" >> \"Redo\". This will return the Lookmark ";
  str << "Manager's contents to its state before the previous Undo was performed." << endl << endl;

  str << ends;

  this->UsersTutorialTxt->GetWidget()->SetValue( str.str() );
  str.rdbuf()->freeze(0);
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SetButtonFrameState(int state)
{
  this->CreateLmkButton->SetEnabled(state);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Checkpoint()
{

  ostrstream str;

  #ifdef _WIN32
  if ( !getenv("HOMEPATH") )
    {
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#ParaViewlmk#" << ends;
  #else
  if ( !getenv("HOME") )
    {
    return;
    }
  str << getenv("HOME") << "/.ParaViewlmk" << ends;
  #endif
   
  this->SaveAll(str.str());

  this->GetTraceHelper()->AddEntry("$kw(%s) Checkpoint",
                      this->GetTclName());

  this->MenuEdit->SetItemState("Undo", vtkKWTkOptions::StateNormal);
  this->MenuEdit->SetItemState("Redo", vtkKWTkOptions::StateDisabled);

}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RedoCallback()
{
//  this->GetTraceHelper()->AddEntry("$kw(%s) RedoCallback",
//                      this->GetTclName());

  this->UndoRedoInternal();

  this->MenuEdit->SetItemState("Redo", vtkKWTkOptions::StateDisabled);
  this->MenuEdit->SetItemState("Undo", vtkKWTkOptions::StateNormal);
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UndoCallback()
{
//  this->GetTraceHelper()->AddEntry("$kw(%s) UndoCallback",
//                      this->GetTclName());

  this->UndoRedoInternal();

  this->MenuEdit->SetItemState("Undo", vtkKWTkOptions::StateDisabled);
  this->MenuEdit->SetItemState("Redo", vtkKWTkOptions::StateNormal);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UndoRedoInternal()
{
  ostrstream str;
  ostrstream tempstr;
  FILE *infile;
  FILE *outfile;
  char buf[300];

  if(this->GetPVApplication()->GetGUIClientOptions()->GetDisableRegistry())
    {
    return;
    }

  // Get the path to the checkpointed file

  #ifndef _WIN32

  if ( !getenv("HOME") )
    {
    return;
    }
  str << getenv("HOME") << "/.ParaViewlmk" << ends;
  tempstr << getenv("HOME") << "/.TempParaViewlmk" << ends;

  #else

  if ( !getenv("HOMEPATH") )
    {
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#ParaViewlmk#" << ends;
  tempstr << "C:" << getenv("HOMEPATH") << "\\#TempParaViewlmk#" << ends;

  #endif

  ifstream checkfile(str.str());

  if ( !checkfile.fail())
    {
    // save out the current contents to a temp file
    this->SaveAll(tempstr.str());
    this->Import(str.str(),0);
//    this->Checkpoint();
    checkfile.close();
    
    //read the session state file in to a new vtkPVLookmark
    if((infile = fopen(tempstr.str(),"r")) && (outfile = fopen(str.str(),"w")))
      {
      while(fgets(buf,300,infile))
        {
        fputs(buf,outfile);
        }
      }
    fclose(infile);
    fclose(outfile);
    remove(tempstr.str());
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::AllOnOffCallback(int state)
{
  vtkIdType i,numberOfLookmarkWidgets, numberOfLookmarkFolders;
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;

  numberOfLookmarkWidgets = this->PVLookmarks->GetNumberOfItems();
  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->PVLookmarks->GetItem(i,lookmarkWidget);
    lookmarkWidget->SetSelectionState(state);
    }
  numberOfLookmarkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    lmkFolderWidget->SetSelectionState(state);
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ImportCallback()
{
  char *filename;

  this->SetButtonFrameState(0);

  if(!(filename = this->PromptForLookmarkFile(0)))
    {
    this->SetButtonFrameState(1);
    this->Script("pack %s -anchor w -fill both -side top",
                  this->LmkScrollFrame->GetWidgetName());
    this->SetButtonFrameState(1);

    return;
    }

  this->SetButtonFrameState(1);

  this->Checkpoint();

  // If "Replace" is selected we remove all preexisting lmks and containers first
  this->Import(filename,this->MenuImport->GetCheckedRadioButtonItem(this,"Import"));

}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Import(char *filename, int appendFlag)
{
  vtkXMLDataParser *parser;
  vtkXMLDataElement *root;
  vtkPVLookmark *lookmarkWidget;
  int retval;
  char msg[500];

  ifstream infile(filename);
  if ( infile.fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }

  this->GetTraceHelper()->AddEntry("$kw(%s) Import \"%s\" %d",
                      this->GetTclName(),filename,appendFlag);

  if(appendFlag==0 && (this->PVLookmarks->GetNumberOfItems()>0 || this->LmkFolderWidgets->GetNumberOfItems()>0) )
    {
    this->RemoveCheckedChildren(this->LmkScrollFrame->GetFrame(),1);
    }

  //parse the .lmk xml file and get the root node for traversing
  parser = vtkXMLDataParser::New();
  parser->SetStream(&infile);
  retval = parser->Parse();
  if(retval==0)
    {
    sprintf(msg,"Error parsing lookmark file in %s.",filename);
    this->GetPVApplication()->GetMainWindow()->ErrorMessage(msg);
    parser->Delete();
    return;
    }
  root = parser->GetRootElement();

  this->Script("[winfo toplevel %s] config -cursor watch", 
                this->GetWidgetName());

  this->ImportInternal(this->GetNumberOfChildLmkItems(this->LmkScrollFrame->GetFrame()),root,this->LmkScrollFrame->GetFrame());
  
  // after all the widgets are generated, go back thru and add d&d targets for each of them
  this->ResetDragAndDropTargetSetAndCallbacks();

  this->Script("[winfo toplevel %s] config -cursor {}", 
                this->GetWidgetName());

  this->Script("%s yview moveto 0",
               this->LmkScrollFrame->GetFrame()->GetParent()->GetWidgetName());

  // this is needed to enable the scrollbar in the lmk mgr
  this->PVLookmarks->GetItem(0,lookmarkWidget);
  if(lookmarkWidget)
    {
    this->Script("update");
    lookmarkWidget->EnableScrollBar();
    }

  infile.close();
  parser->Delete();
}



//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DragAndDropPerformCommand(int x, int y, vtkKWWidget *vtkNotUsed(widget), vtkKWWidget *vtkNotUsed(anchor))
{
  if (  vtkKWTkUtilities::ContainsCoordinates(
        this->GetApplication()->GetMainInterp(),
        this->TopDragAndDropTarget->GetWidgetName(),
        x, y))
    {
    this->TopDragAndDropTarget->SetBorderWidth(2);
    this->TopDragAndDropTarget->SetReliefToGroove();
    }
  else
    {
    this->TopDragAndDropTarget->SetBorderWidth(0);
    this->TopDragAndDropTarget->SetReliefToFlat();
    }
}

// attempt at making the after widget lookmark the target instead of the pane its packed in
void vtkPVLookmarkManager::ResetDragAndDropTargetSetAndCallbacks()
{
  vtkIdType numberOfLookmarkWidgets = this->PVLookmarks->GetNumberOfItems();
  vtkIdType numberOfLookmarkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  vtkKWLookmarkFolder *targetLmkFolder;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkPVLookmark *lookmarkWidget;
  vtkPVLookmark *targetLmkWidget;
  vtkIdType i=0;
  vtkIdType j=0;

  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->PVLookmarks->GetItem(i,lookmarkWidget);
    lookmarkWidget->GetDragAndDropTargetSet()->SetEnable(1);
    // for each lmk container
    for(j=numberOfLookmarkFolders-1;j>=0;j--)
      {
      this->LmkFolderWidgets->GetItem(j,lmkFolderWidget);
      // in this case, we'll have to get the label's grandfather to get the after widget which is the lookmarkFolder widget itself, but now the target area is limited to the label
      if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(lmkFolderWidget->GetSeparatorFrame()))
        {
        lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(lmkFolderWidget->GetSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(lmkFolderWidget->GetSeparatorFrame(), this, "DragAndDropEndCommand");
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(lmkFolderWidget->GetSeparatorFrame(), lmkFolderWidget, "DragAndDropPerformCommand");
//        lookmarkWidget->SetDragAndDropStartCommand(lmkFolderWidget->GetSeparatorFrame(), lmkFolderWidget, "DragAndDropStartCallback");
//        lookmarkWidget->SetDragAndDropStartCommand(lmkFolderWidget->GetSeparatorFrame(), this, "DragAndDropStartCallback");
        }
      if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(lmkFolderWidget->GetNestedSeparatorFrame()))
        {
        lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(lmkFolderWidget->GetNestedSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(lmkFolderWidget->GetNestedSeparatorFrame(), this, "DragAndDropEndCommand");
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(lmkFolderWidget->GetNestedSeparatorFrame(), lmkFolderWidget, "DragAndDropPerformCommand");
//        lookmarkWidget->SetDragAndDropStartCommand(lmkFolderWidget->GetNestedSeparatorFrame(), this, "DragAndDropStartCallback");
        }
      if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(lmkFolderWidget->GetLabelFrame()->GetLabel()))
        {
        lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(lmkFolderWidget->GetLabelFrame()->GetLabel());
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(lmkFolderWidget->GetLabelFrame()->GetLabel(), this, "DragAndDropEndCommand");
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(lmkFolderWidget->GetLabelFrame()->GetLabel(), lmkFolderWidget, "DragAndDropPerformCommand");
//        lookmarkWidget->SetDragAndDropStartCommand(lmkFolderWidget->GetLabelFrame()->GetLabel(), lmkFolderWidget, "DragAndDropStartCallback");
        }
      }
    // add lmk widgets as targets to this lookmark as well, they will become the "after widgets"
    for(j=numberOfLookmarkWidgets-1;j>=0;j--)
      {
      this->PVLookmarks->GetItem(j,targetLmkWidget);
      if(targetLmkWidget != lookmarkWidget)
        {
        if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkWidget->GetSeparatorFrame()))
          {
          lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkWidget->GetSeparatorFrame());
          lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkWidget->GetSeparatorFrame(), this, "DragAndDropEndCommand");
          lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkWidget->GetSeparatorFrame(), targetLmkWidget, "DragAndDropPerformCommand");
//          lookmarkWidget->SetDragAndDropStartCommand(targetLmkWidget->GetSeparatorFrame()->GetFrame(), this, "DragAndDropStartCallback");
          }
        }
      }

    // add top frame as target for the widget
    if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(this->TopDragAndDropTarget))
      {
      lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(this->TopDragAndDropTarget);
      lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(this->TopDragAndDropTarget, this, "DragAndDropEndCommand");
      lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(this->TopDragAndDropTarget, this, "DragAndDropPerformCommand");
//      lookmarkWidget->SetDragAndDropStartCommand(this->TopDragAndDropTarget->GetFrame(), this->TopDragAndDropTarget->GetFrame(), "DragAndDropStartCallback");
      }

    }
  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetMacroFlag())
      {
      continue;
      }
    lmkFolderWidget->GetDragAndDropTargetSet()->SetEnable(1);
    // must check to see if the widgets are descendants of this container widget if so dont add as target
    // for each lmk container, add its internal frame as target to this
    for(j=numberOfLookmarkFolders-1;j>=0;j--)
      {
      this->LmkFolderWidgets->GetItem(j,targetLmkFolder);
      if(targetLmkFolder!=lmkFolderWidget && !this->IsWidgetInsideFolder(targetLmkFolder,lmkFolderWidget))
        {
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkFolder->GetSeparatorFrame()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkFolder->GetSeparatorFrame());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkFolder->GetSeparatorFrame(), this, "DragAndDropEndCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkFolder->GetSeparatorFrame(), targetLmkFolder, "DragAndDropPerformCommand");
//          lmkFolderWidget->SetDragAndDropStartCommand(targetLmkFolder->GetSeparatorFrame()->GetFrame(), this, "DragAndDropStartCallback");
          }
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkFolder->GetNestedSeparatorFrame()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkFolder->GetNestedSeparatorFrame());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkFolder->GetNestedSeparatorFrame(), this, "DragAndDropEndCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkFolder->GetNestedSeparatorFrame(), targetLmkFolder, "DragAndDropPerformCommand");
//          lmkFolderWidget->SetDragAndDropStartCommand(targetLmkFolder->GetNestedSeparatorFrame(), this, "DragAndDropStartCallback");
          }
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkFolder->GetLabelFrame()->GetLabel()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkFolder->GetLabelFrame()->GetLabel());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkFolder->GetLabelFrame()->GetLabel(), this, "DragAndDropEndCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkFolder->GetLabelFrame()->GetLabel(), targetLmkFolder, "DragAndDropPerformCommand");
          }
        }
      }
    // add lmk widgets as targets to this lookmark as well, they will become the "after widgets"
    for(j=numberOfLookmarkWidgets-1;j>=0;j--)
      {
      this->PVLookmarks->GetItem(j,targetLmkWidget);
      if(!this->IsWidgetInsideFolder(targetLmkWidget,lmkFolderWidget))
        {
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkWidget->GetSeparatorFrame()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkWidget->GetSeparatorFrame());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkWidget->GetSeparatorFrame(), targetLmkWidget, "DragAndDropPerformCommand");
//          lmkFolderWidget->SetDragAndDropStartCommand(targetLmkWidget->GetSeparatorFrame()->GetFrame(), this, "DragAndDropStartCallback");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkWidget->GetSeparatorFrame(), this, "DragAndDropEndCommand");
          }
        }
      }

    // add top frame as target for the widget
    if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(this->TopDragAndDropTarget))
      {
      lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(this->TopDragAndDropTarget);
      lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(this->TopDragAndDropTarget, this, "DragAndDropEndCommand");
      lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(this->TopDragAndDropTarget, this, "DragAndDropPerformCommand");
      }
    }
}

//---------------------------------------------------------------------------
int vtkPVLookmarkManager::IsWidgetInsideFolder(vtkKWWidget *lmkItem,vtkKWWidget *parent)
{
  int ret = 0;

  if(parent==lmkItem)
    {
    ret=1;
    }
  else
    {
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      if (this->IsWidgetInsideFolder(lmkItem,parent->GetNthChild(i)))
        {
        ret = 1;
        break;
        }
      }
    }
  return ret;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DragAndDropEndCommand( int vtkNotUsed(x), int vtkNotUsed(y), vtkKWWidget *widget, vtkKWWidget *vtkNotUsed(anchor), vtkKWWidget *target)
{
  // guaranteed that this will be the only target callback called because the targets in the lookmark manager
  // are mutually exclusive. "target" will either be the vtkKWLabel of the folder widget or the entire lookmarkWidget frame
  // in the case of the lookmark widget. 

  // enhancement: take the x,y, coords, loop through lookmarks and folders, and see which contain them

  vtkPVLookmark *lmkWidget;
  vtkKWLookmarkFolder *lmkFolder;

  // the target will always be a vtkKWFrame but it might be after the lmkitems label frame or nested within the its internal frame if the lmkitem is a lmkcontainer

  if((lmkFolder = vtkKWLookmarkFolder::SafeDownCast(target->GetParent())))
    {
    this->DragAndDropWidget(widget, lmkFolder);
    this->PackChildrenBasedOnLocation(lmkFolder->GetParent());
    lmkFolder->RemoveDragAndDropTargetCues();
    }
  else if((lmkFolder = vtkKWLookmarkFolder::SafeDownCast(target->GetParent()->GetParent()->GetParent()->GetParent()->GetParent())))
    {
    //target is either a folder's nested separator frame or its label, in both cases we drop in the folder's first slot
    this->DragAndDropWidget(widget, lmkFolder->GetNestedSeparatorFrame());
    this->PackChildrenBasedOnLocation(lmkFolder->GetLabelFrame()->GetFrame());
    lmkFolder->RemoveDragAndDropTargetCues();
    }
  else if( (lmkWidget = vtkPVLookmark::SafeDownCast(target->GetParent())))
    {
    this->DragAndDropWidget(widget, lmkWidget);
    this->PackChildrenBasedOnLocation(lmkWidget->GetParent());
    lmkWidget->RemoveDragAndDropTargetCues();

    }
  else if(target==this->TopDragAndDropTarget)
    {
    this->DragAndDropWidget(widget, this->TopDragAndDropTarget);
    this->PackChildrenBasedOnLocation(this->TopDragAndDropTarget->GetParent());
    this->TopDragAndDropTarget->SetBorderWidth(0);
    this->TopDragAndDropTarget->SetReliefToFlat();
    }

  this->DestroyUnusedLmkWidgets(this->LmkScrollFrame);

  this->ResetDragAndDropTargetSetAndCallbacks();

  // this is needed to enable the scrollbar in the lmk mgr
  this->PVLookmarks->GetItem(0,lmkWidget);
  if(lmkWidget)
    {
    lmkWidget->EnableScrollBar();
    }
}


//----------------------------------------------------------------------------
int vtkPVLookmarkManager::DragAndDropWidget(vtkKWWidget *widget,vtkKWWidget *AfterWidget)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }

  this->Checkpoint();


  // renumber location vars of siblings widget is leaving
  // handle case when moving within same level
  int oldLoc;
  vtkPVLookmark *lmkWidget;
  vtkPVLookmark *afterLmkWidget;
  vtkKWLookmarkFolder *afterLmkFolder;
  vtkKWLookmarkFolder *lmkFolder;
  vtkKWWidget *dstPrnt;
  vtkIdType loc;

  if((lmkWidget = vtkPVLookmark::SafeDownCast(widget)))
    {
    if(!this->PVLookmarks->IsItemPresent(lmkWidget))
      return 0;

    oldLoc = lmkWidget->GetLocation();
    lmkWidget->SetLocation(-1);
    this->DecrementHigherSiblingLmkItemLocationIndices(widget->GetParent(),oldLoc);

    int newLoc;
    if((afterLmkWidget = vtkPVLookmark::SafeDownCast(AfterWidget)))
      {
      newLoc = afterLmkWidget->GetLocation()+1;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }
    else if((afterLmkFolder = vtkKWLookmarkFolder::SafeDownCast(AfterWidget)))
      {
      newLoc = afterLmkFolder->GetLocation()+1;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }
    else
      {
      newLoc = 0;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }

    vtkPVLookmark *newLmkWidget = vtkPVLookmark::New();
    newLmkWidget->SetMacroFlag(this->IsWidgetInsideFolder(dstPrnt,this->GetMacrosFolder()));
    if(lmkWidget->GetMacroFlag())
      {
      this->GetPVApplication()->GetMainWindow()->GetLookmarkToolbar()->RemoveWidget(lmkWidget->GetToolbarButton());
      }
    lmkWidget->UpdateVariableValues();
    newLmkWidget->SetParent(dstPrnt);
    newLmkWidget->Create(this->GetPVApplication());
    char methodAndArg[200];
    sprintf(methodAndArg,"SelectItemCallback %s",newLmkWidget->GetWidgetName());
    newLmkWidget->GetCheckbox()->SetCommand(this,methodAndArg);
    newLmkWidget->SetName(lmkWidget->GetName());
    newLmkWidget->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
    ostrstream s;
    s << "GetPVLookmark \"" << newLmkWidget->GetName() << "\"" << ends;
    newLmkWidget->GetTraceHelper()->SetReferenceCommand(s.str());
    s.rdbuf()->freeze(0);
//ds
    newLmkWidget->SetDataset(lmkWidget->GetDataset());
    newLmkWidget->CreateDatasetList();
    newLmkWidget->SetLocation(newLoc);
    newLmkWidget->SetComments(lmkWidget->GetComments());
    newLmkWidget->UpdateWidgetValues();
    newLmkWidget->CommentsModifiedCallback();
    newLmkWidget->SetImageData(lmkWidget->GetImageData());
    newLmkWidget->SetPixelSize(lmkWidget->GetPixelSize());
    newLmkWidget->CreateIconFromImageData();
/*
    this->SetImageWidth(lmkIcon->GetWidth());
    this->SetImageHeight(lmkIcon->GetHeight());
    this->SetImagePixelSize(lmkIcon->GetPixelSize());
*/
    newLmkWidget->SetStateScript(lmkWidget->GetStateScript());
    this->Script("pack %s -fill both -expand yes -padx 8",newLmkWidget->GetWidgetName());

    this->PVLookmarks->FindItem(lmkWidget,loc);
    this->PVLookmarks->RemoveItem(loc);
    this->PVLookmarks->InsertItem(loc,newLmkWidget);

    this->RemoveItemAsDragAndDropTarget(lmkWidget);
    this->Script("destroy %s", lmkWidget->GetWidgetName());
    lmkWidget->Delete();
    }
  else if((lmkFolder = vtkKWLookmarkFolder::SafeDownCast(widget)))
    {
    if(!this->LmkFolderWidgets->IsItemPresent(lmkFolder))
      return 0;

    oldLoc = lmkFolder->GetLocation();
    lmkFolder->SetLocation(-1);
    this->DecrementHigherSiblingLmkItemLocationIndices(widget->GetParent(),oldLoc);
    int newLoc;
    if((afterLmkWidget = vtkPVLookmark::SafeDownCast(AfterWidget)))
      {
      newLoc = afterLmkWidget->GetLocation()+1;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }
    else if((afterLmkFolder = vtkKWLookmarkFolder::SafeDownCast(AfterWidget)))
      {
      newLoc = afterLmkFolder->GetLocation()+1;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }
    else
      {
      newLoc = 0;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }

    vtkKWLookmarkFolder *newLmkFolder = vtkKWLookmarkFolder::New();
    newLmkFolder->SetMacroFlag(lmkFolder->GetMacroFlag());
    newLmkFolder->SetParent(dstPrnt);
    newLmkFolder->Create(this->GetPVApplication());
    char methodAndArg[200];
    sprintf(methodAndArg,"SelectItemCallback %s",newLmkFolder->GetWidgetName());
    newLmkFolder->GetCheckbox()->SetCommand(this,methodAndArg);
    newLmkFolder->SetFolderName(lmkFolder->GetLabelFrame()->GetLabel()->GetText());
    newLmkFolder->SetLocation(newLoc);

    this->Script("pack %s -fill both -expand yes -padx 8",newLmkFolder->GetWidgetName());
    this->LmkFolderWidgets->FindItem(lmkFolder,loc);
    this->LmkFolderWidgets->RemoveItem(loc);
    this->LmkFolderWidgets->InsertItem(loc,newLmkFolder);

    //loop through all children to this container's LabeledFrame

    vtkKWWidget *parent = lmkFolder->GetLabelFrame()->GetFrame();
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->MoveCheckedChildren(
        parent->GetNthChild(i), 
        newLmkFolder->GetLabelFrame()->GetFrame());
      }

// need to delete the source folder
    this->RemoveItemAsDragAndDropTarget(lmkFolder);
    this->Script("destroy %s", lmkFolder->GetWidgetName());
    lmkFolder->Delete();
    }

  return 1;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ImportInternal(int locationOfLmkItemAmongSiblings, vtkXMLDataElement *lmkElement, vtkKWWidget *parent)
{
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType j,numLmks, numFolders;

  if(!strcmp("LmkFolder",lmkElement->GetName()))
    {
    lmkFolderWidget = vtkKWLookmarkFolder::New();
    lmkFolderWidget->SetParent(parent);
    if(!strcmp(lmkElement->GetAttribute("Name"),"Macros"))
      {
      lmkFolderWidget->SetMacroFlag(1);
      }
 //   lmkFolderWidget->SetParent(this->LmkScrollFrame->GetFrame());
    lmkFolderWidget->Create(this->GetPVApplication());
    char methodAndArg[200];
    sprintf(methodAndArg,"SelectItemCallback %s",lmkFolderWidget->GetWidgetName());
    lmkFolderWidget->GetCheckbox()->SetCommand(this,methodAndArg);
    this->Script("pack %s -fill both -expand yes -padx 8",lmkFolderWidget->GetWidgetName());
    lmkFolderWidget->SetFolderName(lmkElement->GetAttribute("Name"));
    lmkFolderWidget->SetLocation(locationOfLmkItemAmongSiblings);

    numFolders = this->LmkFolderWidgets->GetNumberOfItems();
    this->LmkFolderWidgets->InsertItem(numFolders,lmkFolderWidget);
    
    // use the label frame of this lmk container as the parent frame in which to pack into (constant)
    // for each xml element (either lookmark or lookmark container) recursively call import with the appropriate location and vtkXMLDataElement
    for(j=0; j<lmkElement->GetNumberOfNestedElements(); j++)
      {
      ImportInternal(j,lmkElement->GetNestedElement(j),lmkFolderWidget->GetLabelFrame()->GetFrame());
      }
    }
  else if(!strcmp("LmkFile",lmkElement->GetName()))
    {
    // in this case locationOfLmkItemAmongSiblings is the number of lookmark element currently in the first level of the lookmark manager which is why we start from that index
    // the parent is the lookmark manager's label frame
    for(j=0; j<lmkElement->GetNumberOfNestedElements(); j++)
      {
      ImportInternal(j+locationOfLmkItemAmongSiblings,lmkElement->GetNestedElement(j),this->LmkScrollFrame->GetFrame());
      }
    }
  else if(!strcmp("Lmk",lmkElement->GetName()))
    {
    // note that in the case of a lookmark, no recursion is done

    // this uses a vtkXMLLookmarkElement to create a vtkPVLookmark object
    // create lookmark widget
    lookmarkWidget = this->GetPVLookmark(lmkElement);
    lookmarkWidget->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
    ostrstream s;
    s << "GetPVLookmark \"" << lookmarkWidget->GetName() << "\"" << ends;
    lookmarkWidget->GetTraceHelper()->SetReferenceCommand(s.str());
    s.rdbuf()->freeze(0);
    vtkKWLookmarkFolder *folder = this->GetMacrosFolder();
    if(folder)
      {
      lookmarkWidget->SetMacroFlag(this->IsWidgetInsideFolder(parent,this->GetMacrosFolder()));
      }
    lookmarkWidget->SetParent(parent);
    lookmarkWidget->Create(this->GetPVApplication());
    char methodAndArg[200];
    sprintf(methodAndArg,"SelectItemCallback %s",lookmarkWidget->GetWidgetName());
    lookmarkWidget->GetCheckbox()->SetCommand(this,methodAndArg);
    lookmarkWidget->UpdateWidgetValues();
    lookmarkWidget->CommentsModifiedCallback();
    this->Script("pack %s -fill both -expand yes -padx 8",lookmarkWidget->GetWidgetName());

    lookmarkWidget->CreateIconFromImageData();
//    lookmarkWidget->SetImageData(lmkElement->GetAttribute("ImageData"));
    lookmarkWidget->SetLocation(locationOfLmkItemAmongSiblings);

    numLmks = this->PVLookmarks->GetNumberOfItems();
    this->PVLookmarks->InsertItem(numLmks,lookmarkWidget);
    }
}


//----------------------------------------------------------------------------
char* vtkPVLookmarkManager::PromptForLookmarkFile(int saveFlag)
{
  ostrstream str;
  vtkKWLoadSaveDialog* dialog = vtkKWLoadSaveDialog::New();
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  char *tempName = new char[100]; 

  if(saveFlag)
    dialog->SaveDialogOn();

  dialog->Create(this->GetPVApplication());

  if (win)
    {
    dialog->SetParent(this->LmkScrollFrame);
    }
  dialog->SetDefaultExtension(".lmk");
  str << "{{} {.lmk} } ";
  str << "{{All files} {*}}" << ends;
  dialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);

  if(!dialog->Invoke())
    {
    dialog->Delete();
    return 0;
    }

  this->Focus();


  dialog->Delete();
  delete [] tempName;

  return dialog->GetFileName();
  
}


//----------------------------------------------------------------------------
vtkPVLookmark *vtkPVLookmarkManager::GetPVLookmark(char *name)
{
  vtkPVLookmark *lookmarkWidget;
  vtkIdType numLmkWidgets = this->PVLookmarks->GetNumberOfItems();
  for(int i=numLmkWidgets-1;i>=0;i--)
    {
    this->PVLookmarks->GetItem(i,lookmarkWidget);
    if(!strcmp(lookmarkWidget->GetName(),name))
      {
      return lookmarkWidget;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkPVLookmark *vtkPVLookmarkManager::GetPVLookmark(vtkXMLDataElement *elem)
{
  vtkPVLookmark *lmk = vtkPVLookmark::New();

  char *lookmarkName = new char[strlen(elem->GetAttribute("Name"))+1]; 
  strcpy(lookmarkName,elem->GetAttribute("Name"));
  lmk->SetName(lookmarkName);
  delete [] lookmarkName;

  if(elem->GetAttribute("Comments"))
    {
    char *lookmarkComments = new char[strlen(elem->GetAttribute("Comments"))+1];
    strcpy(lookmarkComments,elem->GetAttribute("Comments"));
    this->DecodeNewlines(lookmarkComments);
    lmk->SetComments(lookmarkComments);
    delete [] lookmarkComments;
    }
  
  if(elem->GetAttribute("StateScript"))
    {
    char *lookmarkScript = new char[strlen(elem->GetAttribute("StateScript"))+1];
    strcpy(lookmarkScript,elem->GetAttribute("StateScript"));
    this->DecodeNewlines(lookmarkScript);
    lmk->SetStateScript(lookmarkScript);
    delete [] lookmarkScript;
    }
//ds
  if(elem->GetAttribute("Dataset"))
    {
    char *lookmarkDataset = new char[strlen(elem->GetAttribute("Dataset"))+1];
    strcpy(lookmarkDataset,elem->GetAttribute("Dataset"));
    lmk->SetDataset(lookmarkDataset);
    lmk->CreateDatasetList();
    delete [] lookmarkDataset;
    }
 
  if(elem->GetAttribute("ImageData"))
    {
    char *lookmarkImage = new char[strlen(elem->GetAttribute("ImageData"))+1];
    strcpy(lookmarkImage,elem->GetAttribute("ImageData"));
    lmk->SetImageData(lookmarkImage);
    delete [] lookmarkImage;
    }

  if(elem->GetAttribute("PixelSize"))
    {
    int lookmarkPixelSize = 0;
    elem->GetScalarAttribute("PixelSize",lookmarkPixelSize);
    lmk->SetPixelSize(lookmarkPixelSize);
    }
  else
    {
    // if there is not PixelSize attribute, then it is an old lookmark file and the pixel size was 4 (the default is now 3)
    lmk->SetPixelSize(4);
    }
 
  double centerOfRotation[3];
  elem->GetScalarAttribute("XCenterOfRotation",centerOfRotation[0]);
  elem->GetScalarAttribute("YCenterOfRotation",centerOfRotation[1]);
  elem->GetScalarAttribute("ZCenterOfRotation",centerOfRotation[2]);
  lmk->SetCenterOfRotation(centerOfRotation[0],centerOfRotation[1],centerOfRotation[2]);

  return lmk;
}


//----------------------------------------------------------------------------
vtkPVLookmark *vtkPVLookmarkManager::GetPVLookmark(vtkPVXMLElement *elem)
{
  vtkPVLookmark *lmk = vtkPVLookmark::New();

  char *lookmarkName = new char[strlen(elem->GetAttribute("Name"))+1]; 
  strcpy(lookmarkName,elem->GetAttribute("Name"));
  lmk->SetName(lookmarkName);
  delete [] lookmarkName;

  if(elem->GetAttribute("Comments"))
    {
    char *lookmarkComments = new char[strlen(elem->GetAttribute("Comments"))+1];
    strcpy(lookmarkComments,elem->GetAttribute("Comments"));
    this->DecodeNewlines(lookmarkComments);
    lmk->SetComments(lookmarkComments);
    delete [] lookmarkComments;
    }
  
  if(elem->GetAttribute("StateScript"))
    {
    char *lookmarkScript = new char[strlen(elem->GetAttribute("StateScript"))+1];
    strcpy(lookmarkScript,elem->GetAttribute("StateScript"));
    this->DecodeNewlines(lookmarkScript);
    lmk->SetStateScript(lookmarkScript);
    delete [] lookmarkScript;
    }
//ds
  if(elem->GetAttribute("Dataset"))
    {
    char *lookmarkDataset = new char[strlen(elem->GetAttribute("Dataset"))+1];
    strcpy(lookmarkDataset,elem->GetAttribute("Dataset"));
    lmk->SetDataset(lookmarkDataset);
    lmk->CreateDatasetList();
    delete [] lookmarkDataset;
    }

  if(elem->GetAttribute("ImageData"))
    {
    char *lookmarkImage = new char[strlen(elem->GetAttribute("ImageData"))+1];
    strcpy(lookmarkImage,elem->GetAttribute("ImageData"));
    lmk->SetImageData(lookmarkImage);
    delete [] lookmarkImage;
    }

  if(elem->GetAttribute("PixelSize"))
    {
    int lookmarkPixelSize = 0;
    elem->GetScalarAttribute("PixelSize",&lookmarkPixelSize);
    lmk->SetPixelSize(lookmarkPixelSize);
    }
  else
    {
    // if there is not PixelSize attribute, then it is an old lookmark file and the pixel size was 4 (the default is now 3)
    lmk->SetPixelSize(4);
    }
 
  double centerOfRotation[3];
  elem->GetScalarAttribute("XCenterOfRotation",&centerOfRotation[0]);
  elem->GetScalarAttribute("YCenterOfRotation",&centerOfRotation[1]);
  elem->GetScalarAttribute("ZCenterOfRotation",&centerOfRotation[2]);
  lmk->SetCenterOfRotation(centerOfRotation[0],centerOfRotation[1],centerOfRotation[2]);

  return lmk;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::EncodeNewlines(char *string)
{
  int i;
  int len = strlen(string);
  for(i=0;i<len;i++)
    {
    if(string[i]=='\n')
      {
      string[i]='~';
      }
    } 
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DecodeNewlines(char *string)
{
  int i;
  int len = strlen(string);
  for(i=0;i<len;i++)
    {
    if(string[i]=='~')
      {
      string[i]='\n';
      }
    } 
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UpdateLookmarkCallback()
{
  vtkPVLookmark *lookmarkWidget;
  vtkIdType numLmkWidgets,lmkIndex;
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  int numChecked = 0;

  // called before any change to the lookmark manager

  numLmkWidgets = this->PVLookmarks->GetNumberOfItems();

  for(lmkIndex=0; lmkIndex<numLmkWidgets; lmkIndex++)
    {
    this->PVLookmarks->GetItem(lmkIndex,lookmarkWidget);
    if(lookmarkWidget->GetSelectionState())
      numChecked++;
    }
  if(numChecked==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "No Lookmark Selected", 
      "To update a lookmark with a new view, first select only one lookmark by checking its box. Then  go to \"Edit\" --> \"Update Lookmark\".", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }
  else if(numChecked > 1)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "Multiple Lookmarks Selected", 
      "To update a lookmark with a new view, first select only one lookmark by checking its box. Then  go to \"Edit\" --> \"Update Lookmark\".", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }

  this->Checkpoint();

  // if no lookmarks are selected, display dialog
  // if more than one are selected, ask which one
  for(lmkIndex=0; lmkIndex<numLmkWidgets; lmkIndex++)
    {
    this->PVLookmarks->GetItem(lmkIndex,lookmarkWidget);

    if(lookmarkWidget->GetSelectionState())
      {
      lookmarkWidget->Update();
      lookmarkWidget->SetSelectionState(0);
      break;
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::CreateLookmarkCallback(int macroFlag)
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  // if the pipeline is empty, don't add
  if(win->GetSourceList("Sources")->GetNumberOfItems()==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "No Data Loaded", 
      "To create a lookmark you must first open your data and view some feature of interest. Then press \"Create Lookmark\" in either the main window or in the \"Edit\" menu.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }

  this->CreateLookmark(this->GetUnusedLookmarkName(), macroFlag);
  
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::CreateLookmark(char *name, int macroFlag)
{
  vtkIdType numLmkWidgets = this->PVLookmarks->GetNumberOfItems();
  vtkPVLookmark *newLookmark;
  vtkPVReaderModule *mod;
  int indexOfNewLmkWidget;
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  this->GetTraceHelper()->AddEntry("$kw(%s) CreateLookmark \"%s\" %d",
                      this->GetTclName(),name,macroFlag);

  // what if the main window is not maximized in screen? 

  this->Checkpoint();

  // create and initialize pvlookmark:
  newLookmark = vtkPVLookmark::New();
  // all new lmk widgets get appended to end of lmk mgr thus its parent is the LmkListingFrame:
  if(macroFlag)
    {
    newLookmark->SetParent(this->GetMacrosFolder()->GetLabelFrame()->GetFrame());
    } 
  else
    {
    newLookmark->SetParent(this->LmkScrollFrame->GetFrame());
    }
  newLookmark->SetMacroFlag(macroFlag);
  newLookmark->Create(this->GetPVApplication());
  char methodAndArg[200];
  sprintf(methodAndArg,"SelectItemCallback %s",newLookmark->GetWidgetName());
  newLookmark->GetCheckbox()->SetCommand(this,methodAndArg);
  newLookmark->SetName(name);
  newLookmark->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
  ostrstream s;
  s << "GetPVLookmark \"" << newLookmark->GetName() << "\"" << ends;
  newLookmark->GetTraceHelper()->SetReferenceCommand(s.str());
  s.rdbuf()->freeze(0);
  newLookmark->SetCenterOfRotation(win->GetCenterOfRotationStyle()->GetCenter());
//ds
  //find the reader to use by getting the reader of the current pvsource
  // Loop though all sources/Data objects and compute total bounds.
  vtkPVSourceCollection* col = win->GetSourceList("Sources");
  vtkPVSource *pvs;
  if (col == NULL)
    {
    return;
    }
  vtkCollectionIterator *it = col->NewIterator();
//  char *ds = new char[1];
//  ds[0] = '\0';
  vtkStdString ds;
  char *path;
  int i=0;
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>( it->GetCurrentObject() );
    if(!pvs->GetPVInput(0))
      {
      if(this->IsSourceOrOutputsVisible(pvs,pvs->GetVisibility()))
        {
        if(pvs->IsA("vtkPVReaderModule"))
          {
          mod = vtkPVReaderModule::SafeDownCast(pvs);
          path = (char *)mod->GetFileEntry()->GetValue();
          }
        else
          {
          path = pvs->GetModuleName();
          }
        //ds = (char*)realloc((char*)ds,strlen(ds)+strlen(path)+1);
        ds.append(path);
        ds.append(";");
        //sprintf(ds,"%s%s;",ds,path);
        i++;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();
  ds.erase(ds.find_last_of(';',ds.size()));
  newLookmark->SetDataset(ds.c_str());
  newLookmark->CreateDatasetList();
  newLookmark->StoreStateScript();
  newLookmark->UpdateWidgetValues();
  newLookmark->CommentsModifiedCallback();
  this->Script("pack %s -fill both -expand yes -padx 8",newLookmark->GetWidgetName());

  // since the direct children of the LmkListingFrame will always be either lmk widgets or containers
  // counting them will give us the appropriate location to assign the new lmk:
  indexOfNewLmkWidget = this->GetNumberOfChildLmkItems(this->LmkScrollFrame->GetFrame());
  newLookmark->SetLocation(indexOfNewLmkWidget);
  newLookmark->CreateIconFromMainView();

  this->PVLookmarks->InsertItem(numLmkWidgets,newLookmark);

  this->ResetDragAndDropTargetSetAndCallbacks();

  this->Script("update");

//   Try to get the scroll bar to initialize properly (show correct portion).
  this->Script("%s yview moveto 1",
               this->LmkScrollFrame->GetFrame()->GetParent()->GetWidgetName());

}

//-----------------------------------------------------------------------------
int vtkPVLookmarkManager::IsSourceOrOutputsVisible(vtkPVSource* source, int visibilityFlag)
{
  int ret = 0;
  if ( !source )
    {
    return visibilityFlag;
    }
  if(visibilityFlag)
    {
    return visibilityFlag;
    }
  for(int i=0; i<source->GetNumberOfPVConsumers();i++)
    {
    vtkPVSource* consumer = source->GetPVConsumer(i);
    if ( consumer )
      {
      ret = this->IsSourceOrOutputsVisible(consumer, consumer->GetVisibility());
      }
    }
  return ret | source->GetVisibility();
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveAllCallback()
{
  char *filename;
  ostrstream str;

//  this->Checkpoint();

  this->SetButtonFrameState(0);

  if(!(filename = this->PromptForLookmarkFile(1)))
    {
    this->SetButtonFrameState(1);
    return;
    }

  this->GetTraceHelper()->AddEntry("$kw(%s) SaveAll \"%s\"",
                      this->GetTclName(),filename);

#ifndef _WIN32
  if ( !getenv("HOME") )
    {
    return;
    }
  str << getenv("HOME") << "/.ParaViewlmk" << ends;
#else
  if ( !getenv("HOMEPATH") )
    {
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#ParaViewlmk#" << ends;
#endif

  if(!strcmp(filename,str.str()))
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Cannot Save to Application Lookmark File", 
      "Please select a different lookmark file to save to. The one you have chosen is restricted for use by the ParaView application.",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }
  this->SaveAll(filename);

  this->SetButtonFrameState(1);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ExportFolderCallback()
{
  char *filename;
  ostrstream str;
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  int numChecked = 0;
  vtkKWLookmarkFolder *rootFolder = NULL;

  int errorFlag = 0;
//  this->Checkpoint();

  vtkIdType numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      numChecked++;
      }
    }

  if(numChecked==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "No Folders Selected", 
      "To export a folder of lookmarks to a lookmark file, first select a folder by checking its box. Then go to \"File\" --> \"Export Folder\"",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

  this->SetButtonFrameState(0);

  if(!(filename = this->PromptForLookmarkFile(1)))
    {
    this->SetButtonFrameState(1);
    return;
    }


#ifndef _WIN32
  if ( !getenv("HOME") )
    {
    this->SetButtonFrameState(1);
    return;
    }
  str << getenv("HOME") << "/.ParaViewlmk" << ends;
#else
  if ( !getenv("HOMEPATH") )
    {
    this->SetButtonFrameState(1);
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#ParaViewlmk#" << ends;
#endif

  if(!strcmp(filename,str.str()))
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Cannot Save to Application Lookmark File", 
      "Please select a different lookmark file to save to. The one you have chosen is restricted for use by the ParaView application.",
      vtkKWMessageDialog::ErrorIcon);
    this->SetButtonFrameState(1);
    return;
    }


  // increment thru folders until we find one that is selected
  // then check to see if each subsequent selected folder is a descendant of it
  // if it is not, check to see if the stored folder is a descendant of it
  // if this is not the case either, we have multiple folders selected
  numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      if(rootFolder==NULL)
        {
        rootFolder = lmkFolderWidget;
        }
      else if(this->IsWidgetInsideFolder(rootFolder,lmkFolderWidget))
        {
        errorFlag = 0;
        rootFolder = lmkFolderWidget;
        }
      else if(!this->IsWidgetInsideFolder(lmkFolderWidget,rootFolder) && rootFolder->GetParent()==lmkFolderWidget->GetParent())
        {
        errorFlag = 1;
        }
      else
        {
        errorFlag = 1;
        break;
        }
      }
    }

  if(errorFlag)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Folders Selected", 
      "To export a folder of lookmarks to a lookmark file, first select a folder by checking its box. Then go to \"File\" --> \"Export Folder\"",
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    this->SetButtonFrameState(1);

    return;
    }

  if(rootFolder)
    {
    //make sure all selected lookmarks are inside folder, if so, we can rename this folder
    vtkIdType numLmkWidgets = this->PVLookmarks->GetNumberOfItems();
    for(i=numLmkWidgets-1;i>=0;i--)
      {
      this->PVLookmarks->GetItem(i,lookmarkWidget);
      if(lookmarkWidget->GetSelectionState()==1)
        {
        if(!this->IsWidgetInsideFolder(lookmarkWidget,rootFolder))
          {
          vtkKWMessageDialog::PopupMessage(
            this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Lookmarks and Folders Selected", 
            "To export a folder of lookmarks to a lookmark file, first select a folder by checking its box. Then go to \"File\" --> \"Export Folder\"",
            vtkKWMessageDialog::ErrorIcon);
          this->Focus();
          this->SetButtonFrameState(1);

          return;
          }
        }
      }
    this->SaveFolderInternal(filename,rootFolder);
    }

  this->SetButtonFrameState(1);

  vtkIdType numberOfLookmarkWidgets, numberOfLookmarkFolders;
  numberOfLookmarkWidgets = this->PVLookmarks->GetNumberOfItems();
  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->PVLookmarks->GetItem(i,lookmarkWidget);
    lookmarkWidget->SetSelectionState(0);
    }
  numberOfLookmarkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    lmkFolderWidget->SetSelectionState(0);
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveAll(char *filename)
{
  ifstream *infile;
  ofstream *outfile;
  vtkXMLLookmarkElement *root;
  vtkXMLDataParser *parser;
  int retval;
  char msg[500];

  if(this->GetPVApplication()->GetGUIClientOptions()->GetDisableRegistry())
    {
    return;
    }
  
  // write out an empty lookmark file so that the parser will not complain
  outfile = new ofstream(filename,ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    delete outfile;
    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    delete outfile;
    return;
    }

  *outfile << "<LmkFile></LmkFile>";
  outfile->close();
  delete outfile;

  infile = new ifstream(filename);
  if ( !infile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    delete infile;
    return;
    }
  if ( infile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    delete infile;
    return;
    }

  parser = vtkXMLDataParser::New();
  parser->SetStream(infile);
  retval = parser->Parse();
  if(retval==0)
    {
    sprintf(msg,"Error parsing lookmark file in %s.",filename);
    this->GetPVApplication()->GetMainWindow()->ErrorMessage(msg);
    parser->Delete();
    return;
    } 
  root = (vtkXMLLookmarkElement *)parser->GetRootElement();

  this->CreateNestedXMLElements(this->LmkScrollFrame->GetFrame(),root);

  infile->close();
  outfile = new ofstream(filename,ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    delete infile;
    delete outfile;
    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    delete infile;
    delete outfile;
    return;
    }

  root->PrintXML(*outfile,vtkIndent(1));
  outfile->close();
  parser->Delete();

  delete infile;
  delete outfile;
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveFolderInternal(char *filename, vtkKWLookmarkFolder *folder)
{
  ifstream *infile;
  ofstream *outfile;
  vtkXMLLookmarkElement *root;
  vtkXMLDataParser *parser;
  int retval;
  char msg[500];
  
  // write out an empty lookmark file so that the parser will not complain
  outfile = new ofstream(filename,ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }

  *outfile << "<LmkFile></LmkFile>";
  outfile->close();

  infile = new ifstream(filename);
  if ( !infile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }
  if ( infile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }

  parser = vtkXMLDataParser::New();
  parser->SetStream(infile);
  retval = parser->Parse();
  if(retval==0)
    {
    sprintf(msg,"Error parsing lookmark file in %s.",filename);
    this->GetPVApplication()->GetMainWindow()->ErrorMessage(msg);
    parser->Delete();
    delete infile;
    delete outfile;
    return;
    } 
  root = (vtkXMLLookmarkElement *)parser->GetRootElement();

//  this->CreateNestedXMLElements(folder->GetLabelFrame()->GetFrame()->GetFrame(),root);

  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;

  int nextLmkItemIndex=0;
  int counter=0;

  // loop through the children numberOfChildren times
  // if we come across a lmk item whose packed location among its siblings is the next one we're looking for
  //   recurse and break out of the inner loop, init traversal of children and repeat
  //   
  // the two loops are necessary because the user can change location of lmk items and
  // the vtkKWWidgetCollection of children is ordered by when the item was created, not reordered each time its packed (moved)

  vtkKWWidget *parent = folder->GetLabelFrame()->GetFrame();

  while (counter < parent->GetNumberOfChildren())
    {
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      vtkKWWidget *child = parent->GetNthChild(i);
      if(child->IsA("vtkKWLookmark"))
        {
        lookmarkWidget = vtkPVLookmark::SafeDownCast(child);
        if(this->PVLookmarks->IsItemPresent(lookmarkWidget))
          {
          if(lookmarkWidget->GetLocation()==nextLmkItemIndex)
            {
            this->CreateNestedXMLElements(lookmarkWidget,root);
            nextLmkItemIndex++;
            break;
            }
          }
        }
      else if(child->IsA("vtkKWLookmarkFolder"))
        {
        lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(child);
        if(this->LmkFolderWidgets->IsItemPresent(lmkFolderWidget))
          {
          if(lmkFolderWidget->GetLocation()==nextLmkItemIndex)
            {
            this->CreateNestedXMLElements(lmkFolderWidget,root);
            nextLmkItemIndex++;
            break;
            }
          }
        }
      }
    counter++;
    }

  infile->close();
  outfile = new ofstream(filename,ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }

  root->PrintXML(*outfile,vtkIndent(1));
  outfile->close();
  parser->Delete();

  delete infile;
  delete outfile;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::CreateNestedXMLElements(vtkKWWidget *lmkItem, vtkXMLDataElement *dest)
{
//  vtkXMLLookmarkWriter *writer = vtkXMLLookmarkWriter::New();

  if(lmkItem->IsA("vtkKWLookmarkFolder") || lmkItem==this->LmkScrollFrame->GetFrame())
    {
    vtkXMLDataElement *folder = NULL;
    if(lmkItem->IsA("vtkKWLookmarkFolder"))
      {
      vtkKWLookmarkFolder *oldLmkFolder = vtkKWLookmarkFolder::SafeDownCast(lmkItem);
      if(this->LmkFolderWidgets->IsItemPresent(oldLmkFolder))
        {
        folder = vtkXMLDataElement::New();
        folder->SetName("LmkFolder");
        folder->SetAttribute("Name",oldLmkFolder->GetLabelFrame()->GetLabel()->GetText());
        dest->AddNestedElement(folder);

        vtkPVLookmark *lookmarkWidget;
        vtkKWLookmarkFolder *lmkFolderWidget;

        int nextLmkItemIndex=0;
        int counter=0;

        // loop through the children numberOfChildren times
        // if we come across a lmk item whose packed location among its siblings is the next one we're looking for
        //   recurse and break out of the inner loop, init traversal of children and repeat
        //   
        // the two loops are necessary because the user can change location of lmk items and
        // the vtkKWWidgetCollection of children is ordered by when the item was created, not reordered each time its packed (moved)

        vtkKWWidget *parent = 
          oldLmkFolder->GetLabelFrame()->GetFrame();
        
        while (counter < parent->GetNumberOfChildren())
          {
          int nb_children = parent->GetNumberOfChildren();
          for (int i = 0; i < nb_children; i++)
            {
            vtkKWWidget *child = parent->GetNthChild(i);
            if(child->IsA("vtkKWLookmark"))
              {
              lookmarkWidget = vtkPVLookmark::SafeDownCast(child);
              if(this->PVLookmarks->IsItemPresent(lookmarkWidget))
                {
                if(lookmarkWidget->GetLocation()==nextLmkItemIndex)
                  {
                  this->CreateNestedXMLElements(lookmarkWidget,folder);
                  nextLmkItemIndex++;
                  break;
                  }
                }
              }
            else if(child->IsA("vtkKWLookmarkFolder"))
              {
              lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(child);
              if(this->LmkFolderWidgets->IsItemPresent(lmkFolderWidget))
                {
                if(lmkFolderWidget->GetLocation()==nextLmkItemIndex)
                  {
                  this->CreateNestedXMLElements(lmkFolderWidget,folder);
                  nextLmkItemIndex++;
                  break;
                  }
                }
              }
            }
          counter++;
          }
        folder->Delete();
        }
      }
    else if(lmkItem==this->LmkScrollFrame->GetFrame())
      {
      // destination xmldataelement stays the same
      folder = dest;

      vtkKWWidget *parent = lmkItem;
      
      vtkPVLookmark *lookmarkWidget;
      vtkKWLookmarkFolder *lmkFolderWidget;

      int nextLmkItemIndex=0;
      int counter=0;

      // loop through the children numberOfChildren times
      // if we come across a lmk item whose packed location among its siblings is the next one we're looking for
      //   recurse and break out of the inner loop, init traversal of children and repeat
      //   
      // the two loops are necessary because the user can change location of lmk items and
      // the vtkKWWidgetCollection of children is ordered by when the item was created, not reordered each time its packed (moved)

      while (counter < parent->GetNumberOfChildren())
        {
        int nb_children = parent->GetNumberOfChildren();
        for (int i = 0; i < nb_children; i++)
          {
          vtkKWWidget *child = parent->GetNthChild(i);
          if(child->IsA("vtkKWLookmark"))
            {
            lookmarkWidget = vtkPVLookmark::SafeDownCast(child);
            if(this->PVLookmarks->IsItemPresent(lookmarkWidget))
              {
              if(lookmarkWidget->GetLocation()==nextLmkItemIndex)
                {
                this->CreateNestedXMLElements(lookmarkWidget,folder);
                nextLmkItemIndex++;
                break;
                }
              }
            }
          else if(child->IsA("vtkKWLookmarkFolder"))
            {
            lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(child);
            if(this->LmkFolderWidgets->IsItemPresent(lmkFolderWidget))
              {
              if(lmkFolderWidget->GetLocation()==nextLmkItemIndex)
                {
                this->CreateNestedXMLElements(lmkFolderWidget,folder);
                nextLmkItemIndex++;
                break;
                }
              }
            }
          }
        counter++;
        }
      }
    }
  else if(lmkItem->IsA("vtkKWLookmark"))
    {
    vtkPVLookmark *lookmarkWidget = vtkPVLookmark::SafeDownCast(lmkItem);

    if(this->PVLookmarks->IsItemPresent(lookmarkWidget))
      {
      lookmarkWidget->UpdateVariableValues();

      this->EncodeNewlines(lookmarkWidget->GetComments());

      //need to convert newlines in script and image data to encoded character before writing to xml file
      char *stateScript = lookmarkWidget->GetStateScript();
      this->EncodeNewlines(stateScript);

      vtkXMLLookmarkElement *elem = vtkXMLLookmarkElement::New();
      elem->SetName("Lmk");
      elem->SetAttribute("Name",lookmarkWidget->GetName());
      elem->SetAttribute("Comments", lookmarkWidget->GetComments());
      elem->SetAttribute("StateScript", lookmarkWidget->GetStateScript());
      elem->SetAttribute("ImageData", lookmarkWidget->GetImageData());
      elem->SetIntAttribute("PixelSize", lookmarkWidget->GetPixelSize());
//ds
      elem->SetAttribute("Dataset", lookmarkWidget->GetDataset());
      
      float *temp2;
      temp2 = lookmarkWidget->GetCenterOfRotation();
      elem->SetFloatAttribute("XCenterOfRotation", temp2[0]);
      elem->SetFloatAttribute("YCenterOfRotation", temp2[1]);
      elem->SetFloatAttribute("ZCenterOfRotation", temp2[2]);

      dest->AddNestedElement(elem);

//        writer->SetObject(lookmark);
//        writer->CreateInElement(dest);

      this->DecodeNewlines(stateScript);
      lookmarkWidget->SetComments(NULL);

      elem->Delete();

      }
    }
  else
    {
    // if the widget is not a lmk item, recurse with its children widgets but the same destination element as args

    vtkKWWidget *parent = lmkItem;
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      vtkKWWidget *widget = parent->GetNthChild(i);
      this->CreateNestedXMLElements(widget, dest);
      }
    }
//  writer->Delete();
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RenameLookmarkCallback()
{
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  int numChecked =0;

  // increment thru folders until we find one that is selected
  // then check to see if each subsequent selected folder is a descendant of it
  // if it is not, check to see if the stored folder is a descendant of it
  // if this is not the case either, we have multiple folders selected
  vtkIdType numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "A Folder is Selected", 
        "To rename a lookmark, select only one by checking its box. Then go to \"Edit\" --> \"Rename Lookmark\".",
        vtkKWMessageDialog::ErrorIcon);
      return;
      }
    }

  // no folders selected
  // only allow one lookmark to be selected now
  vtkPVLookmark *selectedLookmark = NULL;
  vtkIdType numLmkWidgets = this->PVLookmarks->GetNumberOfItems();
  for(i=numLmkWidgets-1;i>=0;i--)
    {
    this->PVLookmarks->GetItem(i,lookmarkWidget);
    if(lookmarkWidget->GetSelectionState()==1)
      {
      selectedLookmark = lookmarkWidget;
      numChecked++;
      if(numChecked>1)
        {
        vtkKWMessageDialog::PopupMessage(
          this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Lookmarks Selected", 
        "To rename a lookmark, select only one by checking its box. Then go to \"Edit\" --> \"Rename Lookmark\".",
          vtkKWMessageDialog::ErrorIcon);
        return;
        }
      }
    }

  if(selectedLookmark)
    {
    this->Checkpoint();
    selectedLookmark->EditLookmarkCallback();
    selectedLookmark->SetSelectionState(0);
    }

  if(numChecked==0)   // none selected
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "No Lookmarks Selected", 
        "To rename a lookmark, select only one by checking its box. Then go to \"Edit\" --> \"Rename Lookmark\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RenameFolderCallback()
{
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  vtkKWLookmarkFolder *rootFolder = NULL;
  int errorFlag = 0;

  // increment thru folders until we find one that is selected
  // then check to see if each subsequent selected folder is a descendant of it
  // if it is not, check to see if the stored folder is a descendant of it
  // if this is not the case either, we have multiple folders selected
  vtkIdType numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      if(rootFolder==NULL)
        {
        rootFolder = lmkFolderWidget;
        }
      else if(this->IsWidgetInsideFolder(rootFolder,lmkFolderWidget))
        {
        errorFlag = 0;
        rootFolder = lmkFolderWidget;
        }
      else if(!this->IsWidgetInsideFolder(lmkFolderWidget,rootFolder) && rootFolder->GetParent()==lmkFolderWidget->GetParent())
        {
        errorFlag = 1;
        }
      else
        {
        errorFlag = 1;
        break;
        }
      }
    }

  if(errorFlag)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Folders Selected", 
      "To rename a folder, select only one by checking its box. Then go to \"Edit\" --> \"Rename Folder\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }


  if(rootFolder)
    {
    //make sure all selected lookmarks are inside folder, if so, we can rename this folder
    vtkIdType numLmkWidgets = this->PVLookmarks->GetNumberOfItems();
    for(i=numLmkWidgets-1;i>=0;i--)
      {
      this->PVLookmarks->GetItem(i,lookmarkWidget);
      if(lookmarkWidget->GetSelectionState()==1)
        {
        if(!this->IsWidgetInsideFolder(lookmarkWidget,rootFolder))
          {
          vtkKWMessageDialog::PopupMessage(
            this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Lookmarks and Folders Selected", 
            "To rename a folder, select only one by checking its box. Then go to \"Edit\" --> \"Rename Folder\".",
            vtkKWMessageDialog::ErrorIcon);
          return;
          }
        }
      }

    this->Checkpoint();

    rootFolder->EditCallback();
    rootFolder->SetSelectionState(0);
    return;
    }
  else
    {
    // no folders selected
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "No Folders Selected", 
      "To rename a folder, select only one by checking its box. Then go to \"Edit\" --> \"Rename Folder\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RemoveCallback()
{
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  int numChecked = 0;


  vtkIdType numLmkWidgets = this->PVLookmarks->GetNumberOfItems();
  for(i=numLmkWidgets-1;i>=0;i--)
    {
    this->PVLookmarks->GetItem(i,lookmarkWidget);
    if(lookmarkWidget->GetSelectionState()==1)
      {
      numChecked++;
      }
    }
  vtkIdType numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      numChecked++;
      }
    }

  if(numChecked==0)   // none selected
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "No Lookmarks or Folders Selected", 
      "To remove lookmarks or folders, first select them by checking their boxes. Then go to \"Edit\" --> \"Remove Item(s)\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

  if ( !vtkKWMessageDialog::PopupYesNo(
         this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "RemoveItems",
         "Remove Selected Items", 
         "Are you sure you want to remove the selected items from the Lookmark Manager?", 
         vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
         vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault ))
    {
    return;
    }

  this->Checkpoint();

  this->RemoveCheckedChildren(this->LmkScrollFrame->GetFrame(),0);

  // update the callbacks to the thumbnails
  numLmkWidgets = this->PVLookmarks->GetNumberOfItems();
  for(i=0;i<numLmkWidgets;i++)
    {
    this->PVLookmarks->GetItem(i,lookmarkWidget);
    }

  this->Script("%s yview moveto 0",
               this->LmkScrollFrame->GetFrame()->GetParent()->GetWidgetName());

}



//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RemoveItemAsDragAndDropTarget(vtkKWWidget *target)
{
  vtkIdType numberOfLookmarkWidgets = this->PVLookmarks->GetNumberOfItems();
  vtkIdType numberOfLookmarkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  vtkKWLookmarkFolder *targetLmkFolder;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkPVLookmark *lookmarkWidget;
  vtkPVLookmark *targetLmkWidget;
  vtkIdType j=0;

  for(j=numberOfLookmarkFolders-1;j>=0;j--)
    {
    this->LmkFolderWidgets->GetItem(j,lmkFolderWidget);
    if(target != lmkFolderWidget)
      {
      targetLmkWidget = vtkPVLookmark::SafeDownCast(target);
      if(targetLmkWidget)
        lmkFolderWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkWidget->GetSeparatorFrame());
      targetLmkFolder = vtkKWLookmarkFolder::SafeDownCast(target);
      if(targetLmkFolder)
        {
        lmkFolderWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetSeparatorFrame());
        lmkFolderWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetNestedSeparatorFrame());
        lmkFolderWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetLabelFrame()->GetLabel());
        }
      }
    }

  for(j=numberOfLookmarkWidgets-1;j>=0;j--)
    {
    this->PVLookmarks->GetItem(j,lookmarkWidget);
    if(target != lookmarkWidget)
      {
      targetLmkWidget = vtkPVLookmark::SafeDownCast(target);
      if(targetLmkWidget)
        lookmarkWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkWidget->GetSeparatorFrame());
      targetLmkFolder = vtkKWLookmarkFolder::SafeDownCast(target);
      if(targetLmkFolder)
        {
        lookmarkWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetNestedSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetLabelFrame()->GetLabel());
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DecrementHigherSiblingLmkItemLocationIndices(vtkKWWidget *parent, int locationOfLmkItemBeingRemoved)
{
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  int siblingLocation=0;

  int nb_children = parent->GetNumberOfChildren();
  for (int i = 0; i < nb_children; i++)
    {
    vtkKWWidget *sibling = parent->GetNthChild(i);
    if(sibling->IsA("vtkKWLookmark"))
      {
      lookmarkWidget = vtkPVLookmark::SafeDownCast(sibling);
      if(lookmarkWidget)
        {
        siblingLocation = lookmarkWidget->GetLocation();
        if(siblingLocation>locationOfLmkItemBeingRemoved)
          lookmarkWidget->SetLocation(siblingLocation-1);
        }
      }
    else if(sibling->IsA("vtkKWLookmarkFolder"))
      {
      lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(sibling);
      if(lmkFolderWidget)
        {
        siblingLocation = lmkFolderWidget->GetLocation();
        if(siblingLocation > locationOfLmkItemBeingRemoved)
          lmkFolderWidget->SetLocation(siblingLocation-1);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::IncrementHigherSiblingLmkItemLocationIndices(vtkKWWidget *parent, int locationOfLmkItemBeingInserted)
{
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  int siblingLocation=0;

  int nb_children = parent->GetNumberOfChildren();
  for (int i = 0; i < nb_children; i++)
    {
    vtkKWWidget *sibling = parent->GetNthChild(i);
    if(sibling->IsA("vtkKWLookmark"))
      {
      lookmarkWidget = vtkPVLookmark::SafeDownCast(sibling);
      siblingLocation = lookmarkWidget->GetLocation();
      if(siblingLocation>=locationOfLmkItemBeingInserted)
        lookmarkWidget->SetLocation(siblingLocation+1); 
      }
    else if(sibling->IsA("vtkKWLookmarkFolder"))
      {
      lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(sibling);
      siblingLocation = lmkFolderWidget->GetLocation();
      if(siblingLocation>=locationOfLmkItemBeingInserted)
        lmkFolderWidget->SetLocation(siblingLocation+1);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::PackChildrenBasedOnLocation(vtkKWWidget *parent)
{
  parent->UnpackChildren();
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;

  // pack the nested frame if inside a folder, otherwise we are in the top level and we need to pack the top frame first
  if((lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(parent->GetParent()->GetParent()->GetParent()->GetParent())))
    {
    this->Script("pack %s -anchor nw -expand t -fill x",
                  lmkFolderWidget->GetNestedSeparatorFrame()->GetWidgetName());
    this->Script("%s configure -height 12",lmkFolderWidget->GetNestedSeparatorFrame()->GetWidgetName()); 
    }
  else
    {
    this->Script("pack %s -anchor w -fill both -side top",
                  this->TopDragAndDropTarget->GetWidgetName());
    this->Script("%s configure -height 12",
                  this->TopDragAndDropTarget->GetWidgetName());
    }

  int nextLmkItemIndex=0;
  int counter=0;

  while (counter < parent->GetNumberOfChildren())
    {
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      vtkKWWidget *child = parent->GetNthChild(i);
      if(child->IsA("vtkKWLookmark"))
        {
        lookmarkWidget = vtkPVLookmark::SafeDownCast(child);
        if(this->PVLookmarks->IsItemPresent(lookmarkWidget))
          {
          if(lookmarkWidget->GetLocation()==nextLmkItemIndex)
            {
            lookmarkWidget->Pack();
            this->Script("pack %s -fill both -expand yes -padx 8",lookmarkWidget->GetWidgetName());
            nextLmkItemIndex++;
            break;
            }
          }
        }
      else if(child->IsA("vtkKWLookmarkFolder"))
        {
        lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(child);
        if(this->LmkFolderWidgets->IsItemPresent(lmkFolderWidget))
          {
          if(lmkFolderWidget->GetLocation()==nextLmkItemIndex)
            {
            lmkFolderWidget->Pack();
            this->Script("pack %s -fill both -expand yes -padx 8",lmkFolderWidget->GetWidgetName());
            nextLmkItemIndex++;
            break;
            }
          }
        }
      }
    counter++;
    }
}

//----------------------------------------------------------------------------
char *vtkPVLookmarkManager::GetUnusedLookmarkName()
{
  char *name = new char[50];
  vtkPVLookmark *lmkWidget;
  vtkIdType k,numberOfItems;
  int i=0;
  numberOfItems = this->PVLookmarks->GetNumberOfItems();
  
  while(i<=numberOfItems)
    {
    sprintf(name,"Lookmark%d",i);
    k=0;
    this->PVLookmarks->GetItem(k,lmkWidget);
    while(k<numberOfItems && strcmp(name,lmkWidget->GetName()))
      {
      k++;
      this->PVLookmarks->GetItem(k,lmkWidget);
      }
    if(k==numberOfItems)
      break;  //there was not match so this lmkname is valid
    i++;
    }

  return name;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::CreateFolderCallback()
{
  vtkIdType numFolders, numItems;
  vtkKWLookmarkFolder *lmkFolderWidget;

  this->Checkpoint();

  lmkFolderWidget = vtkKWLookmarkFolder::New();
  // append to the end of the lookmark manager:
  lmkFolderWidget->SetParent(this->LmkScrollFrame->GetFrame());
  lmkFolderWidget->Create(this->GetPVApplication());
  char methodAndArg[200];
  sprintf(methodAndArg,"SelectItemCallback %s",lmkFolderWidget->GetWidgetName());
  lmkFolderWidget->GetCheckbox()->SetCommand(this,methodAndArg);
  this->Script("pack %s -fill both -expand yes -padx 8",lmkFolderWidget->GetWidgetName());
  this->Script("%s configure -height 8",lmkFolderWidget->GetLabelFrame()->GetFrame()->GetWidgetName());
  lmkFolderWidget->SetFolderName("New Folder");

  // get the location index to assign the folder
  numItems = this->GetNumberOfChildLmkItems(this->LmkScrollFrame->GetFrame());
  lmkFolderWidget->SetLocation(numItems);

  numFolders = this->LmkFolderWidgets->GetNumberOfItems();
  this->LmkFolderWidgets->InsertItem(numFolders,lmkFolderWidget);

  this->ResetDragAndDropTargetSetAndCallbacks();

  this->Script("update");

  // Try to get the scroll bar to initialize properly (show correct portion).
  this->Script("%s yview moveto 1", 
               this->LmkScrollFrame->GetFrame()->GetParent()->GetWidgetName());

}


//----------------------------------------------------------------------------
int vtkPVLookmarkManager::GetNumberOfChildLmkItems(vtkKWWidget *parentFrame)
{
  int location = 0;
  vtkPVLookmark *lmkWidget;
  vtkKWLookmarkFolder *lmkFolder;

  int nb_children = parentFrame->GetNumberOfChildren();
  for (int i = 0; i < nb_children; i++)
    {
    vtkKWWidget *widget = parentFrame->GetNthChild(i);
    if(widget->IsA("vtkKWLookmark"))
      {
      lmkWidget = vtkPVLookmark::SafeDownCast(widget);
      if(this->PVLookmarks->IsItemPresent(lmkWidget))
        location++;
      }
    else if(widget->IsA("vtkKWLookmarkFolder"))
      {
      lmkFolder = vtkKWLookmarkFolder::SafeDownCast(widget);
      if(this->LmkFolderWidgets->IsItemPresent(lmkFolder))
        location++;
      }
    }
  return location;
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DestroyUnusedLmkWidgets(vtkKWWidget *lmkItem)
{
  if(lmkItem->IsA("vtkKWLookmarkFolder"))
    {
    vtkKWLookmarkFolder *oldLmkFolder = vtkKWLookmarkFolder::SafeDownCast(lmkItem);
    if(!this->LmkFolderWidgets->IsItemPresent(oldLmkFolder))
      {
      oldLmkFolder->RemoveFolder();
      this->Script("destroy %s", oldLmkFolder->GetWidgetName());
      if(oldLmkFolder)
        {
        oldLmkFolder = NULL;
        }
      }
    else
      {
      vtkKWWidget *parent = 
        oldLmkFolder->GetLabelFrame()->GetFrame();
      int nb_children = parent->GetNumberOfChildren();
      for (int i = 0; i < nb_children; i++)
        {
        this->DestroyUnusedLmkWidgets(parent->GetNthChild(i));
        }
      }
    }
  else
    {
    vtkKWWidget *parent = lmkItem;
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->DestroyUnusedLmkWidgets(parent->GetNthChild(i));
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::MoveCheckedChildren(vtkKWWidget *nestedWidget, vtkKWWidget *packingFrame)
{
  vtkIdType loc;

  // Beginning at the Lookmark Manager's internal frame, we are going through each of its nested widgets (nestedWidget)

  if(nestedWidget->IsA("vtkKWLookmarkFolder"))
    {
    vtkKWLookmarkFolder *oldLmkFolder = vtkKWLookmarkFolder::SafeDownCast(nestedWidget);
    if(this->LmkFolderWidgets->IsItemPresent(oldLmkFolder))
      {
      vtkKWLookmarkFolder *newLmkFolder = vtkKWLookmarkFolder::New();
      newLmkFolder->SetParent(packingFrame);
      newLmkFolder->Create(this->GetPVApplication());
      char methodAndArg[200];
      sprintf(methodAndArg,"SelectItemCallback %s",newLmkFolder->GetWidgetName());
      newLmkFolder->GetCheckbox()->SetCommand(this,methodAndArg);
      newLmkFolder->SetFolderName(oldLmkFolder->GetLabelFrame()->GetLabel()->GetText());
      newLmkFolder->SetLocation(oldLmkFolder->GetLocation());
      this->Script("pack %s -fill both -expand yes -padx 8",newLmkFolder->GetWidgetName());

      this->LmkFolderWidgets->FindItem(oldLmkFolder,loc);
      this->LmkFolderWidgets->RemoveItem(loc);
      this->LmkFolderWidgets->InsertItem(loc,newLmkFolder);

      //loop through all children to this container's LabeledFrame

      vtkKWWidget *parent = 
        oldLmkFolder->GetLabelFrame()->GetFrame();
      int nb_children = parent->GetNumberOfChildren();
      for (int i = 0; i < nb_children; i++)
        {
        this->MoveCheckedChildren(
          parent->GetNthChild(i),
          newLmkFolder->GetLabelFrame()->GetFrame());
        }
// deleting old folder
      this->RemoveItemAsDragAndDropTarget(oldLmkFolder);
      this->Script("destroy %s", oldLmkFolder->GetWidgetName());
      oldLmkFolder->Delete();
      }
    }
  else if(nestedWidget->IsA("vtkKWLookmark"))
    {
    vtkPVLookmark *oldLmkWidget = vtkPVLookmark::SafeDownCast(nestedWidget);
  
    if(this->PVLookmarks->IsItemPresent(oldLmkWidget))
      {
      oldLmkWidget->UpdateVariableValues();
      vtkPVLookmark *newLmkWidget = vtkPVLookmark::New();
      newLmkWidget->SetMacroFlag(this->IsWidgetInsideFolder(packingFrame,this->GetMacrosFolder()));
      if(oldLmkWidget->GetMacroFlag())
        {
        this->GetPVApplication()->GetMainWindow()->GetLookmarkToolbar()->RemoveWidget(oldLmkWidget->GetToolbarButton());
        }
      newLmkWidget->SetParent(packingFrame);
      newLmkWidget->Create(this->GetPVApplication());
      char methodAndArg[200];
      sprintf(methodAndArg,"SelectItemCallback %s",newLmkWidget->GetWidgetName());
      newLmkWidget->GetCheckbox()->SetCommand(this,methodAndArg);
      newLmkWidget->SetName(oldLmkWidget->GetName());
      newLmkWidget->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
      ostrstream s;
      s << "GetPVLookmark \"" << newLmkWidget->GetName() << "\"" << ends;
      newLmkWidget->GetTraceHelper()->SetReferenceCommand(s.str());
      s.rdbuf()->freeze(0);
//ds
      newLmkWidget->SetDataset(oldLmkWidget->GetDataset());
      newLmkWidget->CreateDatasetList();
      newLmkWidget->SetLocation(oldLmkWidget->GetLocation());
      newLmkWidget->SetComments(oldLmkWidget->GetComments());
      newLmkWidget->SetImageData(oldLmkWidget->GetImageData());
      newLmkWidget->SetPixelSize(oldLmkWidget->GetPixelSize());
      newLmkWidget->CreateIconFromImageData();
      newLmkWidget->SetStateScript(oldLmkWidget->GetStateScript());
      newLmkWidget->UpdateWidgetValues();
      newLmkWidget->CommentsModifiedCallback();
      this->Script("pack %s -fill both -expand yes -padx 8",newLmkWidget->GetWidgetName());

      this->PVLookmarks->FindItem(oldLmkWidget,loc);
      this->PVLookmarks->RemoveItem(loc);
      this->PVLookmarks->InsertItem(loc,newLmkWidget);

      this->RemoveItemAsDragAndDropTarget(oldLmkWidget);
      this->Script("destroy %s", oldLmkWidget->GetWidgetName());
      oldLmkWidget->Delete();
      }
    }
  else
    {
    vtkKWWidget *parent = nestedWidget;
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->MoveCheckedChildren(
        parent->GetNthChild(i), packingFrame);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RemoveCheckedChildren(vtkKWWidget *nestedWidget, 
                                                 int forceRemoveFlag)
{
  vtkIdType loc;

  // Beginning at the Lookmark Manager's internal frame, we are going through
  // each of its nested widgets (nestedWidget)

  if(nestedWidget->IsA("vtkKWLookmarkFolder"))
    {
    vtkKWLookmarkFolder *oldLmkFolder = 
      vtkKWLookmarkFolder::SafeDownCast(nestedWidget);
    if(this->LmkFolderWidgets->IsItemPresent(oldLmkFolder))
      {
      if(oldLmkFolder->GetSelectionState() || forceRemoveFlag)
        {
        this->RemoveItemAsDragAndDropTarget(oldLmkFolder);
        this->DecrementHigherSiblingLmkItemLocationIndices(
          oldLmkFolder->GetParent(),oldLmkFolder->GetLocation());
        this->LmkFolderWidgets->FindItem(oldLmkFolder,loc);
        this->LmkFolderWidgets->RemoveItem(loc);

        //loop through all children to this container's LabeledFrame

        vtkKWWidget *parent = 
          oldLmkFolder->GetLabelFrame()->GetFrame();
        int nb_children = parent->GetNumberOfChildren();
        for (int i = 0; i < nb_children; i++)
          {
          this->RemoveCheckedChildren(
            parent->GetNthChild(i), 1);
          }

        this->RemoveItemAsDragAndDropTarget(oldLmkFolder);
        this->Script("destroy %s",oldLmkFolder->GetWidgetName());
        oldLmkFolder->Delete();
        oldLmkFolder = NULL;
        }
      else
        {
        vtkKWWidget *parent = 
          oldLmkFolder->GetLabelFrame()->GetFrame();
        int nb_children = parent->GetNumberOfChildren();
        for (int i = 0; i < nb_children; i++)
          {
          this->RemoveCheckedChildren(
            parent->GetNthChild(i), 0);
          }
        }
      }
    }
  else if(nestedWidget->IsA("vtkKWLookmark"))
    {
    vtkPVLookmark *oldLmkWidget = vtkPVLookmark::SafeDownCast(nestedWidget);
  
    if(this->PVLookmarks->IsItemPresent(oldLmkWidget))
      {
      if(oldLmkWidget->GetSelectionState() || forceRemoveFlag)
        {
        this->RemoveItemAsDragAndDropTarget(oldLmkWidget);
        this->DecrementHigherSiblingLmkItemLocationIndices(
          oldLmkWidget->GetParent(),oldLmkWidget->GetLocation());
        this->PVLookmarks->FindItem(oldLmkWidget,loc);
        if(oldLmkWidget->GetMacroFlag())
          {
          this->GetPVApplication()->GetMainWindow()->GetLookmarkToolbar()->RemoveWidget(oldLmkWidget->GetToolbarButton());
          }
        this->PVLookmarks->RemoveItem(loc);
        this->Script("destroy %s", oldLmkWidget->GetWidgetName());
        oldLmkWidget->Delete();
        oldLmkWidget = NULL;
        }
      }
    }
  else
    {
    vtkKWWidget *parent = nestedWidget;
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->RemoveCheckedChildren(
        parent->GetNthChild(i), forceRemoveFlag);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
}
 
//----------------------------------------------------------------------------
void vtkPVLookmarkManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TraceHelper: " << this->TraceHelper << endl;

}

