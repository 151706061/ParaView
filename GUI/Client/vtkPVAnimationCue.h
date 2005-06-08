/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationCue.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationCue - the GUI for an Animation Cue.
// .SECTION Description
// vtkPVAnimationCue is manages the GUI for an Animation Cue. 
// The GUI has two sections, the navgation interface: which shows the label
// of the cue and the timeline: which is used to modify keyframes. ParaView
// puts these two sections in two panes of a split frame so that the label 
// length does not reduce the visible area for the timelines. The parent of an
// object of this class acts as the parent for the Navigation section,
// while the TimeLineParent is the parent for the timeline. Both of which need
// to be set before calling create.
// This class has Virtual mode. In this mode, no proxies are created for this class.
// This mode is used by the Subclass vtkPVAnimationCueTree which represents a GUI
// element which has child cues eg. the cue for the PVSource or for a property
// with multiple elements. Thus, Virtual cue is used merely to group the 
// chidlren. The support for adding children and managing them is provided by the
// subclass vtkPVAnimationCueTree.
//
// .SECTION See Also
// vtkPVAnimationCueTree vtkSMAnimationCueProxy

#ifndef __vtkPVAnimationCue_h
#define __vtkPVAnimationCue_h

#include "vtkPVSimpleAnimationCue.h"

class vtkKWWidget;
class vtkKWLabel;
class vtkPVTimeLine;
class vtkKWFrame;
class vtkSMAnimationCueProxy;
class vtkSMKeyFrameAnimationCueManipulatorProxy;
class vtkPVKeyFrame;
class vtkCollection;
class vtkCollectionIterator;
class vtkPVAnimationScene;
class vtkPVSource;
class vtkSMPropertyStatusManager;
class vtkSMProxy;

class VTK_EXPORT vtkPVAnimationCue : public vtkPVSimpleAnimationCue
{
public:
  static vtkPVAnimationCue* New();
  vtkTypeRevisionMacro(vtkPVAnimationCue, vtkPVSimpleAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication* app, const char* args);

  // Description:
  // TimeLineParent is the frame that contains the timelines.
  // this->Parent is the frame which contains the labels (or the Navgation
  // widget) for the cue.
  void SetTimeLineParent(vtkKWWidget* frame);

  // Description:
  // Label Text is the text shown for this cue.
  virtual void SetLabelText(const char* label);

  // Description:
  // Get the timeline object.
  vtkGetObjectMacro(TimeLine, vtkPVTimeLine);

  virtual void PackWidget();
  virtual void UnpackWidget();

  // Description:
  // Provides for highlighting of the selected cue.
  virtual void GetFocus();
  virtual void RemoveFocus();
  virtual int HasFocus() {return this->Focus;}

  // Description:
  // Remove All Key frames from this cue.
  virtual void RemoveAllKeyFrames();

  // Description:
  // Deletes the keyframe at given index. If the deleted key frame is the
  // currenly selected keyframe, it changes the selection and the timeline is
  // updated.
  void DeleteKeyFrame(int id);
 
  // Description:
  // Replaces a keyframe with another. The Key time and key value of
  // the oldFrame and copied over to the newFrame;
  virtual void ReplaceKeyFrame(vtkPVKeyFrame* oldFrame, vtkPVKeyFrame* newFrame);


  // Description:
  // Start Recording. Once recording has been started new key frames cannot be added directly.
  virtual void StartRecording();

  // Description:
  // Stop Recording.
  virtual void StopRecording();

  virtual void RecordState(double ntime, double offset, int onlyFocus);

  // Description:
  // Set a pointer to the AnimationScene. This is not reference counted. A cue
  // adds itself to the scene when it has two or more key frames (i.e. the is 
  // animatable), and it removes itself from the Scene is the number of keyframes
  // reduces.
  void SetAnimationScene(vtkPVAnimationScene* scene);

  // Description:
  // Pointer to the PVSource that this cue stands for.
  // When the property is modifed (during animation), the cue calls
  // MarkSourcesForUpdate() on the PVSource so the the pipeline is
  // re-rendered.
  void SetPVSource(vtkPVSource*);
  vtkGetObjectMacro(PVSource, vtkPVSource);

  // Description:
  // Time marker is a vartical line used to indicate the current time.
  // This method sets the timemarker of the timeline for this cue alone.
  virtual void SetTimeMarker(double time);
  double GetTimeMarker();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  virtual void SaveState(ofstream* file);

  // Description:
  // Each cue is assigned a unique name. This name is used to indentify
  // the cue in trace/ state. Names are assigned my the vtkPVAnimationManager
  // while creating the PVAnimationCue. A child cue can be obtained from the
  // parent vtkPVAnimationCueTree using this name.
  // Note that this method does not ensure that the name is indeed unique.
  // It is responsibility of the vtkPVAnimationManager to set unique names
  // (atleast among the siblings) for the trace/ state to work properly. Also,
  // only vtkPVAnimationManager must set the name of the cue.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Enable horizontal zooming of the timeline.
  void SetEnableZoom(int zoom);

  // Description:
  virtual void Zoom(double range[2]);
  void Zoom(double s, double e) 
    {
    double r[2]; 
    r[0]=s; r[1] = e;
    this->Zoom(r);
    }

  // Description:
  // The name of a cue for a PVSource depends on the PVSource's name. 
  // For trace to work reliably, the cue's name must be 
  // constructed on using the PVSource (not it's name, since the name may change
  // when the trace is run).
  // This returns the tcl script/string that evaluates the
  // name correctly at runtime. For non-source cues (.i.e. cues for things like Camera
  // or the property cues), this is same as the name of the cue.
  const char* GetTclNameCommand();

  // Description:
  // Updates the visibility of the cue.
  // If the animated property is not "animateable", then it is
  // visible only in Advanced mode.
  virtual void UpdateCueVisibility(int advanced);
  vtkGetMacro(CueVisibility, int);

  // Description:
  // Detachs the cue. i.e. removes it from scene etc. and prepares it
  // to be deleted.
  virtual void Detach();

  // Description:
  // This will select the keyframe. Fires a SelectionChangedEvent.
  virtual void SelectKeyFrame(int id);

  // Description:
  // Set the timeline parameter bounds. This moves the timeline end points.
  // Depending upon is enable_scaling is set, the internal nodes
  // are scaled.
  virtual void SetTimeBounds(double bounds[2], int enable_scaling=0);
  virtual int GetTimeBounds(double* bounds);

  // Description:
  // Creates a new key frame of the specified type and add it to the cue at
  // the given time.  Time is normalized to the span of the cue [0,1]. This
  // method also does not verify is a key frame already exists at the
  // specified time.
  virtual int CreateAndAddKeyFrame(double time, int type);

protected:
  vtkPVAnimationCue();
  ~vtkPVAnimationCue();
//BTX
  // Description:
  // Set/Get the type of the image shown to the left of the label 
  // in the Navigation interface. This is useful esp for simulating
  // the apperance of a tree.
  void SetImageType(int type);
  vtkGetMacro(ImageType, int);

  enum {
    NONE=0,
    IMAGE_OPEN,
    IMAGE_CLOSE
  };
//ETX
 
  void InitializeObservers(vtkObject* object);
  virtual void ExecuteEvent(vtkObject* obj, unsigned long event, void*data);
 
  vtkKWWidget* TimeLineParent;
  vtkPVSource* PVSource;

  vtkKWLabel* Label; 
  vtkKWLabel* Image;
  vtkKWFrame* Frame;

  vtkKWFrame* TimeLineContainer;
  vtkKWFrame* TimeLineFrame;
  vtkPVTimeLine* TimeLine;

  int ImageType;
  int ShowTimeLine;

  char* Name;
  char* TclNameCommand;
  vtkSetStringMacro(TclNameCommand);

  int Focus;
  vtkPVAnimationScene* PVAnimationScene;
  
  // Description:
  // Internal methods to change focus state of this cue.
  void GetSelfFocus();
  void RemoveSelfFocus();

  // Description:
  // A PVCue registers the proxies and adds it to the AnimationScene iff it 
  // has atleast two keyframes and it is not virtual. Whenever this
  // criteria is not met, it is unregistered and removed form the AnimationScene.
  // This ensures that SMState and BatchScript will have only those cue proxies
  // which actually constitute any animation.
  virtual void RegisterProxies();
  virtual void UnregisterProxies();
  
  int CueVisibility;
  int DisableSelectionChangedEvent;
private:
  vtkPVAnimationCue(const vtkPVAnimationCue&); // Not implemented.
  void operator=(const vtkPVAnimationCue&); // Not implemented.
};

#endif


