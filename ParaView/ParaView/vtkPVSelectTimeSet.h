/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSelectTimeSet.h
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
// .NAME vtkPVSelectTimeSet - Special time selection widget used by PVEnSightReaderModule
// .SECTION Description
// This is a PVWidget specially designed to be used with PVEnSightReaderModule.
// It provides support for multiple sets. The time value selected by
// the user is passed to the EnSight reader with a SetTimeValue() call.

#ifndef __vtkPVSelectTimeSet_h
#define __vtkPVSelectTimeSet_h

#include "vtkPVObjectWidget.h"

class vtkKWLabel;
class vtkKWMenu;
class vtkKWLabeledFrame;
class vtkDataArrayCollection;
class vtkPVScalarListWidgetProperty;

class VTK_EXPORT vtkPVSelectTimeSet : public vtkPVObjectWidget
{
public:
  static vtkPVSelectTimeSet* New();
  vtkTypeRevisionMacro(vtkPVSelectTimeSet, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Adds a trace entry.  Side effect is to turn modified flag off.
  virtual void AcceptInternal(vtkClientServerID);
  
  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void ResetInternal();

  // Description:
  // Adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // Called whenthe animation method menu item is selected.
  // Needed for proper tracing.
  // It would be nice if the menu and cascade menus would trace
  // invokation of items (?relying of enumeration of menu items or label?)
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This is the labeled frame around the timeset tree.
  vtkGetObjectMacro(LabeledFrame, vtkKWLabeledFrame);

  // Description:
  // Label displayed on the labeled frame.
  void SetLabel(const char* label);
  const char* GetLabel();

  // Description:
  // Updates the time value label and the time ivar.
  void SetTimeValue(float time);
  vtkGetMacro(TimeValue, float);

  // Description:
  // Calls this->SetTimeValue () and Reader->SetTimeValue()
  // with currently selected time value.
  void SetTimeValueCallback(const char* item);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVSelectTimeSet* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
  // Description:
  // Set/get the command to pass the value to VTK.
  vtkSetStringMacro(SetCommand);
  vtkGetStringMacro(SetCommand);
  
protected:
  vtkPVSelectTimeSet();
  ~vtkPVSelectTimeSet();

  vtkPVSelectTimeSet(const vtkPVSelectTimeSet&); // Not implemented
  void operator=(const vtkPVSelectTimeSet&); // Not implemented

  vtkPVScalarListWidgetProperty *Property;

  char *SetCommand;
  
  vtkSetStringMacro(FrameLabel);
  vtkGetStringMacro(FrameLabel);

  vtkKWWidget* Tree;
  vtkKWWidget* TreeFrame;
  vtkKWLabel* TimeLabel;
  vtkKWLabeledFrame* LabeledFrame;

  void AddRootNode(const char* name, const char* text);
  void AddChildNode(const char* parent, const char* name, 
                    const char* text, const char* data);

  float TimeValue;
  char* FrameLabel;

  vtkDataArrayCollection* TimeSets;
  vtkSetStringMacro(TimeSetsTclName);
  char* TimeSetsTclName;
  
  // Set TimeSetsTclName with the tcl name of the TimeSets instance.
  void SetupTimeSetsTclName();
  
  // Fill the TimeSets collection with that from the actual reader.
  void SetTimeSetsFromReader();

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // An interface for saving a widget into a script.
  virtual void SaveInBatchScriptForPart(ofstream *file, vtkClientServerID);

};

#endif
