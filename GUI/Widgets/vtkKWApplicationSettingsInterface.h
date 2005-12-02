/*=========================================================================

  Module:    vtkKWApplicationSettingsInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWApplicationSettingsInterface - a user interface panel.
// .SECTION Description
// A concrete implementation of a user interface panel.
// See vtkKWUserInterfacePanel for a more detailed description.
// .SECTION See Also
// vtkKWUserInterfacePanel vtkKWUserInterfaceManager 

#ifndef __vtkKWApplicationSettingsInterface_h
#define __vtkKWApplicationSettingsInterface_h

#include "vtkKWUserInterfacePanel.h"

class vtkKWCheckButton;
class vtkKWFrame;
class vtkKWFrameWithLabel;
class vtkKWPushButton;
class vtkKWWindow;
class vtkKWMenuButtonWithLabel;

class KWWIDGETS_EXPORT vtkKWApplicationSettingsInterface : public vtkKWUserInterfacePanel
{
public:
  static vtkKWApplicationSettingsInterface* New();
  vtkTypeRevisionMacro(vtkKWApplicationSettingsInterface,vtkKWUserInterfacePanel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the window (do not ref count it since the window will ref count
  // this widget).
  vtkGetObjectMacro(Window, vtkKWWindow);
  virtual void SetWindow(vtkKWWindow*);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Refresh the interface given the current value of the Window and its
  // views/composites/widgets.
  virtual void Update();

  // Description:
  // Callback used when interaction has been performed.
  virtual void ConfirmExitCallback();
  virtual void SaveUserInterfaceGeometryCallback();
  virtual void SplashScreenVisibilityCallback();
  virtual void BalloonHelpVisibilityCallback();
  virtual void ResetDragAndDropCallback();
  virtual void FlatFrameCallback();
  virtual void FlatButtonsCallback();
  virtual void DPICallback(double dpi);
  virtual void ViewPanelPositionCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Some constants
  //BTX
  static const char *PrintSettingsLabel;
  //ETX

protected:
  vtkKWApplicationSettingsInterface();
  ~vtkKWApplicationSettingsInterface();

  vtkKWWindow       *Window;

  // Interface settings

  vtkKWFrameWithLabel *InterfaceSettingsFrame;

  vtkKWCheckButton  *ConfirmExitCheckButton;
  vtkKWCheckButton  *SaveUserInterfaceGeometryCheckButton;
  vtkKWCheckButton  *SplashScreenVisibilityCheckButton;
  vtkKWCheckButton  *BalloonHelpVisibilityCheckButton;
  vtkKWMenuButtonWithLabel *ViewPanelPositionOptionMenu;

  // Interface customization

  vtkKWFrameWithLabel *InterfaceCustomizationFrame;
  vtkKWPushButton   *ResetDragAndDropButton;

  // Toolbar settings

  vtkKWFrameWithLabel *ToolbarSettingsFrame;
  vtkKWCheckButton  *FlatFrameCheckButton;
  vtkKWCheckButton  *FlatButtonsCheckButton;

  // Print settings

  vtkKWFrameWithLabel      *PrintSettingsFrame;
  vtkKWMenuButtonWithLabel *DPIOptionMenu;

private:
  vtkKWApplicationSettingsInterface(const vtkKWApplicationSettingsInterface&); // Not implemented
  void operator=(const vtkKWApplicationSettingsInterface&); // Not Implemented
};

#endif


