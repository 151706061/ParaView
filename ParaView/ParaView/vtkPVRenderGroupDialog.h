/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRenderGroupDialog.h
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
// .NAME vtkPVRenderGroupDialog - Shows a text version of the timer log entries.
// .SECTION Description
// A widget to display timing information in the timer log.

#ifndef __vtkPVRenderGroupDialog_h
#define __vtkPVRenderGroupDialog_h

#include "vtkKWWidget.h"
class vtkKWApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWWindow;
class vtkKWEntry;
class vtkKWCheckButton;

class VTK_EXPORT vtkPVRenderGroupDialog : public vtkKWWidget
{
public:
  static vtkPVRenderGroupDialog* New();
  vtkTypeRevisionMacro(vtkPVRenderGroupDialog, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Display the interactor
  void Invoke();

  // Description:
  // Callback from the dismiss button that closes the window.
  void Accept();

  // Description:
  // Set the title of the TclInteractor to appear in the titlebar
  vtkSetStringMacro(Title);
  
  // Description:
  // Set the window to which the dialog will be slave.
  // If set, this dialog will always be on top of the master
  // window and will minimize with it (assuming that the windowing
  // system supports this)
  void SetMasterWindow(vtkKWWindow* win);
  
  // Description:
  // Access to the result of the dialog.
  void SetNumberOfProcessesInGroup(int val);
  vtkGetMacro(NumberOfProcessesInGroup, int);

  // Description:
  // A callbacks from the UI.
  void NumberEntryCallback();

  // Description:
  // Initialize the display strings, or Get the desplay strings
  // Chosen by the user.  The first string cannot b e modified
  // by the user.  The display strings entry is not created
  // unless the first display string is initialized.
  void SetDisplayString(int idx, const char* str);
  const char* GetDisplayString(int idx); 

protected:
  vtkPVRenderGroupDialog();
  ~vtkPVRenderGroupDialog();

  // Returns 1 if first display is OK. 0 if user has modified the display.
  void Update();
  void ComputeDisplayStringRoot(const char* str);

  void Append(const char*);
  
  vtkKWWindow*      MasterWindow;

  vtkKWWidget*      ControlFrame;
  vtkKWPushButton*  SaveButton;
  vtkKWPushButton*  ClearButton;
  vtkKWLabel*       NumberLabel;
  vtkKWEntry*       NumberEntry;

  int               DisplayFlag;
  vtkKWWidget*      DisplayFrame;
  vtkKWLabel*       Display0Label;
  vtkKWEntry**      DisplayEntries;
  char*             DisplayStringRoot;

  vtkKWWidget*      ButtonFrame;
  vtkKWPushButton*  AcceptButton;
  int AcceptedFlag;

    
  char*   Title;
  int     Writable;
  int     NumberOfProcessesInGroup;

private:
  vtkPVRenderGroupDialog(const vtkPVRenderGroupDialog&); // Not implemented
  void operator=(const vtkPVRenderGroupDialog&); // Not implemented
};

#endif
