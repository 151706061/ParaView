/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVLineWidget.h
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
// .NAME vtkPVLineWidget -
// .SECTION Description

// Todo:
// Cleanup GUI:
//       * Visibility
//       * Resolution
//       *

#ifndef __vtkPVLineWidget_h
#define __vtkPVLineWidget_h

#include "vtkPV3DWidget.h"

class vtkKWEntry;
class vtkKWLabel;
class vtkLineWidget;

class VTK_EXPORT vtkPVLineWidget : public vtkPV3DWidget
{
public:
  static vtkPVLineWidget* New();
  vtkTypeRevisionMacro(vtkPVLineWidget, vtkPV3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Adds a trace entry.  Side effect is to turn modified flag off.
  virtual void Accept();
  virtual void Accept(const char* sourceTclName);
  
  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void Reset();
  virtual void Reset(const char* sourceTclName);

  // Description:
  // Callbacks to set the points of the 3D widget from the
  // entry values. Bound to <KeyPress-Return>.
  void SetPoint1();
  void SetPoint2();
  void SetPoint1(float x, float y, float z);
  void SetPoint2(float x, float y, float z);
  void GetPoint1(float pt[3]);
  void GetPoint2(float pt[3]);

  // Description:
  // Set the resolution of the line widget.
  void SetResolution();
  void SetResolution(int f);
  int GetResolution();

  // Description:
  // Set the tcl variables that are modified when accept is called.
  void SetPoint1VariableName(const char* varname);
  void SetPoint2VariableName(const char* varname);
  void SetResolutionVariableName(const char* varname);

  vtkGetStringMacro(Point1Variable);
  vtkGetStringMacro(Point2Variable);
  vtkGetStringMacro(ResolutionVariable);

  // Description:
  // Set the tcl labels that are modified when accept is called.
  void SetPoint1LabelTextName(const char* varname);
  void SetPoint2LabelTextName(const char* varname);
  void SetResolutionLabelTextName(const char* varname);

  // Description:
  // Labels for entries
  vtkGetStringMacro(Point1LabelText);
  vtkGetStringMacro(Point2LabelText);
  vtkGetStringMacro(ResolutionLabelText);

  // Description:
  // Determines whether the Resolution entry is shown.
  vtkGetMacro(ShowResolution, int);
  vtkSetMacro(ShowResolution, int);
  vtkBooleanMacro(ShowResolution, int);

  // Description:
  // This method does the actual placing. It sets the first point at 
  // (bounds[0]+bounds[1])/2, bounds[2], (bounds[4]+bounds[5])/2
  // and the second point at
  // (bounds[0]+bounds[1])/2, bounds[3], (bounds[4]+bounds[5])/2
  virtual void ActualPlaceWidget();
    
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVLineWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file, const char *root);

protected:
  vtkPVLineWidget();
  ~vtkPVLineWidget();
  
//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);

  friend class vtkLineWidgetObserver;
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  void UpdateVTKObject(const char* sourceTclName);

  vtkKWEntry* Point1[3];
  vtkKWEntry* Point2[3];
  vtkKWLabel* Labels[2];
  vtkKWLabel* CoordinateLabel[3];
  vtkKWLabel* ResolutionLabel;
  vtkKWEntry* ResolutionEntry;

  vtkSetStringMacro(Point1Variable);
  vtkSetStringMacro(Point2Variable);
  vtkSetStringMacro(ResolutionVariable);

  char *Point1Variable;
  char *Point2Variable;
  char *ResolutionVariable;

  vtkSetStringMacro(Point1LabelText);
  vtkSetStringMacro(Point2LabelText);
  vtkSetStringMacro(ResolutionLabelText);

  char *Point1LabelText;
  char *Point2LabelText;
  char *ResolutionLabelText;

  int ShowResolution;

  // Description:
  // Used internally. Method to save widget parameters into vtk tcl script.
  virtual void SaveInBatchScriptForPart(ofstream *file, const char* sourceTclName);

private:  
  vtkPVLineWidget(const vtkPVLineWidget&); // Not implemented
  void operator=(const vtkPVLineWidget&); // Not implemented
};

#endif
