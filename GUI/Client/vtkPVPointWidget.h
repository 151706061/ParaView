/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPointWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPointWidget - A widget to manipulate an implicit plane.
// .SECTION Description
// This widget creates and manages its own vtkPlane on each process.
// I could not descide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVPointWidget_h
#define __vtkPVPointWidget_h

#include "vtkPV3DWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;

class VTK_EXPORT vtkPVPointWidget : public vtkPV3DWidget
{
public:
  static vtkPVPointWidget* New();
  vtkTypeRevisionMacro(vtkPVPointWidget, vtkPV3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Callback that set the center to the middle of the bounds.
  void PositionResetCallback();

  // Description:
  // This method sets the input to the 3D widget and places the widget.
  virtual void ActualPlaceWidget();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVPointWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  void SetPosition();
  void SetPosition(double,double,double);
  void SetPositionInternal(double,double,double);
  void GetPosition(double pt[3]);

  // Description:
  // For saving the widget into a VTK tcl script.
  virtual void SaveInBatchScriptForPart(ofstream *file, vtkClientServerID);

  // Description:
  // Called when the PVSources reset button is called.
  virtual void ResetInternal();

  //BTX
  // Description:
  // Called when the PVSources accept button is called.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

protected:
  vtkPVPointWidget();
  ~vtkPVPointWidget();


  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  void UpdateVTKObject();

  vtkKWEntry *PositionEntry[3];
  vtkKWPushButton *PositionResetButton;

  vtkKWLabel* Labels[2];
  vtkKWLabel* CoordinateLabel[3];

  double LastAcceptedPosition[3];
  vtkSetVector3Macro(LastAcceptedPosition, double);
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVPointWidget(const vtkPVPointWidget&); // Not implemented
  void operator=(const vtkPVPointWidget&); // Not implemented
};

#endif
