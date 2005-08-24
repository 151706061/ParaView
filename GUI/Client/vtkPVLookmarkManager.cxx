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

#include "vtkPVInteractorStyleCenterOfRotation.h"
#include "vtkPVLookmark.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVGUIClientOptions.h"
#include "vtkKWLookmark.h"
#include "vtkKWLookmarkFolder.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWText.h"
#include "vtkKWTextWithScrollbars.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWidget.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWDragAndDropTargetSet.h"
#include "vtkKWToolbar.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLLookmarkElement.h"
#include "vtkIndent.h"
#include "vtkVector.txx"
#include "vtkObjectFactory.h"
#include "vtkVectorIterator.txx"
#include "vtkPVTraceHelper.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLookmarkManager);
vtkCxxRevisionMacro(vtkPVLookmarkManager, "1.66");

//----------------------------------------------------------------------------
vtkPVLookmarkManager::vtkPVLookmarkManager()
{
  this->Lookmarks = vtkVector<vtkPVLookmark*>::New();
  this->Folders = vtkVector<vtkKWLookmarkFolder*>::New();
  this->MacroExamples = vtkVector<vtkPVLookmark*>::New();

  this->WindowFrame = vtkKWFrame::New();
  this->ScrollFrame = vtkKWFrameWithScrollbar::New();
  this->SeparatorFrame = vtkKWFrame::New();
  this->TopDragAndDropTarget = vtkKWFrame::New();
  this->CreateLookmarkButton = vtkKWPushButton::New();

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
  // Before exiting ParaView, save the current state of the manager:
  this->Checkpoint();

  this->TraceHelper->Delete();
  this->TraceHelper = 0;

  this->CreateLookmarkButton->Delete();
  this->CreateLookmarkButton= NULL;
  this->SeparatorFrame->Delete();
  this->SeparatorFrame= NULL;
  this->TopDragAndDropTarget->Delete();
  this->TopDragAndDropTarget= NULL;

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

  // The order in which we delete these widgets may matter, so I'm deleting lookmarks before folders

  vtkVectorIterator<vtkPVLookmark *> *macroIter = this->MacroExamples->NewIterator();
  while (!macroIter->IsDoneWithTraversal())
    {
    vtkPVLookmark *lmk = 0;
    if (macroIter->GetData(lmk) == VTK_OK && lmk)
      {
      lmk->Delete();
      }
    macroIter->GoToNextItem();
    }
  macroIter->Delete();
  this->MacroExamples->Delete();
  this->MacroExamples = 0;

  vtkVectorIterator<vtkPVLookmark *> *lmkIter = this->Lookmarks->NewIterator();
  while (!lmkIter->IsDoneWithTraversal())
    {
    vtkPVLookmark *lmk = 0;
    if (lmkIter->GetData(lmk) == VTK_OK && lmk)
      {
      lmk->Delete();
      }
    lmkIter->GoToNextItem();
    }
  lmkIter->Delete();
  this->Lookmarks->Delete();
  this->Lookmarks = 0;

  vtkVectorIterator<vtkKWLookmarkFolder *> *folderIter = this->Folders->NewIterator();
  while (!folderIter->IsDoneWithTraversal())
    {
    vtkKWLookmarkFolder *lmkFolder = 0;
    if (folderIter->GetData(lmkFolder) == VTK_OK && lmkFolder)
      {
      lmkFolder->Delete();
      }
    folderIter->GoToNextItem();
    }
  folderIter->Delete();
  this->Folders->Delete();
  this->Folders = 0;

  this->ScrollFrame->Delete();
  this->ScrollFrame = NULL;
  this->WindowFrame->Delete();
  this->WindowFrame= NULL;
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVLookmarkManager::GetPVRenderView()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp == NULL)
    {
    return NULL;
    }

  return this->GetPVApplication()->GetMainView();
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVLookmarkManager::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
vtkPVWindow* vtkPVLookmarkManager::GetPVWindow()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp == NULL)
    {
    return NULL;
    }
  
  return pvApp->GetMainWindow();
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Create(vtkKWApplication *app)
{
  char methodAndArgs[100];

  // Check if already created
  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget
  this->Superclass::Create(app);

  this->SetGeometry("380x700+0+0");
  this->SetDisplayPositionToScreenCenterFirst();

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
  this->ImportMacroExamplesCallback();
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
  this->MenuEdit->AddCommand("Select All", this, "SetStateOfAllLookmarkItems 1");
  this->MenuEdit->AddCommand("Clear All", this, "SetStateOfAllLookmarkItems 0");
  this->MenuEdit->SetItemState("Undo", vtkKWTkOptions::StateDisabled);
  this->MenuEdit->SetItemState("Redo", vtkKWTkOptions::StateDisabled);

  // Menu : Help

  this->MenuHelp->SetParent(root_menu);
  this->MenuHelp->SetTearOff(0);
  this->MenuHelp->Create(app);
  root_menu->AddCascade("Help", this->MenuHelp, 0);
  this->MenuHelp->AddCommand("Quick Start Guide", this, "DisplayQuickStartGuide");
  this->MenuHelp->AddCommand("User's Tutorial", this, "DisplayUsersTutorial");

  this->WindowFrame->SetParent(this);
  this->WindowFrame->Create(this->GetPVApplication());

  this->ScrollFrame->SetParent(this->WindowFrame);
  this->ScrollFrame->Create(this->GetPVApplication());

  this->SeparatorFrame->SetParent(this->WindowFrame);
  this->SeparatorFrame->Create(this->GetPVApplication());
  this->SeparatorFrame->SetBorderWidth(2);
  this->SeparatorFrame->SetReliefToGroove();

  this->CreateLookmarkButton->SetParent(this->WindowFrame);
  this->CreateLookmarkButton->Create(this->GetPVApplication());
  this->CreateLookmarkButton->SetText("Create Lookmark");
  sprintf(methodAndArgs,"CreateLookmarkCallback 0");
  this->CreateLookmarkButton->SetCommand(this,methodAndArgs);

  this->TopDragAndDropTarget->SetParent(this->ScrollFrame->GetFrame());
  this->TopDragAndDropTarget->Create(this->GetPVApplication());

  this->Script("pack %s -padx 2 -pady 4 -expand t", 
                this->CreateLookmarkButton->GetWidgetName());
  this->Script("pack %s -ipady 1 -pady 2 -anchor nw -expand t -fill x",
                 this->SeparatorFrame->GetWidgetName());
  this->Script("pack %s -anchor w -fill both -side top",
                 this->TopDragAndDropTarget->GetWidgetName());
  this->Script("%s configure -height 12",
                 this->TopDragAndDropTarget->GetWidgetName());
  this->Script("pack %s -anchor w -fill x -pady 12 -side top",
                 this->ScrollFrame->GetWidgetName());
  this->Script("pack %s -anchor n -side top -fill x -expand t",
                this->WindowFrame->GetWidgetName());

  this->Script("set commandList \"\"");

  // Import the lookmark file stored in the user's home directory if there is one
  const char *path = this->GetPathToFileInHomeDirectory("ParaViewlmk");
  if(path)
    {
    this->Import(path,0);
    }

  // If the file we just imported did not have a macros folder, create one
  vtkKWLookmarkFolder *folder = this->GetMacrosFolder();
  if(!folder)
    {
    vtkKWLookmarkFolder *macroFolder = vtkKWLookmarkFolder::New();
    macroFolder->SetParent(this->ScrollFrame->GetFrame());
    macroFolder->SetMacroFlag(1);
    macroFolder->Create(this->GetPVApplication());
    this->Script("pack %s -fill both -expand yes -padx 8",macroFolder->GetWidgetName());
    this->Script("%s configure -height 8",macroFolder->GetLabelFrame()->GetFrame()->GetWidgetName());
    macroFolder->SetFolderName("Macros");
    macroFolder->SetLocation(this->GetNumberOfChildLmkItems(this->ScrollFrame->GetFrame()));
    this->Folders->InsertItem(this->Folders->GetNumberOfItems(),macroFolder);
    this->DragAndDropWidget(macroFolder,this->TopDragAndDropTarget);
    this->PackChildrenBasedOnLocation(macroFolder->GetParent());
    this->ResetDragAndDropTargetSetAndCallbacks();
    }
}

const char* vtkPVLookmarkManager::GetPathToFileInHomeDirectory(const char *filename)
{
  ostrstream str;

  #ifndef _WIN32
  if ( !getenv("HOME") )
    {
    return 0;
    }
  str << getenv("HOME") << "/." << filename << ends;
  #else
  if ( !getenv("HOMEPATH") || !getenv("HOMEDRIVE") )
    {
    return 0;
    }
  str << getenv("HOMEDRIVE") << getenv("HOMEPATH") << "\\#" << filename << "#" << ends;
  #endif

  return str.str();
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::AddMacroExampleCallback(int index)
{
  vtkPVLookmark *lookmarkWidget;
  vtkPVLookmark *newLookmark = vtkPVLookmark::New();
  ostrstream s;
  ostrstream methodAndArgs;

  this->MacroExamples->GetItem(index,lookmarkWidget);
  if(!lookmarkWidget)
    {
    return;
    }

  // use this object to initialize the new lookmark widget
  newLookmark->SetName(lookmarkWidget->GetName());
  newLookmark->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
  if(newLookmark->GetName())
    {
    s << "GetPVLookmark \"" << newLookmark->GetName() << "\"" << ends;
    newLookmark->GetTraceHelper()->SetReferenceCommand(s.str());
    s.rdbuf()->freeze(0);
    }
  newLookmark->SetStateScript(lookmarkWidget->GetStateScript());
  newLookmark->SetName(lookmarkWidget->GetName());
  newLookmark->SetComments(lookmarkWidget->GetComments());
  newLookmark->SetDataset(lookmarkWidget->GetDataset());
  newLookmark->CreateDatasetList();
  newLookmark->SetImageData(lookmarkWidget->GetImageData());
  newLookmark->SetPixelSize(lookmarkWidget->GetPixelSize());
  newLookmark->SetMacroFlag(1);
  newLookmark->SetMainFrameCollapsedState(lookmarkWidget->GetMainFrameCollapsedState());
  newLookmark->SetCommentsFrameCollapsedState(lookmarkWidget->GetCommentsFrameCollapsedState());
  newLookmark->SetApplication(this->GetApplication());
  newLookmark->SetParent(this->GetMacrosFolder()->GetLabelFrame()->GetFrame());
  newLookmark->Create(this->GetPVApplication());
  methodAndArgs << "SelectItemCallback" << newLookmark->GetWidgetName() << ends;
  newLookmark->GetCheckbox()->SetCommand(this,methodAndArgs.str());
  methodAndArgs.rdbuf()->freeze(0);
  newLookmark->UpdateWidgetValues();
  newLookmark->CommentsModifiedCallback();
  this->Script("pack %s -fill both -expand yes -padx 8",newLookmark->GetWidgetName());
  newLookmark->CreateIconFromImageData();
  newLookmark->SetLocation(this->GetNumberOfChildLmkItems(this->GetMacrosFolder()->GetLabelFrame()->GetFrame()));
  this->Lookmarks->InsertItem(this->Lookmarks->GetNumberOfItems(),newLookmark);
  this->ResetDragAndDropTargetSetAndCallbacks();
}

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
  numberOfLookmarkWidgets = this->Lookmarks->GetNumberOfItems();
  numberOfLookmarkFolders = this->Folders->GetNumberOfItems();
  int i = 0;

  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->Lookmarks->GetItem(i,lookmarkWidget);
    if(strcmp(lookmarkWidget->GetWidgetName(),widgetName)==0)
      {
      widget = lookmarkWidget;
      break;
      }
    }
  if(!widget)
    {
    for(i=numberOfLookmarkFolders-1;i>=0;i--)
      {
      this->Folders->GetItem(i,lmkFolderWidget);
      if(strcmp(lmkFolderWidget->GetWidgetName(),widgetName)==0)
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
      this->Folders->GetItem(i,lmkFolderWidget);
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
      this->Folders->GetItem(i,lmkFolderWidget);
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
  int retval;
  char msg[500];

  if(this->GetPVApplication()->GetGUIClientOptions()->GetDisableRegistry())
    {
    return;
    }

  const char *path = this->GetPathToFileInHomeDirectory("LookmarkMacros");
  if(!path)
    {
    this->GetPVWindow()->ErrorMessage("Unable to find LookmarkMacros file to import in user's home directory.");
    return;
    }

  ifstream infile(path);
  if ( infile.fail())
    {
    sprintf(msg,"Error opening LookmarkMacros file in %s.",path);
    this->GetPVWindow()->ErrorMessage(msg);
    return;
    }

  //parse the .lmk xml file and get the root node for traversing
  parser = vtkXMLDataParser::New();
  parser->SetStream(&infile);
  retval = parser->Parse();
  if(retval==0)
    {
    sprintf(msg,"Error parsing lookmark file in %s.",path);
    this->GetPVWindow()->ErrorMessage(msg);
    parser->Delete();
    return;
    } 

  root = parser->GetRootElement();

  if(root)
    {
    this->ImportMacroExamplesInternal(0,root,this->MenuExamples);
    }

  parser->Delete();
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ImportMacroExamplesInternal(int locationOfLmkItemAmongSiblings, vtkXMLDataElement *lmkElement, vtkKWMenu *parentMenu)
{
  vtkPVLookmark *lookmarkWidget;
  vtkIdType j,numLmks;

  if(strcmp("LmkFolder",lmkElement->GetName())==0)
    {
/*
    vtkKWMenu *childMenu = vtkKWMenu::New();
    childMenu->SetParent(this->MenuEdit);
    childMenu->SetTearOff(0);
    childMenu->Create(this->GetPVApplication());
    parentMenu->AddCascade(lmkElement->GetAttribute("Name"), childMenu,0);
*/
    // use the label frame of this lmk container as the parent frame in which to pack into (constant)
    // for each xml element (either lookmark or lookmark container) recursively call import with the appropriate location and vtkXMLDataElement
    for(j=0; j<lmkElement->GetNumberOfNestedElements(); j++)
      {
      ImportMacroExamplesInternal(j,lmkElement->GetNestedElement(j),parentMenu);
      }
//    childMenu->Delete();
    }
  else if(strcmp("LmkFile",lmkElement->GetName())==0)
    {
    // in this case locationOfLmkItemAmongSiblings is the number of lookmark element currently in the first level of the lookmark manager which is why we start from that index
    // the parent is the lookmark manager's label frame
    for(j=0; j<lmkElement->GetNumberOfNestedElements(); j++)
      {
      ImportMacroExamplesInternal(j+locationOfLmkItemAmongSiblings,lmkElement->GetNestedElement(j),parentMenu);
      }
    }
  else if(strcmp("Lmk",lmkElement->GetName())==0)
    {
    // create lookmark widget
    lookmarkWidget = this->GetPVLookmark(lmkElement);
    lookmarkWidget->SetMacroFlag(1);
    numLmks = this->MacroExamples->GetNumberOfItems();
    this->MacroExamples->InsertItem(numLmks,lookmarkWidget);
    ostrstream checkCommand;
    checkCommand << "AddMacroExampleCallback " << numLmks << ends;
    parentMenu->AddCommand(lookmarkWidget->GetName(), this, checkCommand.str());
    checkCommand.rdbuf()->freeze(0);
    }
}

//----------------------------------------------------------------------------
vtkKWLookmarkFolder* vtkPVLookmarkManager::GetMacrosFolder()
{
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType numLmkFolders = this->Folders->GetNumberOfItems();
  for(int i=numLmkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
    if(strcmp(lmkFolderWidget->GetFolderName(),"Macros")==0)
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
  str << "Press \"Create Lookmark\". Note that a lookmark entry has appeared." << endl << endl;
  str << "Step 4:" << endl << endl;
  str << "Visit some other feature of interest, set the view parameters as desired." << endl << endl;
  str << "Step 5:" << endl << endl;
  str << "Press \"Create Lookmark\". Note that another lookmark entry has appeared." << endl << endl;
  str << "Step 6:" << endl << endl;
  str << "Click the thumbnail of the first lookmark. Note that ParaView returns to those view parameters and then hands control over to you." << endl << endl;
  str << "Step 7:" << endl << endl;
  str << "Click the thumbnail of the second lookmark. Note the same behavior." << endl << endl;
  str << "Step 8:" << endl << endl;
  str << "Read the User's Tutorial also available in the Help menu and explore the Lookmark Manager interface, to learn how to:" << endl << endl;
  str << "- Organize and edit lookmarks" << endl << endl;
  str << "- Save and import lookmarks to and from disk" << endl << endl;
  str << "- Use lookmarks on different datasets" << endl << endl;
  str << ends;
  this->QuickStartGuideTxt->GetWidget()->SetText( str.str() );
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
  str << "file and then imported in a later ParaView session or shared with co-workers." << endl << endl;

  str << "Feedback:" << endl << endl;
  str << "Any feedback you ";
  str << "have would be of great value. To report errors, suggest features or changes, or comment ";
  str << "on the functionality, please send an email to sdm-vs@ca.sandia.gov or call Eric Stanton ";
  str << "at 505-284-4422." << endl << endl;

  str << "Terminology:" << endl << endl;
  str << "Lookmark Manager - It is here that you import lookmarks into ParaView, toggle from ";
  str << "one lookmark to another, save and remove lookmarks as you see fit, create ";
  str << "new lookmarks of data, and interact with lookmark macros. The Lookmark Manager is also hierarchical so that you ";
  str << "can organize the lookmarks into nested folders." << endl << endl;

  str << "Lookmark Widget - You interact with lookmarks through the lookmark widgets displayed in ";
  str << "the Lookmark Manager. Each widget contains a thumbnail preview of the lookmark, a ";
  str << "collapsible comments area, its default dataset name, and the name of the lookmark itself. ";
  str << "A single-click of the thumbnail and you will visit that lookmark in the data window. A checkbox ";
  str << "placed in front of each widget allows lookmarks to be selected for removing, ";
  str << "renaming, and updating. A lookmark widget can also be dragged and dropped to other ";
  str << "parts of the Lookmark Manager by grabbing its label." << endl << endl;

  str << "Lookmark File - The contents of the Lookmark Manager or a sub-folder can be saved to a ";
  str << "lookmark file, a text-based, XML-structured file that stores the state information of all ";
  str << "lookmarks in the Lookmark Manager. This file can be loaded into any ParaView session and is ";
  str << "not tied to a particular dataset. It is this file that can easily be shared with co-workers." << endl << endl;

  str << "Lookmark Macros - Lookmark macros, or simply macros, are special kinds of lookmarks ";
  str << "that are intended to be general enough to be used on more than one dataset. Standard ";
  str << "lookmarks store the path to the dataset from which it was created and try to open the ";
  str << "dataset in ParaView (if it is not already open) when the lookmark is viewed. Macros, ";
  str << "on the other hand, will use datasets that are currently open in ParaView (see How to ";
  str << "view a macro and How to use macros with multiple readers and/or sources). All macros ";
  str << "are stored in the permanent \"Macros\" folder in the Lookmark Manager." << endl << endl;

  str << "Readers, Sources, and Filters - A reader refers to the item in the Selection Window ";
  str << "representing a dataset that has been opened in ParaView. Sources are models that can ";
  str << "be created by selecting from the Source menu of the main ParaView window. Filters are ";
  str << "used to manipulate and operate on readers and sources." << endl << endl;

  str << "How To:" << endl << endl;
  str << "Display the Lookmark Manager window - Select \"Window\" --> \"Lookmark Manager\" in the ";
  str << "top ParaView menu. The window that appears is detached from the main ParaView window. Note that you ";
  str << "can interact with the main ParaView window and the Lookmark Manager window remains in the foreground. ";
  str << "The Lookmark Manager window can be closed and reopened without affecting the contents of the Lookmark ";
  str << "Manager." << endl << endl;

  str << "Create a new lookmark - This can be accomplished two ways. The first is by pressing the \"Create Lookmark\" button in the Lookmark Manager. ";
  str << "The second is by pressing the icon that looks like a book in the Lookmark Toolbar (see How to display the Lookmark Toolbar) which will display the ";
  str << "Lookmark Manager, if needed and create a lookmark. Note that the Lookmark Manager will momentarily be moved behind the main ParaView window. This ";
  str << "is normal and necessary to generate the thumbnail of the current view. The state of the applicable ";
  str << "filters is saved with the lookmark. It is assigned an initial name of the form Lookmark#. A lookmark ";
  str << "widget is then appended to the bottom of the Lookmark Manager." << endl << endl;

  str << "View a lookmark - Click on the thumbnail of the lookmark you wish to view. You will then ";
  str << "witness the appropriate filters being created in the main ParaView window. Note the lookmark name ";
  str << "has been appended to the filter name of each filter belonging to this lookmark.  Clicking this same ";
  str << "lookmark again will cause these filters to be deleted if possible (i.e. if they have not been set as ";
  str << "inputs to other filters) and the saved filters will be regenerated. See also How to change the dataset ";
  str << "to which lookmarks are applied. " << endl << endl;

  str << "Update an existing lookmark - Select the lookmark to be updated and then press \"Edit\" --> ";
  str << "\"Update Lookmark\". This stores the state of all filters that contribute to the current view in that ";
  str << "lookmark. The lookmark's thumbnail is also replaced to reflect the current view. All other attributes ";
  str << "of the lookmark widget (name, comments, etc.) remain unchanged." << endl << endl;

  str << "Save the contents of the Lookmark Manager - Press \"File\" --> \"Save As\". You will be asked ";
  str << "to select a new or pre-existing lookmark file (with a .lmk extension) to which to save. All information ";
  str << "needed to recreate the current state of the Lookmark Manager is written to this file. This file can be ";
  str << "opened and edited in a text editor." << endl << endl;

  str << "Export the contents of a folder - Press \"File\" --> \"Export Folder\". You will be asked to ";
  str << "select a new or pre-existing lookmark file. All information needed to recreate the lookmarks and/or ";
  str << "folders nested within the selected folder is written to this file." << endl << endl;

  str << "Import a lookmark file - Press \"File\" --> \"Import\" --> and either \"Append\" or \"Replace\" ";
  str << "in its cascaded menu. The first will append the contents of the imported lookmark file to the existing ";
  str << "contents of the Lookmark Manager. The latter will first remove the contents of the Lookmark Manager and ";
  str << "then import the new lookmark file." << endl << endl;

  str << "Automatic saving and loading of the contents of the Lookmark Manager - Any time you modify ";
  str << "the Lookmark Manager in some way (create or update a lookmark, move, rename, or remove items, import ";
  str << "or export lookmarks), after the modification takes place a lookmark file by the name of \".ParaViewlmk\" ";
  str << "on UNIX and \"#ParaViewlmk#\" on Windows ";
  str << "is written to the user's home directory containing the state of the Lookmark Manager at that point in ";
  str << "time. This file is automatically imported into ParaView at the start of the session. It can be used to ";
  str << "recover your lookmarks in the event of a ParaView crash." << endl << endl;

  str << "Use a lookmark on a different dataset - This is the purpose of lookmark macros (see How to create a macro and How to view a macro." << endl << endl;

  str << "Display the Lookmark Toolbar - The Lookmark Toolbar consists of a button with an image of a book that will ";
  str << "display the Lookmark Manager and create a lookmark of the current view when pressed. Also, any macros contained in your ";
  str << "\"Macros\" folder are displayed next to it. To display this toolbar, press Window --> Toolbars --> Lookmark." << endl << endl;
  
  str << "Create a folder - Press \"Edit\" --> \"Create Folder\". This appends to the end of the Lookmark ";
  str << "Manager an empty folder named \"New Folder\". You can now drag lookmarks or other folders into this folder (see How to move ";
  str << "lookmarks and/or folders)." << endl << endl;

  str << "Move lookmarks and/or folders - A lookmark or folder can be moved in between any other lookmark ";
  str << "or folder in the Lookmark Manager. Simply move your mouse over the name of the item you wish to relocate ";
  str << "and hold the left mouse button down. Then drag the widget to the desired location (a box will appear under ";
  str << "your mouse if the location is a valid drop point) then release the left mouse button. Releasing over the ";
  str << "label of a folder will drop the item in the first nested entry of that folder." << endl << endl;

  str << "Remove lookmarks and/or folders - Select any combination of lookmarks and/or folders and then press ";
  str << "\"Edit\" --> \"Remove Item(s)\" button. You will be asked to verify that you wish to delete these. This prompt ";
  str << "may be turned off. " << endl << endl;

  str << "Rename a lookmark or folder - Select the lookmark or folder you wish to rename and press \"Edit\" --> ";
  str << "\"Rename Lookmark\" or \"Edit\" --> \"Rename Folder\".  This will replace the name with an editable text field ";
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

  str << "Undo a change to the Lookmark Manager - Press \"Edit\" --> \"Undo\". This will return the Lookmark ";
  str << "Manager's contents to its state before the previous action was performed." << endl << endl;

  str << "Redo a change to the Lookmark Manager - Press \"Edit\" --> \"Redo\". This will return the Lookmark ";
  str << "Manager's contents to its state before the previous Undo was performed." << endl << endl;

  str << "Create a new macro - Select Edit --> Create Macro in the Lookmark Manager. Note that when a macro is ";
  str << "created the Lookmark Manager will momentarily be moved behind the main ParaView window. This is normal and ";
  str << "necessary to correctly generate the macro�s thumbnail of the current view. The states of the applicable readers, ";
  str << "sources, and filters are saved with the macro. It is assigned an initial name of the form Lookmark# (for simplicity, ";
  str << "since a macro is a type of lookmark). A lookmark widget is then appended to the bottom of the Macros folder and its ";
  str << "thumbnail is displayed in the Lookmark Toolbar as well (see How to display the Lookmark Toolbar)." << endl << endl;

  str << "Convert a lookmark to a macro - Drag the desired lookmark into the \"Macros\" folder. You will notice the ";
  str << "dataset label has been removed from the widget. Also, the thumbnail of the macro will be displayed in the Lookmark ";
  str << "Toolbar (see How to display the Lookmark Toolbar)." << endl << endl;

  str << "Convert a macro to a lookmark - Drag the desired macro out of the \"Macros\" folder elsewhere in the ";
  str << "Lookmark Manager. You will notice the dataset label has been added back to the widget. Also, the thumbnail of ";
  str << "the macro has been removed from the Lookmark Toolbar (see How to display the Lookmark Toolbar)." << endl << endl;

  str << "View a macro - This can be accomplished two ways, by clicking on its thumbnail in the Lookmark Manager ";
  str << "or its counterpart in the Lookmark Toolbar (see How to display the Lookmark Toolbar). If the macro was created ";
  str << "from a single dataset, it will be used on the currently viewed dataset. The way the \"currently viewed dataset\" ";
  str << "is determined is by noting which source is currently highlighted in the Selection Window and finding its input ";
  str << "reader or source. The macro might otherwise have originated from multiple readers and/or sources (see How to use ";
  str << "macros with multiple readers and/or sources). Macros will maintain the existing camera angle and time step but ";
  str << "recreate all other operations performed by its filters. They will work primarily only on datasets with similar ";
  str << "properties as the one(s) from which it was created." << endl << endl;

  str << "Distribute pre-defined macros with ParaView - While a lookmark macro can be created in the Lookmark ";
  str << "Manager by the user, it can also be loaded automatically into the Edit --> Add Existing Macro menu. This can be ";
  str << "useful if you want to make available to users a set of \"pre-defined\" views of their data. Simply save a lookmark ";
  str << "file of the desired lookmarks (using either \"Save As\" or \"Export Folder\") to a file in your home directory named ";
  str << "\".LookmarkMacros\" on UNIX or \"#LookmarkMacros\" on Windows. Then, when ParaView is launched, it will read from ";
  str << "this file to populate the macros menu." << endl << endl;

  str << "Add pre-defined macros to the Macros folder - Press Edit --> Add Existing Macro and select from the ";
  str << "available macros (see How to distribute pre-defined macros in ParaView). You will then see a lookmark widget ";
  str << "appear in the Macros folder by that name." << endl << endl;

  str << "Use macros with multiple readers and/or sources - When a macro that originated from multiple readers ";
  str << "and/or sources is selected to be viewed (by clicking its thumbnail), the following algorithm is used to determine ";
  str << "which open readers and sources should be used in place of the readers and sources stored with the macro." << endl;
  str << " - For each reader stored with the macro: " << endl;
  str << "   - If there is only one open reader whose data type matches " << endl;
  str << "     the one stored with the macro, that reader is used." << endl;
  str << "   - Otherwise, if the filename of any of the open readers matches " << endl;
  str << "     the one stored with the macro, that reader is used." << endl;
  str << "   - Otherwise, if there are multiple open readers of the same ";
  str << "     type as the one in the macro, ask the user to specify which ";
  str << "     to use in place of the one stored with the macro." << endl;
  str << " - For each source stored with the macro:" << endl;
  str << "   - Unlike readers, macros can be used on sources of different " << endl;
  str << "     types. If there is only one source open, that source is used. " << endl;
  str << "     Otherwise, if there are multiple sources open, the user is asked " << endl;
  str << "     to specify which to use in place of the one stored with the macro." << endl << endl;

  str << ends;

  this->UsersTutorialTxt->GetWidget()->SetText( str.str() );
  str.rdbuf()->freeze(0);
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SetButtonFrameState(int state)
{
  this->CreateLookmarkButton->SetEnabled(state);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Checkpoint()
{
  const char *path = this->GetPathToFileInHomeDirectory("ParaViewlmk");
  if(path)
    {
    this->SaveAll(path);
    }

  this->MenuEdit->SetItemState("Undo", vtkKWTkOptions::StateNormal);
  this->MenuEdit->SetItemState("Redo", vtkKWTkOptions::StateDisabled);

}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RedoCallback()
{
  this->UndoRedoInternal();

  this->MenuEdit->SetItemState("Redo", vtkKWTkOptions::StateDisabled);
  this->MenuEdit->SetItemState("Undo", vtkKWTkOptions::StateNormal);
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UndoCallback()
{
  this->UndoRedoInternal();

  this->MenuEdit->SetItemState("Undo", vtkKWTkOptions::StateDisabled);
  this->MenuEdit->SetItemState("Redo", vtkKWTkOptions::StateNormal);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UndoRedoInternal()
{
  FILE *infile = NULL;
  FILE *outfile = NULL;
  char buf[300];

  if(this->GetPVApplication()->GetGUIClientOptions()->GetDisableRegistry())
    {
    return;
    }

  // Get the path to the checkpointed file
  const char *checkpointPath = this->GetPathToFileInHomeDirectory("ParaViewlmk");
  const char *tempfilePath = this->GetPathToFileInHomeDirectory("TempParaViewlmk");

  if(!checkpointPath || !tempfilePath)
    {
    // error message
    return;
    }

  ifstream checkfile(checkpointPath);

  if ( !checkfile.fail())
    {
    // save out the current contents to a temp file
    this->SaveAll(tempfilePath);
    this->Import(checkpointPath,0);
    checkfile.close();
    // copy over from temp file to checkpointed file
    if((infile = fopen(tempfilePath,"r")) && (outfile = fopen(checkpointPath,"w")))
      {
      while(fgets(buf,300,infile))
        {
        fputs(buf,outfile);
        }
      }
    remove(tempfilePath);
    }

  if(infile)
    {
    fclose(infile);
    }
  if (outfile)
    {
    fclose(outfile);
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SetStateOfAllLookmarkItems(int state)
{
  vtkIdType i,numberOfLookmarkWidgets, numberOfLookmarkFolders;
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;

  numberOfLookmarkWidgets = this->Lookmarks->GetNumberOfItems();
  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->Lookmarks->GetItem(i,lookmarkWidget);
    lookmarkWidget->SetSelectionState(state);
    }
  numberOfLookmarkFolders = this->Folders->GetNumberOfItems();
  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
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
    this->Script("pack %s -anchor w -fill both -side top",
                  this->ScrollFrame->GetWidgetName());
    this->SetButtonFrameState(1);

    return;
    }

  this->SetButtonFrameState(1);

  this->Checkpoint();

  this->Import(filename,this->MenuImport->GetCheckedRadioButtonItem(this,"Import"));

}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Import(const char *filename, int appendFlag)
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
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }

  this->GetTraceHelper()->AddEntry("$kw(%s) Import \"%s\" %d",
                      this->GetTclName(),filename,appendFlag);

  // If we are replacing the current items and there are items that exist, remove them
  if(appendFlag==0 && (this->Lookmarks->GetNumberOfItems()>0 || this->Folders->GetNumberOfItems()>0) )
    {
    this->RemoveCheckedChildren(this->ScrollFrame->GetFrame(),1);
    }

  //parse the .lmk xml file and get the root node for traversing
  parser = vtkXMLDataParser::New();
  parser->SetStream(&infile);
  retval = parser->Parse();
  if(retval==0)
    {
    sprintf(msg,"Error parsing lookmark file in %s.",filename);
    this->GetPVWindow()->ErrorMessage(msg);
    parser->Delete();
    return;
    }
  root = parser->GetRootElement();

  this->Script("[winfo toplevel %s] config -cursor watch", 
                this->GetWidgetName());

  if(!root)
    {
    sprintf(msg,"Error parsing lookmark file in %s.",filename);
    this->GetPVWindow()->ErrorMessage(msg);
    parser->Delete();
    return;
    }

  this->ImportInternal(this->GetNumberOfChildLmkItems(this->ScrollFrame->GetFrame()),root,this->ScrollFrame->GetFrame());
  
  // after all the widgets are generated, go back thru and add d&d targets for each of them
  this->ResetDragAndDropTargetSetAndCallbacks();

  this->Script("[winfo toplevel %s] config -cursor {}", 
                this->GetWidgetName());

  if(appendFlag)
    {
    this->Script("%s yview moveto 1",
                this->ScrollFrame->GetFrame()->GetParent()->GetWidgetName());
    }
  else
    {
    this->Script("%s yview moveto 0",
                this->ScrollFrame->GetFrame()->GetParent()->GetWidgetName());
    }

  // this is needed to enable the scrollbar in the lmk mgr
  this->Lookmarks->GetItem(0,lookmarkWidget);
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

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ResetDragAndDropTargetSetAndCallbacks()
{
  vtkIdType numberOfLookmarkWidgets = this->Lookmarks->GetNumberOfItems();
  vtkIdType numberOfLookmarkFolders = this->Folders->GetNumberOfItems();
  vtkKWLookmarkFolder *targetLmkFolder;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkPVLookmark *lookmarkWidget;
  vtkPVLookmark *targetLmkWidget;
  vtkIdType i=0;
  vtkIdType j=0;

  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->Lookmarks->GetItem(i,lookmarkWidget);
    lookmarkWidget->GetDragAndDropTargetSet()->SetEnable(1);
    // reset the drag and drop targets for this vtkKWLookmark starting with folders
    // for each lmk folder, add its internal frame, its label, and its nested frame as targets to this widget
    for(j=numberOfLookmarkFolders-1;j>=0;j--)
      {
      this->Folders->GetItem(j,lmkFolderWidget);
      if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(lmkFolderWidget->GetSeparatorFrame()))
        {
        lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(lmkFolderWidget->GetSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(lmkFolderWidget->GetSeparatorFrame(), this, "DragAndDropEndCommand");
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(lmkFolderWidget->GetSeparatorFrame(), lmkFolderWidget, "DragAndDropPerformCommand");
        }
      if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(lmkFolderWidget->GetNestedSeparatorFrame()))
        {
        lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(lmkFolderWidget->GetNestedSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(lmkFolderWidget->GetNestedSeparatorFrame(), this, "DragAndDropEndCommand");
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(lmkFolderWidget->GetNestedSeparatorFrame(), lmkFolderWidget, "DragAndDropPerformCommand");
        }
      if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(lmkFolderWidget->GetLabelFrame()->GetLabel()))
        {
        lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(lmkFolderWidget->GetLabelFrame()->GetLabel());
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(lmkFolderWidget->GetLabelFrame()->GetLabel(), this, "DragAndDropEndCommand");
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(lmkFolderWidget->GetLabelFrame()->GetLabel(), lmkFolderWidget, "DragAndDropPerformCommand");
        }
      }
    // now add the lookmarks as targets so long as they aren't the same as this one
    for(j=numberOfLookmarkWidgets-1;j>=0;j--)
      {
      this->Lookmarks->GetItem(j,targetLmkWidget);
      if(targetLmkWidget != lookmarkWidget)
        {
        if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkWidget->GetSeparatorFrame()))
          {
          lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkWidget->GetSeparatorFrame());
          lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkWidget->GetSeparatorFrame(), this, "DragAndDropEndCommand");
          lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkWidget->GetSeparatorFrame(), targetLmkWidget, "DragAndDropPerformCommand");
          }
        }
      }

    // now add the top frame as a target so that we can drag to the first position in the lookmark manager
    if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(this->TopDragAndDropTarget))
      {
      lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(this->TopDragAndDropTarget);
      lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(this->TopDragAndDropTarget, this, "DragAndDropEndCommand");
      lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(this->TopDragAndDropTarget, this, "DragAndDropPerformCommand");
      }
    }

  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
    // don't allow the macros folder to be moved
    if(lmkFolderWidget->GetMacroFlag())
      {
      continue;
      }
    lmkFolderWidget->GetDragAndDropTargetSet()->SetEnable(1);
    // must check to see if the widgets are descendants of this container widget if so dont add as target
    // for each lmk folder, add its internal frame, its label, and its nested frame as targets to this widget
    for(j=numberOfLookmarkFolders-1;j>=0;j--)
      {
      this->Folders->GetItem(j,targetLmkFolder);
      if(targetLmkFolder!=lmkFolderWidget && !this->IsWidgetInsideFolder(targetLmkFolder,lmkFolderWidget))
        {
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkFolder->GetSeparatorFrame()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkFolder->GetSeparatorFrame());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkFolder->GetSeparatorFrame(), this, "DragAndDropEndCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkFolder->GetSeparatorFrame(), targetLmkFolder, "DragAndDropPerformCommand");
          }
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkFolder->GetNestedSeparatorFrame()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkFolder->GetNestedSeparatorFrame());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkFolder->GetNestedSeparatorFrame(), this, "DragAndDropEndCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkFolder->GetNestedSeparatorFrame(), targetLmkFolder, "DragAndDropPerformCommand");
          }
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkFolder->GetLabelFrame()->GetLabel()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkFolder->GetLabelFrame()->GetLabel());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkFolder->GetLabelFrame()->GetLabel(), this, "DragAndDropEndCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkFolder->GetLabelFrame()->GetLabel(), targetLmkFolder, "DragAndDropPerformCommand");
          }
        }
      }
    // now add the lookmarks as targets so long as they aren't the same as this one
    for(j=numberOfLookmarkWidgets-1;j>=0;j--)
      {
      this->Lookmarks->GetItem(j,targetLmkWidget);
      if(!this->IsWidgetInsideFolder(targetLmkWidget,lmkFolderWidget))
        {
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkWidget->GetSeparatorFrame()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkWidget->GetSeparatorFrame());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkWidget->GetSeparatorFrame(), targetLmkWidget, "DragAndDropPerformCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkWidget->GetSeparatorFrame(), this, "DragAndDropEndCommand");
          }
        }
      }

    // now add the top frame as a target so that we can drag to the first position in the lookmark manager
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
  // "target" could be the label, frame, or nested frame of a folder OR the frame of a lookmark widget

  // enhancement: take the x,y, coords, loop through lookmarks and folders, and see which contain them

  vtkPVLookmark *lmkWidget;
  vtkKWLookmarkFolder *lmkFolder;

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

  this->DestroyUnusedLmkWidgets(this->ScrollFrame);

  this->ResetDragAndDropTargetSetAndCallbacks();

  // this is needed to enable the scrollbar in the lmk mgr
  this->Lookmarks->GetItem(0,lmkWidget);
  if(lmkWidget)
    {
    lmkWidget->EnableScrollBar();
    }
}


//----------------------------------------------------------------------------
int vtkPVLookmarkManager::DragAndDropWidget(vtkKWWidget *widget,vtkKWWidget *AfterWidget)
{
  int oldLoc;
  vtkPVLookmark *lmkWidget;
  vtkPVLookmark *afterLmkWidget;
  vtkKWLookmarkFolder *afterLmkFolder;
  vtkKWLookmarkFolder *lmkFolder;
  vtkKWWidget *dstPrnt;
  vtkIdType loc;
  ostrstream s;
  int newLoc;
  char methodAndArg[200];

  if (!widget || !widget->IsCreated())
    {
    return 0;
    }

  this->Checkpoint();

  // what type of widget are we dragging? lookmark or folder?
  if((lmkWidget = vtkPVLookmark::SafeDownCast(widget)))
    {
    if(!this->Lookmarks->IsItemPresent(lmkWidget))
      {
      return 0;
      }

    oldLoc = lmkWidget->GetLocation();
    // shift the location vars of the sibling lookmark items belonging to the widget being dragged
    lmkWidget->SetLocation(-1);
    this->DecrementHigherSiblingLmkItemLocationIndices(widget->GetParent(),oldLoc);

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
      this->GetPVWindow()->GetLookmarkToolbar()->RemoveWidget(lmkWidget->GetToolbarButton());
      }
    lmkWidget->UpdateVariableValues();
    newLmkWidget->SetParent(dstPrnt);
    newLmkWidget->Create(this->GetPVApplication());
    sprintf(methodAndArg,"SelectItemCallback %s",newLmkWidget->GetWidgetName());
    newLmkWidget->GetCheckbox()->SetCommand(this,methodAndArg);
    newLmkWidget->SetName(lmkWidget->GetName());
    newLmkWidget->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
    if(newLmkWidget->GetName())
      {
      s << "GetPVLookmark \"" << newLmkWidget->GetName() << "\"" << ends;
      newLmkWidget->GetTraceHelper()->SetReferenceCommand(s.str());
      s.rdbuf()->freeze(0);
      }
    newLmkWidget->SetDataset(lmkWidget->GetDataset());
    newLmkWidget->CreateDatasetList();
    newLmkWidget->SetLocation(newLoc);
    newLmkWidget->SetComments(lmkWidget->GetComments());
    newLmkWidget->SetMainFrameCollapsedState(lmkWidget->GetMainFrameCollapsedState());
    newLmkWidget->SetCommentsFrameCollapsedState(lmkWidget->GetCommentsFrameCollapsedState());
    newLmkWidget->UpdateWidgetValues();
    newLmkWidget->CommentsModifiedCallback();
    newLmkWidget->SetImageData(lmkWidget->GetImageData());
    newLmkWidget->SetPixelSize(lmkWidget->GetPixelSize());
    newLmkWidget->CreateIconFromImageData();
    newLmkWidget->SetStateScript(lmkWidget->GetStateScript());
    this->Script("pack %s -fill both -expand yes -padx 8",newLmkWidget->GetWidgetName());

    this->Lookmarks->FindItem(lmkWidget,loc);
    this->Lookmarks->RemoveItem(loc);
    this->Lookmarks->InsertItem(loc,newLmkWidget);

    this->RemoveItemAsDragAndDropTarget(lmkWidget);
    this->Script("destroy %s", lmkWidget->GetWidgetName());
    lmkWidget->Delete();
    }
  else if((lmkFolder = vtkKWLookmarkFolder::SafeDownCast(widget)))
    {
    if(!this->Folders->IsItemPresent(lmkFolder))
      {
      return 0;
      }

    oldLoc = lmkFolder->GetLocation();
    // shift the location vars of the sibling lookmark items belonging to the widget being dragged
    lmkFolder->SetLocation(-1);
    this->DecrementHigherSiblingLmkItemLocationIndices(widget->GetParent(),oldLoc);
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
    sprintf(methodAndArg,"SelectItemCallback %s",newLmkFolder->GetWidgetName());
    newLmkFolder->GetCheckbox()->SetCommand(this,methodAndArg);
    newLmkFolder->SetFolderName(lmkFolder->GetLabelFrame()->GetLabel()->GetText());
    newLmkFolder->SetMainFrameCollapsedState(lmkFolder->GetMainFrameCollapsedState());
    newLmkFolder->SetLocation(newLoc);
    this->Script("pack %s -fill both -expand yes -padx 8",newLmkFolder->GetWidgetName());
    newLmkFolder->UpdateWidgetValues();
    this->Folders->FindItem(lmkFolder,loc);
    this->Folders->RemoveItem(loc);
    this->Folders->InsertItem(loc,newLmkFolder);

    //loop through all children to this container's LabeledFrame

    vtkKWWidget *parent = lmkFolder->GetLabelFrame()->GetFrame();
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->MoveCheckedChildren(
        parent->GetNthChild(i), 
        newLmkFolder->GetLabelFrame()->GetFrame());
      }

    // delete the source folder
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
  const char *nameAttribute;
  int ival;
  char methodAndArg[200];

  if(strcmp("LmkFolder",lmkElement->GetName())==0)
    {
    nameAttribute = lmkElement->GetAttribute("Name");
    // if its a macros folder and we already have one, don't create but visit its children, passing as the parent the currrent macros folder packing frame
    if(nameAttribute && strcmp(nameAttribute,"Macros")==0 && this->GetMacrosFolder())
      {     
      for(j=0; j<lmkElement->GetNumberOfNestedElements(); j++)
        {
        ImportInternal(j,lmkElement->GetNestedElement(j),this->GetMacrosFolder()->GetLabelFrame()->GetFrame());
        }
      return;
      }

    lmkFolderWidget = vtkKWLookmarkFolder::New();
    lmkFolderWidget->SetParent(parent);
    if(nameAttribute && strcmp(nameAttribute,"Macros")==0 )
      {
      lmkFolderWidget->SetMacroFlag(1);
      }
    lmkFolderWidget->Create(this->GetPVApplication());
    sprintf(methodAndArg,"SelectItemCallback %s",lmkFolderWidget->GetWidgetName());
    lmkFolderWidget->GetCheckbox()->SetCommand(this,methodAndArg);
    this->Script("pack %s -fill both -expand yes -padx 8",lmkFolderWidget->GetWidgetName());
    lmkFolderWidget->SetFolderName(lmkElement->GetAttribute("Name"));
    lmkElement->GetScalarAttribute("MainFrameCollapsedState",ival);
    lmkFolderWidget->SetMainFrameCollapsedState(ival);
    lmkFolderWidget->UpdateWidgetValues();
    lmkFolderWidget->SetLocation(locationOfLmkItemAmongSiblings);

    numFolders = this->Folders->GetNumberOfItems();
    this->Folders->InsertItem(numFolders,lmkFolderWidget);

    // use the label frame of this lmk container as the parent frame in which to pack into (constant)
    // for each xml element (either lookmark or lookmark folder) recursively call import with the appropriate location and vtkXMLDataElement
    for(j=0; j<lmkElement->GetNumberOfNestedElements(); j++)
      {
      ImportInternal(j,lmkElement->GetNestedElement(j),lmkFolderWidget->GetLabelFrame()->GetFrame());
      }
    }
  else if(strcmp("LmkFile",lmkElement->GetName())==0)
    {
    // in this case locationOfLmkItemAmongSiblings is the number of lookmark element currently in the first level of the lookmark manager which is why we start from that index
    // the parent is the lookmark manager's label frame
    for(j=0; j<lmkElement->GetNumberOfNestedElements(); j++)
      {
      ImportInternal(j+locationOfLmkItemAmongSiblings,lmkElement->GetNestedElement(j),this->ScrollFrame->GetFrame());
      }
    }
  else if(strcmp("Lmk",lmkElement->GetName())==0)
    {
    // note that in the case of a lookmark, no recursion is done

    // this uses a vtkXMLLookmarkElement to create a vtkPVLookmark object
    // create lookmark widget
    lookmarkWidget = this->GetPVLookmark(lmkElement);
    lookmarkWidget->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
    ostrstream s;
    if(lookmarkWidget->GetName())
      {
      s << "GetPVLookmark \"" << lookmarkWidget->GetName() << "\"" << ends;
      lookmarkWidget->GetTraceHelper()->SetReferenceCommand(s.str());
      s.rdbuf()->freeze(0);
      }
    vtkKWLookmarkFolder *folder = this->GetMacrosFolder();
    if(folder)
      {
      lookmarkWidget->SetMacroFlag(this->IsWidgetInsideFolder(parent,this->GetMacrosFolder()));
      }
    lookmarkWidget->SetParent(parent);
    lookmarkWidget->Create(this->GetPVApplication());
    sprintf(methodAndArg,"SelectItemCallback %s",lookmarkWidget->GetWidgetName());
    lookmarkWidget->GetCheckbox()->SetCommand(this,methodAndArg);
    lookmarkWidget->UpdateWidgetValues();
    lookmarkWidget->CommentsModifiedCallback();
    this->Script("pack %s -fill both -expand yes -padx 8",lookmarkWidget->GetWidgetName());

    lookmarkWidget->CreateIconFromImageData();
    lookmarkWidget->SetLocation(locationOfLmkItemAmongSiblings);

    numLmks = this->Lookmarks->GetNumberOfItems();
    this->Lookmarks->InsertItem(numLmks,lookmarkWidget);
    }
}


//----------------------------------------------------------------------------
char* vtkPVLookmarkManager::PromptForLookmarkFile(int saveFlag)
{
  ostrstream str;
  vtkKWLoadSaveDialog* dialog = vtkKWLoadSaveDialog::New();
  vtkPVWindow *win = this->GetPVWindow();

  if(saveFlag)
    {
    dialog->SaveDialogOn();
    }

  dialog->Create(this->GetPVApplication());

  if (win)
    {
    dialog->SetParent(this->ScrollFrame);
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

  return dialog->GetFileName();
  
}


//----------------------------------------------------------------------------
vtkPVLookmark *vtkPVLookmarkManager::GetPVLookmark(char *name)
{
  vtkPVLookmark *lookmarkWidget;
  vtkIdType numLmkWidgets = this->Lookmarks->GetNumberOfItems();
  for(int i=numLmkWidgets-1;i>=0;i--)
    {
    this->Lookmarks->GetItem(i,lookmarkWidget);
    if(strcmp(lookmarkWidget->GetName(),name)==0)
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
 
  if(elem->GetAttribute("Name"))
    {
    char *lookmarkName = new char[strlen(elem->GetAttribute("Name"))+1]; 
    strcpy(lookmarkName,elem->GetAttribute("Name"));
    lmk->SetName(lookmarkName);
    delete [] lookmarkName;
    }

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

  int ival;
  if(elem->GetScalarAttribute("MainFrameCollapsedState",ival))
    {
    lmk->SetMainFrameCollapsedState(ival);
    }

  if(elem->GetScalarAttribute("CommentsFrameCollapsedState",ival))
    {
    lmk->SetCommentsFrameCollapsedState(ival);
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
  vtkPVWindow *win = this->GetPVWindow();
  int numChecked = 0;

  numLmkWidgets = this->Lookmarks->GetNumberOfItems();

  for(lmkIndex=0; lmkIndex<numLmkWidgets; lmkIndex++)
    {
    this->Lookmarks->GetItem(lmkIndex,lookmarkWidget);
    if(lookmarkWidget->GetSelectionState())
      {
      numChecked++;
      } 
    }

  // if no lookmarks are selected or if more than one are selected, fail
  if(numChecked==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "No Lookmark Selected", 
      "To update a lookmark to the current view, first select only one lookmark by checking its box. Then  go to \"Edit\" --> \"Update Lookmark\".", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }
  else if(numChecked > 1)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "Multiple Lookmarks Selected", 
      "To update a lookmark to the current view, first select only one lookmark by checking its box. Then  go to \"Edit\" --> \"Update Lookmark\".", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }

  this->Checkpoint();

  for(lmkIndex=0; lmkIndex<numLmkWidgets; lmkIndex++)
    {
    this->Lookmarks->GetItem(lmkIndex,lookmarkWidget);
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
  vtkPVWindow *win = this->GetPVWindow();
  vtkPVLookmark *lmk;

  // if the pipeline is empty, don't add
  if(win->GetSourceList("Sources")->GetNumberOfItems()==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "No Data Loaded", 
      "To create a lookmark you must first open your data and view some feature of interest. Then press \"Create Lookmark\" in the main window of the Lookmark Manager or in its \"Edit\" menu. Also, if the Lookmark toolbar is enabled, you can press the icon of a book in the main ParaView window.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }

  lmk = this->CreateLookmark(this->GetUnusedLookmarkName(), macroFlag);

  return;
}


//----------------------------------------------------------------------------
vtkPVLookmark* vtkPVLookmarkManager::CreateLookmark(char *name, int macroFlag)
{
  vtkIdType numLmkWidgets = this->Lookmarks->GetNumberOfItems();
  vtkPVLookmark *newLookmark;
  int indexOfNewLmkWidget;
  vtkPVWindow *win = this->GetPVWindow();
  char methodAndArg[200];
  ostrstream s;

  this->GetTraceHelper()->AddEntry("$kw(%s) CreateLookmark \"%s\" %d",
                      this->GetTclName(),name,macroFlag);

  this->Checkpoint();

  // create and initialize pvlookmark:

  newLookmark = vtkPVLookmark::New();
  // set the parent depending on whether it is a macro or normal lookmark
  if(macroFlag)
    {
    newLookmark->SetParent(this->GetMacrosFolder()->GetLabelFrame()->GetFrame());
    } 
  else
    {
    newLookmark->SetParent(this->ScrollFrame->GetFrame());
    }
  newLookmark->SetMacroFlag(macroFlag);
  newLookmark->Create(this->GetPVApplication());
  sprintf(methodAndArg,"SelectItemCallback %s",newLookmark->GetWidgetName());
  newLookmark->GetCheckbox()->SetCommand(this,methodAndArg);
  newLookmark->SetName(name);
  newLookmark->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
  if(newLookmark->GetName())
    {
    s << "GetPVLookmark \"" << newLookmark->GetName() << "\"" << ends;
    newLookmark->GetTraceHelper()->SetReferenceCommand(s.str());
    s.rdbuf()->freeze(0);
    }
  newLookmark->SetCenterOfRotation(win->GetCenterOfRotationStyle()->GetCenter());
  newLookmark->InitializeDataset();
  newLookmark->StoreStateScript();
  newLookmark->UpdateWidgetValues();
  newLookmark->CommentsModifiedCallback();
  this->Script("pack %s -fill both -expand yes -padx 8",newLookmark->GetWidgetName());

  // determing location of widget
  if(macroFlag)
    {
    indexOfNewLmkWidget = this->GetNumberOfChildLmkItems(this->GetMacrosFolder()->GetLabelFrame()->GetFrame());
    }
  else
    {
    indexOfNewLmkWidget = this->GetNumberOfChildLmkItems(this->ScrollFrame->GetFrame());
    }
  newLookmark->SetLocation(indexOfNewLmkWidget);
  newLookmark->CreateIconFromMainView();

  this->Lookmarks->InsertItem(numLmkWidgets,newLookmark);

  this->ResetDragAndDropTargetSetAndCallbacks();

  this->Script("update");

  this->Script("%s yview moveto 1",
               this->ScrollFrame->GetFrame()->GetParent()->GetWidgetName());

  return newLookmark;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveAllCallback()
{
  char *filename;

  this->SetButtonFrameState(0);

  if(!(filename = this->PromptForLookmarkFile(1)))
    {
    this->SetButtonFrameState(1);
    return;
    }

  const char *path = this->GetPathToFileInHomeDirectory("ParaViewlmk");

  if(path)
    {
    if(strcmp(filename,path)==0)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetPVApplication(), this->GetPVWindow(), "Cannot Save to Application Lookmark File", 
        "Please select a different lookmark file to save to. The one you have chosen is restricted for use by the ParaView application.",
        vtkKWMessageDialog::ErrorIcon);
      return;
      }
    }

  this->GetTraceHelper()->AddEntry("$kw(%s) SaveAll \"%s\"",
                      this->GetTclName(),filename);

  this->SaveAll(filename);

  this->SetButtonFrameState(1);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ExportFolderCallback()
{
  char *filename;
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  int numChecked = 0;
  vtkKWLookmarkFolder *rootFolder = NULL;
  int errorFlag = 0;
  vtkIdType numberOfLookmarkWidgets, numberOfLookmarkFolders;

  vtkIdType numLmkFolders = this->Folders->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      numChecked++;
      }
    }

  if(numChecked==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVWindow(), "No Folders Selected", 
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

  const char *path = this->GetPathToFileInHomeDirectory("ParaViewlmk");
  if(path)
    {
    if(strcmp(filename,path)==0)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetPVApplication(), this->GetPVWindow(), "Cannot Save to Application Lookmark File", 
        "Please select a different lookmark file to save to. The one you have chosen is restricted for use by the ParaView application.",
        vtkKWMessageDialog::ErrorIcon);
      this->SetButtonFrameState(1);
      return;
      }
    }

  // loop thru folders until we find one that is selected
  // then check to see if each subsequent selected folder is a descendant of it
  // if it is not, check to see if the stored folder is a descendant of it
  // if this is not the case either, we have multiple folders selected

  numLmkFolders = this->Folders->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
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
      this->GetPVApplication(), this->GetPVWindow(), "Multiple Folders Selected", 
      "To export a folder of lookmarks to a lookmark file, first select a folder by checking its box. Then go to \"File\" --> \"Export Folder\"",
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    this->SetButtonFrameState(1);

    return;
    }

  if(rootFolder)
    {
    //make sure all selected lookmarks are inside folder
    vtkIdType numLmkWidgets = this->Lookmarks->GetNumberOfItems();
    for(i=numLmkWidgets-1;i>=0;i--)
      {
      this->Lookmarks->GetItem(i,lookmarkWidget);
      if(lookmarkWidget->GetSelectionState()==1)
        {
        if(!this->IsWidgetInsideFolder(lookmarkWidget,rootFolder))
          {
          vtkKWMessageDialog::PopupMessage(
            this->GetPVApplication(), this->GetPVWindow(), "Multiple Lookmarks and Folders Selected", 
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

  numberOfLookmarkWidgets = this->Lookmarks->GetNumberOfItems();
  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->Lookmarks->GetItem(i,lookmarkWidget);
    lookmarkWidget->SetSelectionState(0);
    }
  numberOfLookmarkFolders = this->Folders->GetNumberOfItems();
  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
    lmkFolderWidget->SetSelectionState(0);
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveAll(const char *filename)
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
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    delete outfile;
    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
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
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();
    delete infile;
    return;
    }
  if ( infile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
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
    this->GetPVWindow()->ErrorMessage(msg);
    parser->Delete();
    delete infile;
    return;
    } 

  root = (vtkXMLLookmarkElement *)parser->GetRootElement();

  if(!root)
    {
    sprintf(msg,"Error parsing lookmark file in %s.",filename);
    this->GetPVWindow()->ErrorMessage(msg);
    parser->Delete();
    delete infile;
    return;
    }

  this->CreateNestedXMLElements(this->ScrollFrame->GetFrame(),root);

  infile->close();
  outfile = new ofstream(filename,ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
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
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
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
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
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
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }
  if ( infile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
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
    this->GetPVWindow()->ErrorMessage(msg);
    parser->Delete();
    delete infile;
    delete outfile;
    return;
    } 

  root = (vtkXMLLookmarkElement *)parser->GetRootElement();

  if(!root)
    {
    sprintf(msg,"Error parsing lookmark file in %s.",filename);
    this->GetPVWindow()->ErrorMessage(msg);
    parser->Delete();
    delete infile;
    delete outfile;
    return;
    }

  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;

  int nextLmkItemIndex=0;
  int counter=0;

  // loop through the children of the folder
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
        if(this->Lookmarks->IsItemPresent(lookmarkWidget))
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
        if(this->Folders->IsItemPresent(lmkFolderWidget))
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
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Focus();

    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVWindow(), "Could Not Open Lookmark File", 
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
  vtkXMLDataElement *folder;
  vtkKWLookmarkFolder *oldLmkFolder;
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  int nextLmkItemIndex, counter, nb_children, i;
  vtkKWWidget *child;
  vtkKWWidget *parent;
  char *stateScript;
  vtkXMLLookmarkElement *elem;
  float *fval;
  vtkKWWidget *widget;

  if(lmkItem->IsA("vtkKWLookmarkFolder") || lmkItem==this->ScrollFrame->GetFrame())
    {
    if(lmkItem->IsA("vtkKWLookmarkFolder"))
      {
      oldLmkFolder = vtkKWLookmarkFolder::SafeDownCast(lmkItem);
      if(this->Folders->IsItemPresent(oldLmkFolder))
        {
        folder = vtkXMLDataElement::New();
        folder->SetName("LmkFolder");
        oldLmkFolder->UpdateVariableValues();
        if(strlen(oldLmkFolder->GetFolderName())==0)
          {
          oldLmkFolder->SetFolderName("Unnamed Folder");
          }
        folder->SetAttribute("Name",oldLmkFolder->GetFolderName());
        folder->SetIntAttribute("MainFrameCollapsedState",oldLmkFolder->GetMainFrameCollapsedState());
        dest->AddNestedElement(folder);

        // loop through the children of this folder
        // if we come across a lmk item whose packed location among its siblings is the next one we're looking for
        //   recurse and break out of the inner loop, init traversal of children and repeat
        //   
        // the two loops are necessary because the user can change location of lmk items and
        // the vtkKWWidgetCollection of children is ordered by when the item was created, not reordered each time its packed (moved)

        vtkKWWidget *parent = 
          oldLmkFolder->GetLabelFrame()->GetFrame();
       counter = nextLmkItemIndex = nb_children = 0;
        while (counter < parent->GetNumberOfChildren())
          {
          nb_children = parent->GetNumberOfChildren();
          for (i = 0; i < nb_children; i++)
            {
            child = parent->GetNthChild(i);
            if(child->IsA("vtkKWLookmark"))
              {
              lookmarkWidget = vtkPVLookmark::SafeDownCast(child);
              if(this->Lookmarks->IsItemPresent(lookmarkWidget))
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
              if(this->Folders->IsItemPresent(lmkFolderWidget))
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
    else if(lmkItem==this->ScrollFrame->GetFrame())
      {
      // destination xmldataelement stays the same
      folder = dest;
      parent = lmkItem;
      
      // loop through the children numberOfChildren times
      // if we come across a lmk item whose packed location among its siblings is the next one we're looking for
      //   recurse and break out of the inner loop, init traversal of children and repeat
      //   
      // the two loops are necessary because the user can change location of lmk items and
      // the vtkKWWidgetCollection of children is ordered by when the item was created, not reordered each time its packed (moved)
      
      counter = nextLmkItemIndex = nb_children = 0;
      while (counter < parent->GetNumberOfChildren())
        {
        nb_children = parent->GetNumberOfChildren();
        for (i = 0; i < nb_children; i++)
          {
          child = parent->GetNthChild(i);
          if(child->IsA("vtkKWLookmark"))
            {
            lookmarkWidget = vtkPVLookmark::SafeDownCast(child);
            if(this->Lookmarks->IsItemPresent(lookmarkWidget))
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
            if(this->Folders->IsItemPresent(lmkFolderWidget))
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
    lookmarkWidget = vtkPVLookmark::SafeDownCast(lmkItem);

    if(this->Lookmarks->IsItemPresent(lookmarkWidget))
      {
      lookmarkWidget->UpdateVariableValues();

      this->EncodeNewlines(lookmarkWidget->GetComments());

      //need to convert newlines in script and image data to encoded character before writing to xml file
      stateScript = lookmarkWidget->GetStateScript();
      this->EncodeNewlines(stateScript);

      elem = vtkXMLLookmarkElement::New();
      elem->SetName("Lmk");
      if(strlen(lookmarkWidget->GetName())==0)
        {
        lookmarkWidget->SetName("Unnamed Lookmark");
        }
      elem->SetAttribute("Name",lookmarkWidget->GetName());
      elem->SetAttribute("Comments", lookmarkWidget->GetComments());
      elem->SetAttribute("StateScript", lookmarkWidget->GetStateScript());
      elem->SetAttribute("ImageData", lookmarkWidget->GetImageData());
      elem->SetIntAttribute("PixelSize", lookmarkWidget->GetPixelSize());
      elem->SetAttribute("Dataset", lookmarkWidget->GetDataset());
      elem->SetIntAttribute("MainFrameCollapsedState", lookmarkWidget->GetMainFrameCollapsedState());
      elem->SetIntAttribute("CommentsFrameCollapsedState", lookmarkWidget->GetCommentsFrameCollapsedState());
      
      fval = lookmarkWidget->GetCenterOfRotation();
      elem->SetFloatAttribute("XCenterOfRotation", fval[0]);
      elem->SetFloatAttribute("YCenterOfRotation", fval[1]);
      elem->SetFloatAttribute("ZCenterOfRotation", fval[2]);

      dest->AddNestedElement(elem);

      this->DecodeNewlines(stateScript);
      lookmarkWidget->SetComments(NULL);

      elem->Delete();
      }
    }
  else
    {
    // if the widget is not a lmk item, recurse with its children widgets but the same destination element as args
    parent = lmkItem;
    nb_children = parent->GetNumberOfChildren();
    for (i = 0; i < nb_children; i++)
      {
      widget = parent->GetNthChild(i);
      this->CreateNestedXMLElements(widget, dest);
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RenameLookmarkCallback()
{
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  int numChecked =0;

  // Error if a folder is selected
  vtkIdType numLmkFolders = this->Folders->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetPVApplication(), this->GetPVWindow(), "A Folder is Selected", 
        "To rename a lookmark, select only one by checking its box. Then go to \"Edit\" --> \"Rename Lookmark\".",
        vtkKWMessageDialog::ErrorIcon);
      return;
      }
    }

  // no folders selected
  // only allow one lookmark to be selected now
  vtkPVLookmark *selectedLookmark = NULL;
  vtkIdType numLmkWidgets = this->Lookmarks->GetNumberOfItems();
  for(i=numLmkWidgets-1;i>=0;i--)
    {
    this->Lookmarks->GetItem(i,lookmarkWidget);
    if(lookmarkWidget->GetSelectionState()==1)
      {
      selectedLookmark = lookmarkWidget;
      numChecked++;
      if(numChecked>1)
        {
        vtkKWMessageDialog::PopupMessage(
          this->GetPVApplication(), this->GetPVWindow(), "Multiple Lookmarks Selected", 
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
  else
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVWindow(), "No Lookmarks Selected", 
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
  vtkIdType numLmkFolders = this->Folders->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
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
      this->GetPVApplication(), this->GetPVWindow(), "Multiple Folders Selected", 
      "To rename a folder, select only one by checking its box. Then go to \"Edit\" --> \"Rename Folder\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }


  if(rootFolder)
    {
    //make sure all selected lookmarks are inside folder, if so, we can rename this folder
    vtkIdType numLmkWidgets = this->Lookmarks->GetNumberOfItems();
    for(i=numLmkWidgets-1;i>=0;i--)
      {
      this->Lookmarks->GetItem(i,lookmarkWidget);
      if(lookmarkWidget->GetSelectionState()==1)
        {
        if(!this->IsWidgetInsideFolder(lookmarkWidget,rootFolder))
          {
          vtkKWMessageDialog::PopupMessage(
            this->GetPVApplication(), this->GetPVWindow(), "Multiple Lookmarks and Folders Selected", 
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
      this->GetPVApplication(), this->GetPVWindow(), "No Folders Selected", 
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


  vtkIdType numLmkWidgets = this->Lookmarks->GetNumberOfItems();
  for(i=numLmkWidgets-1;i>=0;i--)
    {
    this->Lookmarks->GetItem(i,lookmarkWidget);
    if(lookmarkWidget->GetSelectionState()==1)
      {
      numChecked++;
      }
    }
  vtkIdType numLmkFolders = this->Folders->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      numChecked++;
      }
    }

  if(numChecked==0)   // none selected
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVWindow(), "No Lookmarks or Folders Selected", 
      "To remove lookmarks or folders, first select them by checking their boxes. Then go to \"Edit\" --> \"Remove Item(s)\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

  if ( !vtkKWMessageDialog::PopupYesNo(
         this->GetPVApplication(), this->GetPVWindow(), "RemoveItems",
         "Remove Selected Items", 
         "Are you sure you want to remove the selected items from the Lookmark Manager?", 
         vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
         vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault ))
    {
    return;
    }

  this->Checkpoint();

  this->RemoveCheckedChildren(this->ScrollFrame->GetFrame(),0);

  this->Script("%s yview moveto 0",
               this->ScrollFrame->GetFrame()->GetParent()->GetWidgetName());

}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RemoveItemAsDragAndDropTarget(vtkKWWidget *target)
{
  vtkIdType numberOfLookmarkWidgets = this->Lookmarks->GetNumberOfItems();
  vtkIdType numberOfLookmarkFolders = this->Folders->GetNumberOfItems();
  vtkKWLookmarkFolder *targetLmkFolder;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkPVLookmark *lookmarkWidget;
  vtkPVLookmark *targetLmkWidget;
  vtkIdType j=0;

  for(j=numberOfLookmarkFolders-1;j>=0;j--)
    {
    this->Folders->GetItem(j,lmkFolderWidget);
    if(target != lmkFolderWidget)
      {
      targetLmkWidget = vtkPVLookmark::SafeDownCast(target);
      if(targetLmkWidget)
        {
        lmkFolderWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkWidget->GetSeparatorFrame());
        }
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
    this->Lookmarks->GetItem(j,lookmarkWidget);
    if(target != lookmarkWidget)
      {
      targetLmkWidget = vtkPVLookmark::SafeDownCast(target);
      if(targetLmkWidget)
        {
        lookmarkWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkWidget->GetSeparatorFrame());
        }
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
          {
          lookmarkWidget->SetLocation(siblingLocation-1);
          }
        }
      }
    else if(sibling->IsA("vtkKWLookmarkFolder"))
      {
      lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(sibling);
      if(lmkFolderWidget)
        {
        siblingLocation = lmkFolderWidget->GetLocation();
        if(siblingLocation > locationOfLmkItemBeingRemoved)
          {
          lmkFolderWidget->SetLocation(siblingLocation-1);
          }
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
        {
        lookmarkWidget->SetLocation(siblingLocation+1); 
        }
      }
    else if(sibling->IsA("vtkKWLookmarkFolder"))
      {
      lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(sibling);
      siblingLocation = lmkFolderWidget->GetLocation();
      if(siblingLocation>=locationOfLmkItemBeingInserted)
        {
        lmkFolderWidget->SetLocation(siblingLocation+1);
        }
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
        if(this->Lookmarks->IsItemPresent(lookmarkWidget))
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
        if(this->Folders->IsItemPresent(lmkFolderWidget))
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
  numberOfItems = this->Lookmarks->GetNumberOfItems();
  
  while(i<=numberOfItems)
    {
    sprintf(name,"Lookmark%d",i);
    k=0;
    this->Lookmarks->GetItem(k,lmkWidget);
    while(k<numberOfItems && strcmp(name,lmkWidget->GetName())!=0)
      {
      this->Lookmarks->GetItem(++k,lmkWidget);
      }
    if(k==numberOfItems)
      {
      break;  //there was not match so this lmkname is valid
      }
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
  lmkFolderWidget->SetParent(this->ScrollFrame->GetFrame());
  lmkFolderWidget->Create(this->GetPVApplication());
  char methodAndArg[200];
  sprintf(methodAndArg,"SelectItemCallback %s",lmkFolderWidget->GetWidgetName());
  lmkFolderWidget->GetCheckbox()->SetCommand(this,methodAndArg);
  this->Script("pack %s -fill both -expand yes -padx 8",lmkFolderWidget->GetWidgetName());
  this->Script("%s configure -height 8",lmkFolderWidget->GetLabelFrame()->GetFrame()->GetWidgetName());
  lmkFolderWidget->SetFolderName("New Folder");

  // get the location index to assign the folder
  numItems = this->GetNumberOfChildLmkItems(this->ScrollFrame->GetFrame());
  lmkFolderWidget->SetLocation(numItems);

  numFolders = this->Folders->GetNumberOfItems();
  this->Folders->InsertItem(numFolders,lmkFolderWidget);

  this->ResetDragAndDropTargetSetAndCallbacks();

  this->Script("update");

  // Try to get the scroll bar to initialize properly (show correct portion).
  this->Script("%s yview moveto 1", 
               this->ScrollFrame->GetFrame()->GetParent()->GetWidgetName());

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
      if(this->Lookmarks->IsItemPresent(lmkWidget))
        {
        location++;
        }
      }
    else if(widget->IsA("vtkKWLookmarkFolder"))
      {
      lmkFolder = vtkKWLookmarkFolder::SafeDownCast(widget);
      if(this->Folders->IsItemPresent(lmkFolder))
        {
        location++;
        }
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
    if(!this->Folders->IsItemPresent(oldLmkFolder))
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

  if(nestedWidget->IsA("vtkKWLookmarkFolder"))
    {
    vtkKWLookmarkFolder *oldLmkFolder = vtkKWLookmarkFolder::SafeDownCast(nestedWidget);
    if(this->Folders->IsItemPresent(oldLmkFolder))
      {
      vtkKWLookmarkFolder *newLmkFolder = vtkKWLookmarkFolder::New();
      newLmkFolder->SetParent(packingFrame);
      newLmkFolder->Create(this->GetPVApplication());
      char methodAndArg[200];
      sprintf(methodAndArg,"SelectItemCallback %s",newLmkFolder->GetWidgetName());
      newLmkFolder->GetCheckbox()->SetCommand(this,methodAndArg);
      newLmkFolder->SetFolderName(oldLmkFolder->GetLabelFrame()->GetLabel()->GetText());
      newLmkFolder->SetMainFrameCollapsedState(oldLmkFolder->GetMainFrameCollapsedState());
      newLmkFolder->SetLocation(oldLmkFolder->GetLocation());
      this->Script("pack %s -fill both -expand yes -padx 8",newLmkFolder->GetWidgetName());

      this->Folders->FindItem(oldLmkFolder,loc);
      this->Folders->RemoveItem(loc);
      this->Folders->InsertItem(loc,newLmkFolder);

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

      this->PackChildrenBasedOnLocation(newLmkFolder->GetLabelFrame()->GetFrame());

      // deleting old folder
      this->RemoveItemAsDragAndDropTarget(oldLmkFolder);
      this->Script("destroy %s", oldLmkFolder->GetWidgetName());
      oldLmkFolder->Delete();
      }
    }
  else if(nestedWidget->IsA("vtkKWLookmark"))
    {
    vtkPVLookmark *oldLmkWidget = vtkPVLookmark::SafeDownCast(nestedWidget);
  
    if(this->Lookmarks->IsItemPresent(oldLmkWidget))
      {
      oldLmkWidget->UpdateVariableValues();
      vtkPVLookmark *newLmkWidget = vtkPVLookmark::New();
      newLmkWidget->SetMacroFlag(this->IsWidgetInsideFolder(packingFrame,this->GetMacrosFolder()));
      if(oldLmkWidget->GetMacroFlag())
        {
        this->GetPVWindow()->GetLookmarkToolbar()->RemoveWidget(oldLmkWidget->GetToolbarButton());
        }
      newLmkWidget->SetParent(packingFrame);
      newLmkWidget->Create(this->GetPVApplication());
      char methodAndArg[200];
      sprintf(methodAndArg,"SelectItemCallback %s",newLmkWidget->GetWidgetName());
      newLmkWidget->GetCheckbox()->SetCommand(this,methodAndArg);
      newLmkWidget->SetName(oldLmkWidget->GetName());
      newLmkWidget->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
      ostrstream s;
      if(newLmkWidget->GetName())
        {
        s << "GetPVLookmark \"" << newLmkWidget->GetName() << "\"" << ends;
        newLmkWidget->GetTraceHelper()->SetReferenceCommand(s.str());
        s.rdbuf()->freeze(0);
        }
      newLmkWidget->SetDataset(oldLmkWidget->GetDataset());
      newLmkWidget->CreateDatasetList();
      newLmkWidget->SetLocation(oldLmkWidget->GetLocation());
      newLmkWidget->SetComments(oldLmkWidget->GetComments());
      newLmkWidget->SetImageData(oldLmkWidget->GetImageData());
      newLmkWidget->SetPixelSize(oldLmkWidget->GetPixelSize());
      newLmkWidget->CreateIconFromImageData();
      newLmkWidget->SetStateScript(oldLmkWidget->GetStateScript());
      newLmkWidget->SetMainFrameCollapsedState(oldLmkWidget->GetMainFrameCollapsedState());
      newLmkWidget->SetCommentsFrameCollapsedState(oldLmkWidget->GetCommentsFrameCollapsedState());
      newLmkWidget->UpdateWidgetValues();
      newLmkWidget->CommentsModifiedCallback();
      this->Script("pack %s -fill both -expand yes -padx 8",newLmkWidget->GetWidgetName());

      this->Lookmarks->FindItem(oldLmkWidget,loc);
      this->Lookmarks->RemoveItem(loc);
      this->Lookmarks->InsertItem(loc,newLmkWidget);

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
    if(this->Folders->IsItemPresent(oldLmkFolder))
      {
      if(oldLmkFolder->GetSelectionState() || forceRemoveFlag)
        {
        this->RemoveItemAsDragAndDropTarget(oldLmkFolder);
        this->DecrementHigherSiblingLmkItemLocationIndices(
          oldLmkFolder->GetParent(),oldLmkFolder->GetLocation());
        this->Folders->FindItem(oldLmkFolder,loc);
        this->Folders->RemoveItem(loc);

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
  
    if(this->Lookmarks->IsItemPresent(oldLmkWidget))
      {
      if(oldLmkWidget->GetSelectionState() || forceRemoveFlag)
        {
        this->RemoveItemAsDragAndDropTarget(oldLmkWidget);
        this->DecrementHigherSiblingLmkItemLocationIndices(
          oldLmkWidget->GetParent(),oldLmkWidget->GetLocation());
        this->Lookmarks->FindItem(oldLmkWidget,loc);
        if(oldLmkWidget->GetMacroFlag())
          {
          this->GetPVWindow()->GetLookmarkToolbar()->RemoveWidget(oldLmkWidget->GetToolbarButton());
          }
        this->Lookmarks->RemoveItem(loc);
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

  this->PropagateEnableState(this->Menu);
  this->PropagateEnableState(this->CreateLookmarkButton);

  vtkIdType i,numberOfLookmarkWidgets, numberOfLookmarkFolders;
  vtkPVLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;

  numberOfLookmarkWidgets = this->Lookmarks->GetNumberOfItems();
  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->Lookmarks->GetItem(i,lookmarkWidget);
    this->PropagateEnableState(lookmarkWidget);
    }
  numberOfLookmarkFolders = this->Folders->GetNumberOfItems();
  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->Folders->GetItem(i,lmkFolderWidget);
    this->PropagateEnableState(lmkFolderWidget);
    }

}
 
//----------------------------------------------------------------------------
void vtkPVLookmarkManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TraceHelper: " << this->TraceHelper << endl;

}

