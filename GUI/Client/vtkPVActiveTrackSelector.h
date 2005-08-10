/*=========================================================================

  Program:   ParaView
  Module:    vtkPVActiveTrackSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVActiveTrackSelector - Widget that shows menus to select the
// active track.
// .SECTION Description


#ifndef __vtkPVActiveTrackSelector_h
#define __vtkPVActiveTrackSelector_h

#include "vtkPVTracedWidget.h"

class vtkKWLabel;
class vtkKWMenuButton;
class vtkPVActiveTrackSelectorInternals;
class vtkPVAnimationCueTree;
class vtkPVAnimationCue;
class vtkSMAnimationCueProxy;
class vtkSMProxy;
class vtkPVSource;

class VTK_EXPORT vtkPVActiveTrackSelector : public vtkPVTracedWidget
{
public:
  static vtkPVActiveTrackSelector* New();
  vtkTypeRevisionMacro(vtkPVActiveTrackSelector, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication* app);

  // Description:
  // Add the AnimationCue for a PVSource.
  void AddSource(vtkPVAnimationCueTree*);
  void RemoveSource(vtkPVAnimationCueTree*);
  void RemoveSource(vtkPVSource*);

  // Description:
  // These are the callbacks for menus.
  void SelectSourceCallback(const char* key);
  void SelectPropertyCallback(int cue_index);

  // Description:
  // When ever a cue gets focus, this method should be called
  // so that the cue gets selected in the Track Selector as well.
  // Call with argument NULL when a cue is unselected.
  // Returns 0 if passed cue does not exist, 1 otherwise.
  int SelectCue(vtkPVAnimationCue*);
  int SelectCue(const char* sourceName, vtkSMAnimationCueProxy* cue);

  // Description:
  // Accessors to menu buttons
  vtkGetObjectMacro(SourceMenuButton, vtkKWMenuButton);
  vtkGetObjectMacro(PropertyMenuButton, vtkKWMenuButton);

  // Description:
  // Returns the currently selected cue.
  vtkGetObjectMacro(CurrentCue, vtkPVAnimationCue);

  // Description:
  // Determines whether the currently select cue gets the focus in
  // the track view. True by default/
  vtkGetMacro(FocusCurrentCue, int);
  vtkSetMacro(FocusCurrentCue, int);

  // Description:
  // If PackHorizontally, the sub-widgets will be packed horizontally,
  // instead of being gridded vertically. This is false by default.
  // Call before Create().
  vtkSetMacro(PackHorizontally, int);
  vtkGetMacro(PackHorizontally, int);

  // Description:
  // (Shallow) copy all the source cues from the source widget.
  // If onlyCopySources is true, only cues that have an associated
  // PVSource are copied.
  void ShallowCopy(vtkPVActiveTrackSelector* source,
                   int onlyCopySources=0);

  // Description:
  // If true, only properties belonging to the proxy of the 
  // point PVSource are shown in the properties dialog. False
  // by default.
  vtkSetMacro(DisplayOnlyPVSourceProperties, int);
  vtkGetMacro(DisplayOnlyPVSourceProperties, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's Limited
  // Edition Mode, etc.), the "enable" state of the object is updated and
  // propagated to its internal parts/subwidgets. 
  virtual void UpdateEnableState();
protected:
  vtkPVActiveTrackSelector();
  ~vtkPVActiveTrackSelector();
  void SelectSourceCallbackInternal(const char*key);
  void SelectPropertyCallbackInternal(int cue_index);

  void BuildPropertiesMenu(const char* pretext, vtkPVAnimationCueTree* cueTree);
  void CleanupPropertiesMenu();
  void CleanupSource();

  vtkPVAnimationCueTree* CurrentSourceCueTree;
  vtkPVAnimationCue* CurrentCue;
  vtkKWLabel* SourceLabel;
  vtkKWMenuButton* SourceMenuButton;

  vtkKWLabel* PropertyLabel;
  vtkKWMenuButton* PropertyMenuButton;
 
  vtkPVActiveTrackSelectorInternals* Internals;

  int PackHorizontally;
  int FocusCurrentCue;

  int DisplayOnlyPVSourceProperties;

private:
  vtkPVActiveTrackSelector(const vtkPVActiveTrackSelector&); // Not implemented.
  void operator=(const vtkPVActiveTrackSelector&); // Not implemented.
};

#endif
