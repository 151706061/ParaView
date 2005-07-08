/*=========================================================================

  Module:    vtkPVSourceNotebook.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSourceNotebook - Specialized notebook for PVSources. 
// .SECTION Description
// The notebook creates a parameter, display and information page
// for sources.  Accept, Reset and Delete buttons are created on the
// Parameters page.  A display gui is created on the display page. 
// An information gui is created on the information page. 



#ifndef __vtkPVSourceNotebook_h
#define __vtkPVSourceNotebook_h

#include "vtkKWCompositeWidget.h"

class vtkKWApplication;
class vtkKWNotebook;
class vtkPVSource;
class vtkKWLabelWithLabel;
class vtkKWEntryWithLabel;
class vtkKWPushButton;
class vtkKWPushButtonWithMenu;
class vtkPVApplication;
class vtkPVInformationGUI;
class vtkPVDisplayGUI;
class vtkKWLabel;
class vtkKWFrame;

class VTK_EXPORT vtkPVSourceNotebook : public vtkKWCompositeWidget
{
public:
  static vtkPVSourceNotebook* New();
  vtkTypeRevisionMacro(vtkPVSourceNotebook,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  void Close();
    
  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Since thie GUI is shared, the call backs need a source
  // to modify.  The source sets this variable and calls update
  // when it becomes active.
  void SetPVSource(vtkPVSource *source);
  vtkGetObjectMacro(PVSource, vtkPVSource);
  void Update();
  
  // Description:
  // I am not exactly sure what this method does.  I know it
  // disables the delete button of the source is not deletable.
  // I assume that this works with PropagateSnableState and
  // disables a widget if any parent is disabled.
  void UpdateEnableStateWithSource(vtkPVSource* pvs);  
  void UpdateEnableState();  

  // Description:
  // This is so more specific updates can be called.
  // I do this because I suspect full updates are slow.
  vtkGetObjectMacro(DisplayGUI, vtkPVDisplayGUI);
  
  // Description:
  // Make the Accept button turn green/white when one of the parameters 
  // has changed.
  void SetAcceptButtonColorToModified();
  void SetAcceptButtonColorToUnmodified();
  vtkGetMacro(AcceptButtonRed, int);

  // Description:
  // This method is called when the user enters a label in the label entry.
  void LabelEntryCallback();

  // Description:
  // These are to setup the notebook when a new source before accept.
  // We could have a special state instead of these general methods.
  void Raise(const char* pageName);
  void HidePage(const char* pageName);
  void ShowPage(const char* pageName);

  // Description:
  // This is where the source will put its custom widgets.
  // Parent of sources parameter frames.
  vtkGetObjectMacro(MainParameterFrame, vtkKWFrame);

  // Description:
  // Just a safe down cast of application.
  vtkPVApplication* GetPVApplication();
  
  // Description:
  // Button callback methods.
  void AcceptButtonCallback();
  void ResetButtonCallback();
  void DeleteButtonCallback();
  
  // Description:
  // Popup menu callbacks.
  void SetAutoAccept(int val);
  vtkGetMacro(AutoAccept, int);
  void EventuallyAccept();
  void EventuallyAcceptCallBack();

  // Description:
  // Variable used to block the Accept during the initialization/clone of PVSource
  vtkSetMacro(CloneInitializeLock,int);
  vtkGetMacro(CloneInitializeLock,int);
  vtkBooleanMacro(CloneInitializeLock, int);

protected:
  vtkPVSourceNotebook();
  ~vtkPVSourceNotebook();

  vtkPVSource* PVSource;

  vtkKWNotebook* Notebook;
  vtkPVDisplayGUI* DisplayGUI;
  vtkPVInformationGUI* InformationGUI;
  vtkKWFrame *DescriptionFrame;
  vtkKWLabelWithLabel *NameLabel;
  vtkKWLabelWithLabel *TypeLabel;
  vtkKWEntryWithLabel *LabelEntry;
  vtkKWLabelWithLabel *LongHelpLabel;
  vtkKWFrame *ButtonFrame;
  vtkKWPushButtonWithMenu *AcceptButton;
  vtkKWPushButton *ResetButton;
  vtkKWPushButton *DeleteButton;
  vtkKWFrame *MainParameterFrame;

  vtkKWPushButton* AcceptPullDownArrow;
  
  // We have to manage updates separate from the VTK pipeline.
  int AcceptButtonRed;
  int AutoAccept;
  int CloneInitializeLock;  //Used to block the SetAcceptButtonColorToUnmodified
  Tcl_TimerToken TimerToken;

  // Description:
  // Change description interface to reflect a new source.
  void UpdateDescriptionFrame(vtkPVSource* pvs);

private:
  vtkPVSourceNotebook(const vtkPVSourceNotebook&); // Not implemented
  void operator=(const vtkPVSourceNotebook&); // Not implemented
};

#endif

