/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtentEntry.h
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
// .NAME vtkPVExtentEntry -
// .SECTION Description
// Although I could make this a subclass of vtkPVVector Entry,
// Vector entry is too general, and some inherited method may be confusing.
// The resaon I created this class is to get a special behavior
// for animations.

#ifndef __vtkPVExtentEntry_h
#define __vtkPVExtentEntry_h

#include "vtkPVObjectWidget.h"

class vtkKWEntry;
class vtkKWLabel;

class VTK_EXPORT vtkPVExtentEntry : public vtkPVObjectWidget
{
public:
  static vtkPVExtentEntry* New();
  vtkTypeRevisionMacro(vtkPVExtentEntry, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Create(vtkKWApplication *pvApp);
  
  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Adds a trace entry.  Side effect is to turn modified flag off.
  virtual void Accept();
  
  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void Reset();

  // Description:
  // Methods to set this widgets value from a script.
  void SetValue(int v1, int v2, int v3, int v4, int v5, int v6);
  
  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterface *ai);

  // Description:
  // This method gets called when the user selects this widget to animate.
  // It sets up the script and animation parameters.
  void AnimationMenuCallback(vtkPVAnimationInterface *ai, int Mode);

  // Description:
  // The label.
  void SetLabel(const char* label);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVExtentEntry* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

protected:
  vtkPVExtentEntry();
  ~vtkPVExtentEntry();
  
  vtkKWLabel *LabelWidget;

  vtkKWEntry *XMinEntry;
  vtkKWEntry *XMaxEntry;
  vtkKWEntry *YMinEntry;
  vtkKWEntry *YMaxEntry;
  vtkKWEntry *ZMinEntry;
  vtkKWEntry *ZMaxEntry;


  vtkSetStringMacro(EntryLabel);
  vtkGetStringMacro(EntryLabel);
  char* EntryLabel;

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVExtentEntry(const vtkPVExtentEntry&); // Not implemented
  void operator=(const vtkPVExtentEntry&); // Not implemented
};

#endif
