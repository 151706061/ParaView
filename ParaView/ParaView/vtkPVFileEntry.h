/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFileEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVFileEntry -
// .SECTION Description

#ifndef __vtkPVFileEntry_h
#define __vtkPVFileEntry_h

#include "vtkPVObjectWidget.h"

class vtkKWLabel;
class vtkKWPushButton;
class vtkKWEntry;
class vtkPVSource;
class vtkKWScale;
class vtkKWFrame;
class vtkPVFileEntryProperty;
class vtkKWListSelectOrder;
class vtkPVFileEntryObserver;
class vtkKWPopupButton;

//BTX
template<class KeyType,class DataType> class vtkArrayMap;
//ETX

class VTK_EXPORT vtkPVFileEntry : public vtkPVObjectWidget
{
public:
  static vtkPVFileEntry* New();
  vtkTypeRevisionMacro(vtkPVFileEntry, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The label can be set before or after create is called.
  void SetLabel(const char* label);
  const char* GetLabel();

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // This method allows scripts to modify the widgets value.
  virtual void SetValue(const char* fileName);
  const char* GetValue();

  // Description:
  // Called when the browse button is pressed.
  void BrowseCallback();

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // The extension used in the file dialog
  vtkSetStringMacro(Extension);
  vtkGetStringMacro(Extension);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVFileEntry* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  //BTX
  // Description:
  // Called when accept button is pushed.
  // Sets objects variable to the widgets value.
  // Side effect is to turn modified flag off.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void ResetInternal();
  
  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // This callback is called when entry changes.
  void EntryChangedCallback();

  // Description:
  // This callback is called when timestep changes.
  void TimestepChangedCallback();

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This method gets called when the user selects this widget to animate.
  // It sets up the script and animation parameters.
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai);


  // Description:
  // Set the current time step.
  void SetTimeStep(int ts);

  // Description:
  // Used internally. Method to save widget parameters into vtk tcl script.
  void SaveInBatchScriptForPart(ofstream* file, vtkClientServerID);

  // Description:
  // Get the range of files.

  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();

  // Description:
  // Get the number of files
  virtual int GetNumberOfFiles();

  // Description:
  // For event handling.
  void ExecuteEvent(vtkObject *o, unsigned long event, void* calldata);
  
protected:
  vtkPVFileEntry();
  ~vtkPVFileEntry();

  vtkKWLabel *LabelWidget;
  vtkKWPushButton *BrowseButton;
  vtkKWEntry *Entry;

  char* Extension;
  int InSetValue;

  // Timestep scale
  vtkKWFrame *TimestepFrame;
  vtkKWScale *Timestep;
  int TimeStep;

  vtkSetStringMacro(Path);
  char* Path;

  int IgnoreFileListEvents;

  vtkKWListSelectOrder* FileListSelect;
  vtkKWPopupButton* FileListPopup;

  vtkPVFileEntryProperty *Property;

  //BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
    vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  //ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
    vtkPVXMLPackageParser* parser);
  
  unsigned long ListObserverTag;
  vtkPVFileEntryObserver* Observer;

  void UpdateTimeStep();
  void UpdateAvailableFiles();

  int Initialized;

private:
  vtkPVFileEntry(const vtkPVFileEntry&); // Not implemented
  void operator=(const vtkPVFileEntry&); // Not implemented
};

#endif
