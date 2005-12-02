/*=========================================================================

  Module:    vtkKWTclInteractor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTclInteractor - a KW version of interactor.tcl
// .SECTION Description
// A widget to interactively execute Tcl commands

#ifndef __vtkKWTclInteractor_h
#define __vtkKWTclInteractor_h

#include "vtkKWTopLevel.h"

class vtkKWFrame;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWTextWithScrollbars;

class KWWIDGETS_EXPORT vtkKWTclInteractor : public vtkKWTopLevel
{
public:
  static vtkKWTclInteractor* New();
  vtkTypeRevisionMacro(vtkKWTclInteractor, vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create();
  
  // Description:
  // Append text to the display window. Can be used for sending
  // debugging information to the command prompt when no standard
  // output is available.
  virtual void AppendText(const char* text);

  // Description:
  // Evaluate the tcl string
  virtual void EvaluateCallback();

  // Description:
  // Callback for the down arrow key
  virtual void DownCallback();
  
  // Description:
  // Callback for the up arrow key
  virtual void UpCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWTclInteractor();
  ~vtkKWTclInteractor();

  vtkKWFrame      *ButtonFrame;
  vtkKWPushButton *DismissButton;
  vtkKWFrame      *CommandFrame;
  vtkKWLabel      *CommandLabel;
  vtkKWEntry      *CommandEntry;
  vtkKWTextWithScrollbars *DisplayText;
  
  int TagNumber;
  int CommandIndex;

private:
  vtkKWTclInteractor(const vtkKWTclInteractor&); // Not implemented
  void operator=(const vtkKWTclInteractor&); // Not implemented
};

#endif

