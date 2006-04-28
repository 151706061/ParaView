/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVCRControl.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVVCRControl - Toolbar for the VCR control.
// .SECTION Description
// Toolbar for the vcr buttons. This control has 3 modes.
// PLAYBACK :- only buttons for playback are shown.
// RECORD:- only buttons for recording are shown.
// BOTH:- both playback and recording buttons are shown.
// Note that the mode must be set before calling Create.

#ifndef __vtkPVVCRControl_h
#define __vtkPVVCRControl_h

#include "vtkKWToolbar.h"

class vtkKWPushButton;
class vtkKWCheckButton;

class VTK_EXPORT vtkPVVCRControl : public vtkKWToolbar
{
public:
  static vtkPVVCRControl* New();
  vtkTypeRevisionMacro(vtkPVVCRControl, vtkKWToolbar);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the callbacks.
  void SetPlayCommand(vtkKWObject* calledObject, const char* commandString);
  void SetStopCommand(vtkKWObject* calledObject, const char* commandString);
  void SetGoToBeginningCommand(vtkKWObject* calledObject, const char* commandString);
  void SetGoToEndCommand(vtkKWObject* calledObject, const char* commandString);
  void SetGoToPreviousCommand(vtkKWObject* calledObject, const char* commandString);
  void SetGoToNextCommand(vtkKWObject* calledObject, const char* commandString);
  void SetLoopCheckCommand(vtkKWObject* calledObject, const char* commandString);
  void SetRecordCheckCommand(vtkKWObject* calledObject, const char* commandString);
  void SetRecordStateCommand(vtkKWObject* calledObject, const char* commandString);
  void SetSaveAnimationCommand(vtkKWObject* calledObject, const char* commandString);

  // Description:
  // Create the widget.
  virtual void UpdateEnableState();

  // Description:
  // Get/Set if animation is Playing.
  vtkSetMacro(InPlay, int);
  vtkGetMacro(InPlay, int);

  void SetLoopButtonState(int state);
  int GetLoopButtonState();

  void SetRecordCheckButtonState(int state);
  int GetRecordCheckButtonState();
    
  // Description:
  // Internal callbacks for the buttons.
  void PlayCallback();
  void StopCallback();
  void GoToBeginningCallback();
  void GoToEndCallback();
  void GoToPreviousCallback();
  void GoToNextCallback();
  void LoopCheckCallback(int);
  void RecordCheckCallback(int);
  void RecordStateCallback();
  void SaveAnimationCallback();

  // Description:
  // VCR Control can have 3 modes,
  // PLAYBACK, RECORD, BOTH.
  // The mode must be set before calling Create.
  vtkSetMacro(Mode, int);
  vtkGetMacro(Mode, int);
  void SetModeToPlayBack() { this->SetMode(vtkPVVCRControl::PLAYBACK); }
  void SetModeToRecord() { this->SetMode(vtkPVVCRControl::RECORD); }
  void SetModeToBoth() { this->SetMode(vtkPVVCRControl::BOTH); }
//BTX
  enum {
    PLAYBACK,
    RECORD,
    BOTH
  };
//ETX
  
protected:
  vtkPVVCRControl();
  ~vtkPVVCRControl();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  int InPlay; // used to decide enable state of the buttons.
  vtkKWPushButton *PlayButton;
  vtkKWPushButton *StopButton;
  vtkKWPushButton *GoToBeginningButton;
  vtkKWPushButton *GoToEndButton;
  vtkKWPushButton *GoToPreviousButton;
  vtkKWPushButton *GoToNextButton;
  vtkKWCheckButton *LoopCheckButton;
  vtkKWCheckButton *RecordCheckButton;
  vtkKWPushButton *RecordStateButton;
  vtkKWPushButton *SaveAnimationButton;

  char* PlayCommand;
  char* StopCommand;
  char* GoToBeginningCommand;
  char* GoToEndCommand;
  char* GoToPreviousCommand;
  char* GoToNextCommand;
  char* LoopCheckCommand;
  char* RecordCheckCommand;
  char* RecordStateCommand;
  char* SaveAnimationCommand;

  int Mode;

  vtkSetStringMacro(PlayCommand);
  vtkSetStringMacro(StopCommand);
  vtkSetStringMacro(GoToBeginningCommand);
  vtkSetStringMacro(GoToEndCommand);
  vtkSetStringMacro(GoToPreviousCommand);
  vtkSetStringMacro(GoToNextCommand);
  vtkSetStringMacro(LoopCheckCommand);
  vtkSetStringMacro(RecordCheckCommand);
  vtkSetStringMacro(RecordStateCommand);
  vtkSetStringMacro(SaveAnimationCommand);

  void InvokeCommand(const char* command);
private:
  vtkPVVCRControl(const vtkPVVCRControl&); // Not implemented.
  void operator=(const vtkPVVCRControl&); // Not implemented.
};

#endif

