/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoundsDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBoundsDisplay - an entry with a label
// .SECTION Description
// The LabeledEntry creates an entry with a label in front of it; both are
// contained in a frame


#ifndef __vtkPVBoundsDisplay_h
#define __vtkPVBoundsDisplay_h

#include "vtkPVWidget.h"

class vtkKWApplication;
class vtkKWBoundsDisplay;
class vtkPVInputMenu;

class VTK_EXPORT vtkPVBoundsDisplay : public vtkPVWidget
{
public:
  static vtkPVBoundsDisplay* New();
  vtkTypeRevisionMacro(vtkPVBoundsDisplay, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);

  // Description:
  // The bounds display gets its data object from the input menu.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);

  // Description:
  // This calculates new bounds to display (using the input menu).
  virtual void Update();

  // Description:
  // Access to the KWWidget.  
  virtual void SetWidget(vtkKWBoundsDisplay*);
  vtkGetObjectMacro(Widget, vtkKWBoundsDisplay);

  // Description:
  // Set / get ShowHide for this object.
  vtkSetMacro(ShowHideFrame, int);
  vtkBooleanMacro(ShowHideFrame, int);
  vtkGetMacro(ShowHideFrame, int);

  // Description:
  // Set / get label for this object.
  void SetLabel(const char* label);
  const char* GetLabel();
  
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVBoundsDisplay* 
    ClonePrototype(vtkPVSource* pvSource,
                   vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Empty method to keep superclass from complaining.
  virtual void SaveInBatchScript(ofstream*) {};

  // Description:
  // Empty method to keep superclass from complaining.
  virtual void Trace(ofstream*) {};

protected:
  vtkPVBoundsDisplay();
  ~vtkPVBoundsDisplay();

  int ShowHideFrame;
  vtkKWBoundsDisplay *Widget;
  vtkPVInputMenu *InputMenu;

  vtkPVBoundsDisplay(const vtkPVBoundsDisplay&); // Not implemented
  void operator=(const vtkPVBoundsDisplay&); // Not implemented

  vtkGetStringMacro(FrameLabel);
  vtkSetStringMacro(FrameLabel);

  char* FrameLabel;

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  vtkPVWidget* ClonePrototypeInternal(
    vtkPVSource* pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};


#endif
