/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPointSourceWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPointSourceWidget -  a PointWidget which contains a separate point source
// This widget contains a vtkPointWidget as well as a vtkPointSource. 
// This vtkPointSource (which is created on all processes) can be used as 
// input or source to filters (for example as streamline seed).
//
// If an InputMenu is specified, then DefaultRadius has no effect.  If
// InputMenu is not specified, then RadiusScaleFactor has no effect.

#ifndef __vtkPVPointSourceWidget_h
#define __vtkPVPointSourceWidget_h

#include "vtkPVPointWidget.h"

class vtkPVInputMenu;
class vtkPVScaleFactorEntry;
class vtkPVVectorEntry;
class vtkSMSourceProxy;

class VTK_EXPORT vtkPVPointSourceWidget : public vtkPVPointWidget
{
public:
  static vtkPVPointSourceWidget* New();
  vtkTypeRevisionMacro(vtkPVPointSourceWidget, vtkPVPointWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Controls the radius of the point cloud.
  vtkGetObjectMacro(RadiusWidget, vtkPVScaleFactorEntry);

  // Description:
  // Controls the number of points in the point cloud.
  vtkGetObjectMacro(NumberOfPointsWidget, vtkPVVectorEntry);

  // Description:
  // Returns if any subwidgets are modified.
  virtual int GetModifiedFlag();

  // Description:
  // Create the point source in the VTK Tcl script.
  // Savea a point source (one for all parts).
  virtual void SaveInBatchScript(ofstream *file);

  //BTX
  // Description:
  // The methods get called when the Accept button is pressed. 
  // It sets the VTK objects value using this widgets value.
  virtual void Accept();
  //ETX

  // Description:
  // The methods get called when the Reset button is pressed. 
  // It sets this widgets value using the VTK objects value.
  virtual void ResetInternal();

  // Description:
  // Initialize widget after creation
  virtual void Initialize();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Values to be set from XML.
  vtkSetMacro(RadiusScaleFactor, float);
  vtkSetMacro(DefaultRadius, float);
  vtkSetMacro(DefaultNumberOfPoints, int);
  vtkSetMacro(ShowEntries, int);
  vtkSetMacro(BindRadiusToInput, int);
  void SetInputMenu(vtkPVInputMenu *im);

  // Description:
  // This is called if the input menu changes.
  virtual void Update();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
  virtual vtkSMProxy* GetProxyByName(const char*)
    { return reinterpret_cast<vtkSMProxy*>(this->SourceProxy); }
  
  // Description:
  // Register the animatable proxies and make them available for animation.
  // Called by vtkPVSelectWidget when the widget is selected. This
  // is to make sure that only the selected widget shows up in the
  // animation interface and thus avoids confusion.
  virtual void EnableAnimation();

  // Description:
  // Unregister animatable proxies so that they are not available for
  // animation. Called by vtkPVSelectWidget when this widget is deselected.
  // is to make sure that only the selected widget shows up in the
  // animation interface and thus avoids confusion.
  virtual void DisableAnimation();

protected:
  vtkPVPointSourceWidget();
  ~vtkPVPointSourceWidget();

//BTX
  virtual void CopyProperties(vtkPVWidget *clone, vtkPVSource *pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  virtual int ReadXMLAttributes(vtkPVXMLElement *element,
                                vtkPVXMLPackageParser *parser);

  //vtkPVPointWidget* PointWidget;
  vtkSMSourceProxy *SourceProxy;
  char *SourceProxyName;
  vtkSetStringMacro(SourceProxyName);

  vtkPVScaleFactorEntry* RadiusWidget;
  vtkPVVectorEntry* NumberOfPointsWidget;

  float RadiusScaleFactor;
  float DefaultRadius;
  vtkPVInputMenu *InputMenu;
  int DefaultNumberOfPoints;
  int ShowEntries;
  int BindRadiusToInput;
  
  vtkPVPointSourceWidget(const vtkPVPointSourceWidget&); // Not implemented
  void operator=(const vtkPVPointSourceWidget&); // Not implemented

};

#endif
