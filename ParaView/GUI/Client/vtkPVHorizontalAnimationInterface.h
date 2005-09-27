/*=========================================================================

  Program:   ParaView
  Module:    vtkPVHorizontalAnimationInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVHorizontalAnimationInterface - An object that controls an animation.
//
// .SECTION Description
// This is the GUI for the horizontal animation interface which 
// contains the timelines. 
// .SECTION See Also
// vtkPVVerticalAnimationInterface

#ifndef __vtkPVHorizontalAnimationInterface_h
#define __vtkPVHorizontalAnimationInterface_h

#include "vtkPVTracedWidget.h"

class vtkCollection;
class vtkCollectionIterator;
class vtkKWApplication;
class vtkKWFrame;
class vtkKWFrameWithScrollbar;
class vtkKWSplitFrame;
class vtkPVAnimationCue;
class vtkPVAnimationCueTree;

class VTK_EXPORT vtkPVHorizontalAnimationInterface : public vtkPVTracedWidget
{
public:
  vtkTypeRevisionMacro(vtkPVHorizontalAnimationInterface, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVHorizontalAnimationInterface* New();

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication* app);

  virtual void ResizeCallback();

  void AddAnimationCueTree(vtkPVAnimationCueTree* pvCueTree);
  void RemoveAnimationCueTree(vtkPVAnimationCueTree* pvCueTree);

  void SetTimeMarker(double ntime);

  void StartRecording();
  void StopRecording();
  void RecordState(double ntime, double offset, int onlyFocus);

  // Description:
  // Set the timeline parameter bounds. This moves the timeline end points.
  // Depending upon is enable_scaling is set, the internal nodes
  // are scaled.
  virtual void SetTimeBounds(double bounds[2], int enable_scaling=0);
  virtual int GetTimeBounds(double* bounds);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  void SaveState(ofstream* file);

  vtkGetObjectMacro(ParentTree, vtkPVAnimationCueTree);

  // Description:
  // Save restore window geometry.
  virtual void SaveWindowGeometryToRegistry();
  virtual void RestoreWindowGeometryFromRegistry();

  // Description:
  // Remove All Key frames from this cue.
  void RemoveAllKeyFrames();

protected:
  vtkPVHorizontalAnimationInterface();
  ~vtkPVHorizontalAnimationInterface();

  vtkKWSplitFrame* SplitFrame;
  vtkKWFrame* TimeLineFrame;
  vtkKWFrame* PropertiesFrame;
  vtkKWFrameWithScrollbar* ScrollFrame;

  vtkCollection* AnimationEntries;
  vtkCollectionIterator* AnimationEntriesIterator;

  vtkPVAnimationCueTree* ParentTree;
  void InitializeObservers(vtkPVAnimationCue* cue);
  void ExecuteEvent(vtkObject* obj, unsigned long event, void* calldata);

private:
  vtkPVHorizontalAnimationInterface(const vtkPVHorizontalAnimationInterface&); // Not implemented.
  void operator=(const vtkPVHorizontalAnimationInterface&); // Not implemented.
};

#endif



