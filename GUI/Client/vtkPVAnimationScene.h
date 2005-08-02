/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationScene.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationScene
// .SECTION Description
// GUI for vtkSMAnimationSceneProxy.

#ifndef __vtkPVAnimationScene_h
#define __vtkPVAnimationScene_h

#include "vtkPVTracedWidget.h"

class vtkPVAnimationSceneObserver;
class vtkKWFrame;
class vtkKWPushButton;
class vtkKWCheckButton;
class vtkKWScaleWithEntry;
class vtkKWMenuButton;
class vtkKWThumbWheel;
class vtkKWLabel;
class vtkSMAnimationSceneProxy;
class vtkPVAnimationCue;
class vtkPVRenderView;
class vtkPVAnimationManager;
class vtkPVWindow;
class vtkPVVCRControl;

class VTK_EXPORT vtkPVAnimationScene : public vtkPVTracedWidget
{
public:
  static vtkPVAnimationScene* New();
  vtkTypeRevisionMacro(vtkPVAnimationScene, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication* app);

  // Description:
  // Callbacks
  void SetPlayModeToSequence();
  void SetPlayModeToRealTime();
  void DurationChangedCallback();
  void DurationChangedKeyReleaseCallback();
  void LoopCheckButtonCallback();
  void ToolbarLoopCheckButtonCallback();
  void TimeScaleCallback();
  void ToolbarRecordCheckButtonCallback();
  void RecordCheckCallback();
  void SaveAnimationCallback();

  // Description:
  int  IsInPlay();
  void Play();
  void Stop();
  void GoToBeginning();
  void GoToEnd();

  // Description:
  // Goes to the next/previous frame (incremented by 1/framerate).
  void GoToNext();
  void GoToPrevious();


  void StartRecording();
  void StopRecording();
  void RecordState();

  // Description:
  // Get/Set the duration for which the scene is played in seconds.
  void SetDuration(double seconds);
  void SetDurationWithTrace(double s);
  double GetDuration();

  // Description:
  // Set the play mode to RealTime(1) or Sequence(0).
  void SetPlayMode(int mode);
  int GetPlayMode();

  // Description:
  // Set the frame rate.
  void SetFrameRate(double fps);
  double GetFrameRate();

  // Description:
  // Set if to play the animation in a loop.
  void SetLoop(int loop);
  void SetLoopWithTrace(int loop);
  int GetLoop();
  
  // Description:
  // Sets the current time for the animation state.
  // Note that this time is not normalized time. It extends from
  // [0, Duration].
  void SetCurrentTime( double time);
  void SetCurrentTimeWithTrace(double time);
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
  // Avoid windows name mangling.
#define GetTickCount() GetCurrentTime()
#endif
  // Description:
  // Get the current animation time step.
  double GetCurrentTime();
  
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetTickCount
  //BTX
  int GetTickCount();
  //ETX
#endif

  // Description:
  // Sets the current time for the animation state. 
  // This is normalized time [0,1], normalized to the duration 
  // of the scene.
  void SetNormalizedCurrentTime(double ntime);
  double GetNormalizedCurrentTime();

  // Description:
  // Add/Remove animation cues from the scene. PVAnimationCues are not
  // reference counted. Hence, PVAnimationCue must ensure that it is
  // removed from the PVScene before it is deleted.
  void AddAnimationCue(vtkPVAnimationCue* cue);
  void RemoveAnimationCue(vtkPVAnimationCue* cue);

  virtual void SaveInBatchScript(ofstream* file);

  void SaveState(ofstream* file);

  // Description:
  // Access to the render view. Needed to build up the source list.
  // SetWindow can't be an object macro because that sets up a circular
  // reference.
  virtual void SetWindow(vtkPVWindow *window);
  vtkGetObjectMacro(Window, vtkPVWindow);

  void SetRenderView(vtkPVRenderView* pvRenderView);
  vtkGetObjectMacro(RenderView, vtkPVRenderView);

  void SetAnimationManager(vtkPVAnimationManager*);

  virtual void UpdateEnableState();


  void SaveImages(const char* fileRoot, const char* ext, int width, int height, 
                                         double framerate);
  void SaveGeometry(const char* filename);

  void InvalidateAllGeometries();

  // Description:
  // Helper methods to show/hide the animation toolbar.
  void ShowAnimationToolbar() { this->SetAnimationToolbarVisibility(1); }
  void HideAnimationToolbar() { this->SetAnimationToolbarVisibility(0); }
  void SetAnimationToolbarVisibility(int visible);

  // Description:
  // Set if cache should be used for playing animation.
  void SetCaching(int enable);
  int GetCaching();

  // Description:
  // Whenever the properties of the scence changed, this class
  // can call a callback. This method is used to set the callback.
  // When target=NULL, the callback is removed.
  void SetPropertiesChangedCallback(vtkKWWidget* target, 
    const char* methodAndArgs);

  void PrepareForDelete();
protected:
  vtkPVAnimationScene();
  ~vtkPVAnimationScene();

  void CaptureErrorEvents();
  void ReleaseErrorEvents();

  vtkPVRenderView* RenderView;
  vtkPVWindow* Window;
  vtkPVAnimationManager* AnimationManager;

  // Animation Control.
  vtkPVVCRControl* VCRControl;
  vtkPVVCRControl* VCRToolbar;

  vtkKWLabel* TimeLabel;
  vtkKWScaleWithEntry* TimeScale;

  vtkKWLabel* DurationLabel;
  vtkKWThumbWheel* DurationThumbWheel; 

  vtkKWLabel* PlayModeLabel;
  vtkKWMenuButton* PlayModeMenuButton;
  
  vtkSMAnimationSceneProxy* AnimationSceneProxy;
  char* AnimationSceneProxyName;
  vtkSetStringMacro(AnimationSceneProxyName);

  virtual void ExecuteEvent(vtkObject* , unsigned long event, void* calldata);
//BTX
  vtkPVAnimationSceneObserver* Observer;
  friend class vtkPVAnimationSceneObserver;
//ETX
  void CreateProxy();
  int InPlay;
  int InvokingError;

  unsigned long ErrorEventTag;

  char* PropertiesChangedCallbackCommand;
  vtkSetStringMacro(PropertiesChangedCallbackCommand);
  void InvokePropertiesChangedCallback();

private:
  vtkPVAnimationScene(const vtkPVAnimationScene&); // Not implemented.
  void operator=(const vtkPVAnimationScene&); // Not implemented.
};

#endif
