/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLODRenderModuleUI - Default serial render module.
// .SECTION Description
// This render module uses quadric clustering to create a single level of detail.


#ifndef __vtkPVLODRenderModuleUI_h
#define __vtkPVLODRenderModuleUI_h

#include "vtkPVRenderModuleUI.h"

class vtkKWLabel;
class vtkKWPushButton;
class vtkKWRadioButton;
class vtkKWLabeledFrame;
class vtkKWScale;
class vtkKWCheckButton;
class vtkKWSplitFrame;
class vtkLabeledFrame;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVCameraIcon;
class vtkPVData;
class vtkPVInteractorStyleControl;
class vtkPVLODRenderModuleUIObserver;
class vtkPVSource;
class vtkPVSourceList;
class vtkPVSourcesNavigationWindow;
class vtkPVTreeComposite;
class vtkPVWindow;
class vtkPVRenderModule;
class vtkPVLODRenderModule;

class VTK_EXPORT vtkPVLODRenderModuleUI : public vtkPVRenderModuleUI
{
public:
  static vtkPVLODRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVLODRenderModuleUI,vtkPVRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Create the TK widgets associated with the view.
  virtual void Create(vtkKWApplication *app, const char *);
  
  // Description:
  // The subclass should implement this method and 
  // downcast it to the right type.  It can then access
  // any unique methods of the specific render module.
  virtual void SetRenderModule(vtkPVRenderModule *rm);
  
  // Description:
  // Callback for the interrupt render check button
  void RenderInterruptsEnabledCheckCallback();
  void SetRenderInterruptsEnabled(int state);
  
  // Description:
  // Threshold for individual actors as number of points.
  void LODThresholdScaleCallback();
  void LODThresholdLabelCallback();
  void LODCheckCallback();

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetLODThreshold(float);
  vtkGetMacro(LODThreshold, float);
  vtkBooleanMacro(LODThreshold, float);

  // Description:
  // LOD resolution determines how many cells are in decimated model.
  void LODResolutionScaleCallback();
  void LODResolutionLabelCallback();

  // Description:
  // This method sets the resolution without tracing or
  // changing the UI scale.
  void SetLODResolutionInternal(int threshold);

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetLODResolution(int);
  vtkGetMacro(LODResolution, int);
  vtkBooleanMacro(LODResolution, int);

  // Description:
  // Export the render module state to a file.
  virtual void SaveState(ofstream *file);
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkPVLODRenderModuleUI();
  ~vtkPVLODRenderModuleUI();
 
  vtkPVLODRenderModule* LODRenderModule;

  int UseReductionFactor;
  
  vtkKWLabeledFrame *LODFrame;
  vtkKWCheckButton *RenderInterruptsEnabledCheck;

  vtkKWWidget*      LODScalesFrame;
  vtkKWLabel*       LODResolutionLabel;
  vtkKWScale*       LODResolutionScale;
  vtkKWLabel*       LODResolutionValue;
  vtkKWLabel*       LODThresholdLabel;
  vtkKWCheckButton* LODCheck;
  vtkKWScale*       LODThresholdScale;
  vtkKWLabel*       LODThresholdValue;

  float LODThreshold;
  int   LODResolution;
  int   RenderInterruptsEnabled;

  vtkPVLODRenderModuleUI(const vtkPVLODRenderModuleUI&); // Not implemented
  void operator=(const vtkPVLODRenderModuleUI&); // Not implemented
};


#endif
