/*=========================================================================

  Program:   ParaView
  Module:    vtkPVItemSelection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVItemSelection - widget to select a set of data arrays.
// .SECTION Description
// vtkPVItemSelection is used for selecting which set of data arrays to 
// load when a reader has the ability to selectively load arrays.
// This class is able to deal with two different (though similar) type of domains: 
// vtkSMStringListDomain and vtkSMStringListRangeDomain

#ifndef __vtkPVItemSelection_h
#define __vtkPVItemSelection_h

#include "vtkPVWidget.h"

class vtkCollection;
class vtkDataArraySelection;
class vtkKWLabel;
class vtkKWFrameWithLabel;
class vtkKWPushButton;
class vtkPVItemSelectionArraySet;
class vtkKWFrame;

class VTK_EXPORT vtkPVItemSelection : public vtkPVWidget
{
public:
  static vtkPVItemSelection* New();
  vtkTypeRevisionMacro(vtkPVItemSelection, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
//BTX
  // Description:
  // Methods for setting the value of the VTKReader from the widget.
  // User internally when user hits Accept.
  virtual void Accept();
  virtual void PostAccept();
//ETX

  // Description:
  // Methods for setting the value of the widget from the VTKReader.
  // User internally when user hits Reset.
  virtual void ResetInternal();

  // Description:
  // Update our local vtkDataItemSelection instance with the reader's
  // settings.
  virtual void Initialize();

  // Description:
  // Used to change the label of the widget. If not specified,
  // the label is constructed using the AttributeName
  vtkSetStringMacro(LabelText);
  vtkGetStringMacro(LabelText);

  // Description:
  // Callback for the AllOn and AllOff buttons.
  void AllOnCallback();
  void AllOffCallback();

  // Description:
  // Access to change this widgets state from a script. Used for tracing.
  void SetArrayStatus(const char *name, int status);

  // Description:
  // Get the number of array names listed in this widget.
  int GetNumberOfArrays();
  
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVItemSelection* ClonePrototype(vtkPVSource* pvSource,
                                      vtkArrayMap<vtkPVWidget*,
                                      vtkPVWidget*>* map);
//ETX
  
  // Description:
  // Save this widget to a file. 
  // Ingore parts for thsi reader specific widget. 
  virtual void SaveInBatchScript(ofstream *file);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
  virtual void CheckButtonCallback(int);

protected:
  vtkPVItemSelection();
  ~vtkPVItemSelection();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  virtual void UpdateGUI();
  virtual void UpdateSelections(int fromReader);
  virtual void SetPropertyFromGUI();

  char* LabelText;
 
  vtkKWFrameWithLabel* LabeledFrame;
 
  vtkKWFrame* ButtonFrame;
  vtkKWPushButton* AllOnButton;
  vtkKWPushButton* AllOffButton;

  vtkKWFrame *CheckFrame;
  vtkCollection* ArrayCheckButtons;
  vtkKWLabel *NoArraysLabel;

  vtkDataArraySelection* Selection;
 
  vtkPVItemSelectionArraySet* ArraySet;

  const char* GetNameFromNumber(int num);
  int GetNumberFromName(const char* name, int* val);

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVItemSelection(const vtkPVItemSelection&); // Not implemented
  void operator=(const vtkPVItemSelection&); // Not implemented
};

#endif
