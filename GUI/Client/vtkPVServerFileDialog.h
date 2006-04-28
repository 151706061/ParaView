/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerFileDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerFileDialog - For opening remote files on server.
// .SECTION Description
// A dialog to replace Tk's Open and Save file dialogs.
// We will develop the dialog for local (opening) files first ...
// This creates a vtkSMServerFileListingProxy to obtain the information.
// This proxy is not registered with the ProxyManager.

#ifndef __vtkPVServerFileDialog_h
#define __vtkPVServerFileDialog_h

#include "vtkKWLoadSaveDialog.h"

class vtkIntArray;
class vtkKWApplication;
class vtkKWCanvas;
class vtkKWEntry;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWListBox;
class vtkKWMenuButton;
class vtkKWPushButton;
class vtkKWScrollbar;
class vtkKWWindow;
class vtkPVApplication;
class vtkStringList;
class vtkSMProxy;

class VTK_EXPORT vtkPVServerFileDialog : public vtkKWLoadSaveDialog
{
public:
  static vtkPVServerFileDialog* New();
  vtkTypeRevisionMacro(vtkPVServerFileDialog, vtkKWLoadSaveDialog);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Invoke the dialog, display it and enter an event loop until the user
  // confirm (OK) or cancel the dialog.
  // Note that a dialog is a modal toplevel by default.
  // This method returns a zero if the dialog was killed or 
  // canceled, nonzero otherwise.
  virtual int Invoke();
  
  // Description:
  // Confirm the action (load/save the file) and close this Dialog
  virtual void OK();

  // Description:
  // Button callbacks
  void SelectFile(const char* name, const char* id);
  void SelectDirectory(const char* name, const char* id);
  void DownDirectoryCallback();
  void ExtensionsMenuButtonCallback(int typeIdx);

  // Description:
  // Cast vtkKWApplication to vtkPVApplication.
  vtkPVApplication* GetPVApplication();

  // Description:
  // This method is called when canvas size changes.
  virtual void Reconfigure();

  // Description:
  //
  void AddDescriptionString(const char*);
  void AddExtensionString(const char*);

  // BTX
  // Description:
  // Get/Set the server from which to get the file listing.
  // Typically DATA_SERVER_ROOT or RENDER_SERVER_ROOT. By default, 
  // set to DATA_SERVER_ROOT. Must be set before calling Create().
  vtkSetMacro(Servers, vtkTypeUInt32);
  vtkGetMacro(Servers, vtkTypeUInt32);
  // ETX

protected:
  vtkPVServerFileDialog();
  ~vtkPVServerFileDialog();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  void Update();
  int Insert(const char* name, int y, int directory);

  // Get rid of backslashes.
  void ConvertLastPath();

  vtkKWFrame*      TopFrame;
  vtkKWFrame*       MiddleFrame;
  vtkKWCanvas*      FileList;
  vtkKWFrame*      BottomFrame;

  vtkKWLabel*       DirectoryDisplay;
  vtkKWMenuButton*  DirectoryMenuButton;

  vtkKWLabel*       FileNameLabel;
  vtkKWEntry*       FileNameEntry;
  vtkKWMenuButton*  FileNameMenuButton;

  vtkKWLabel*       ExtensionsLabel;
  vtkKWFrame*      ExtensionsDisplayFrame;
  vtkKWLabel*       ExtensionsDisplay;
  vtkKWMenuButton*  ExtensionsMenuButton;

  vtkKWPushButton*  LoadSaveButton;
  vtkKWPushButton*  CancelButton;

  vtkKWLabel*       DownDirectoryButton;
    
  char*             SelectBoxId;
  vtkSetStringMacro(SelectBoxId);

  char*             SelectedDirectory;
  vtkSetStringMacro(SelectedDirectory);

  void UpdateExtensionsMenu();
  vtkStringList*    FileTypeStrings;
  vtkStringList*    FileTypeDescriptions;
  vtkStringList*    ExtensionStrings;
  int               CheckExtension(const char* name);

  // Server-side helper.
  void CreateServerSide();

  vtkSMProxy* ServerFileListingProxy;
  // BTX
  vtkTypeUInt32 Servers;
  // ETX

  vtkKWScrollbar* ScrollBar;

  // Description:
  // This method calculates the bounding box of object "name". 
  void CalculateBBox(vtkKWWidget* canvas, const char* name, int bbox[4]);

private:
  vtkPVServerFileDialog(const vtkPVServerFileDialog&); // Not implemented
  void operator=(const vtkPVServerFileDialog&); // Not implemented
};

#endif
