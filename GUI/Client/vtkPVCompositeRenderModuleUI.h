/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeRenderModuleUI - UI for composite options..
// .SECTION Description
// This render module user interface control compositing.


#ifndef __vtkPVCompositeRenderModuleUI_h
#define __vtkPVCompositeRenderModuleUI_h

#include "vtkPVLODRenderModuleUI.h"

class vtkPVCompositeRenderModule;

class VTK_EXPORT vtkPVCompositeRenderModuleUI : public vtkPVLODRenderModuleUI
{
public:
  static vtkPVCompositeRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVCompositeRenderModuleUI,vtkPVLODRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is called right before the application starts its
  // main loop.  It was created to disable compositing after the 
  // server information in the process module is valid.
  virtual void Initialize();
  
  // Description:
  // Create the widget.
  virtual void Create();
      
  // Description:
  // Callback for the use char check button.  
  // The ones without parameters are only public because they are callbacks;
  // they cannot be used from a script because they do not change the state of
  // the check.  Use the ones with parameters from a script.
  void CompositeWithFloatCallback(int val);
  void CompositeWithRGBACallback(int val);
  void CompositeCompressionCallback(int val);

  // Description:
  // Tracing uses the method with the argument.
  // A reduction value of 1 is equivalent to having the feature
  // disabled.
  void ReductionCheckCallback(int state);
  void ReductionFactorScaleCallback(double value);
  void SetReductionFactor(int val);

  // Description:
  // Squirt compression os a combination of run-length encoding
  // and bit compression.  A level of 0 is the same as disabling
  // squirt compression.
  void SquirtCheckCallback(int state);
  void SquirtLevelScaleCallback(double value);
  void SetSquirtLevel(int val);

  // Description:
  // Threshold for collecting data to a single process (MBytes).
  void CompositeCheckCallback(int state);
  void CompositeThresholdScaleCallback(double value);
  void CompositeThresholdLabelCallback(double value);
  void SetCompositeThreshold(float val);
  vtkGetMacro(CompositeThreshold, float);

  // Description:
  // This is a hack to disable a feature that is 
  // not working yet for tiled displays.
  void SetCompositeOptionEnabled(int val);

  // Description:
  // Export the render module state to a file.
  virtual void SaveState(ofstream *file);
  
protected:
  vtkPVCompositeRenderModuleUI();
  ~vtkPVCompositeRenderModuleUI();
 
  vtkKWFrameWithLabel *ParallelRenderParametersFrame;

  vtkKWCheckButton *CompositeWithFloatCheck;
  vtkKWCheckButton *CompositeWithRGBACheck;
  vtkKWCheckButton *CompositeCompressionCheck;

  vtkKWLabel*       CompositeLabel;
  vtkKWCheckButton* CompositeCheck;
  vtkKWScale*       CompositeThresholdScale;
  vtkKWLabel*       CompositeThresholdLabel;
  float             CompositeThreshold;

  vtkKWLabel*       ReductionLabel;
  vtkKWCheckButton* ReductionCheck;
  vtkKWScale*       ReductionFactorScale;
  vtkKWLabel*       ReductionFactorLabel;
  int               ReductionFactor;

  vtkKWLabel*       SquirtLabel;
  vtkKWCheckButton* SquirtCheck;
  vtkKWScale*       SquirtLevelScale;      
  vtkKWLabel*       SquirtLevelLabel;
  int               SquirtLevel;

  int CompositeWithFloatFlag;
  int CompositeWithRGBAFlag;
  int CompositeCompressionFlag;

  int CompositeOptionEnabled;

  vtkPVCompositeRenderModuleUI(const vtkPVCompositeRenderModuleUI&); // Not implemented
  void operator=(const vtkPVCompositeRenderModuleUI&); // Not implemented
};


#endif


