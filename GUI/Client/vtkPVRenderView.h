/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderView - For using styles
// .SECTION Description
// This is a render view that is a parallel object.  It needs to be cloned
// in all the processes to work correctly.  After cloning, the parallel
// nature of the object is transparent.
// Other features:
// I am going to try to divert the events to a vtkInteractorStyle object.
// I also have put compositing into this object.  I had to create a separate
// renderwindow and renderer for the off screen compositing (Hacks).
// Eventually I need to merge these back into the main renderer and renderer
// window.


#ifndef __vtkPVRenderView_h
#define __vtkPVRenderView_h

#include "vtkKWView.h"

class vtkInteractorObserver;
class vtkKWChangeColorButton;
class vtkKWLabel;
class vtkKWPushButton;
class vtkKWRadioButton;
class vtkKWScaleWithEntry;
class vtkKWTopLevel;
class vtkPVSourceNotebook;
class vtkKWSplitFrame;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVAxesWidget;
class vtkPVCameraIcon;
class vtkPVCameraControl;
class vtkPVCornerAnnotationEditor;
class vtkPVData;
class vtkPVInteractorStyleControl;
class vtkPVSource;
class vtkPVSourceList;
class vtkPVSourcesNavigationWindow;
class vtkPVTreeComposite;
class vtkPVWindow;
class vtkPVRenderModuleUI;
class vtkPVRenderViewObserver;
class vtkSMRenderModuleProxy;
class vtkTimerLog;
class vtkSMProxy;

#define VTK_PV_VIEW_MENU_LABEL       " 3D View Properties"

class VTK_EXPORT vtkPVRenderView : public vtkKWView
{
public:
  static vtkPVRenderView* New();
  vtkTypeRevisionMacro(vtkPVRenderView,vtkKWView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // I wanted to get rid of these methods, but they are used
  // for corner annotation to work.
  void AddAnnotationProp(vtkPVCornerAnnotationEditor *c);
  void RemoveAnnotationProp(vtkPVCornerAnnotationEditor *c);

  // Description:
  // Set the application right after construction.
  void CreateRenderObjects(vtkPVApplication *pvApp);
  
  // Description:
  // Add widgets to vtkKWView's notebook
  virtual void CreateViewProperties();
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Method called by the toolbar reset camera button.
  void ResetCamera();
  
  // Description:
  // Specify the position, focal point, and view up of the camera.
  void SetCameraState(float p0, float p1, float p2,
                      float fp0, float fp1, float fp2,
                      float up0, float up1, float up2);
  
  // Description:
  // Set the parallel scale of the camera.
  void SetCameraParallelScale(float scale);

  // Description:
  // Callback to set the view up or down one of the three axes.
  void StandardViewCallback(float x, float y, float z);

  // Description:
  // Make snapshot of the render window.
  virtual void SaveAsImage() { this->Superclass::SaveAsImage(); }
  virtual void SaveAsImage(const char* filename) ;

  // Description:
  // Returns the RenderModuleProxy (same as the one got 
  // from vtkPVApplication::GetRenderModuleProxy())
  vtkGetObjectMacro(RenderModuleProxy, vtkSMRenderModuleProxy);
  
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // Composites
  void Render();
  void ForceRender();
  void EventuallyRender();
  void EventuallyRenderCallBack();
  void DisableRendering();

  // Description:
  // Tcl "update" has to be called for various reasons (packing).
  // This calls update without triggering EventuallyRenderCallback.
  // I was having problems with multiple renders.
  void UpdateTclButAvoidRendering();
  
  // Description:
  // Callback method bound to expose or configure events.
  void Exposed();
  void Configured();
  
  // Description:
  // Update the navigation window for a particular source
  void UpdateNavigationWindow(vtkPVSource *currentSource, int nobind);

  // Description:
  // Get the frame for the navigation window
  vtkGetObjectMacro(NavigationFrame, vtkKWFrameWithLabel);

  // Description:
  // Show either navigation window with a fragment of pipeline or a
  // source window with a list of sources. If the argument registry
  // is 1, then the value will be stored in the registry.
  void ShowNavigationWindowCallback(int registry);
  void ShowSelectionWindowCallback(int registry);
  
  // Description:
  // Export the renderer and render window to a file.
  void SaveInBatchScript(ofstream *file);
  void SaveState(ofstream *file);

  // Description:
  // Change the background color.
  void SetRendererBackgroundColor(double r, double g, double b);

  // Description:
  // Close the view - called from the vtkkwwindow. This default method
  // will simply call Close() for all the composites. Can be overridden.
  virtual void Close();

  // Description:
  // This method Sets all IVars to NULL and unregisters
  // vtk objects.  This should eliminate circular references.
  void PrepareForDelete();
  
  // Description:
  // Get the parallel projection check button
  vtkGetObjectMacro(ParallelProjectionCheck, vtkKWCheckButton);

  // Description:
  // Callback for toggling between parallel and perspective.
  void ParallelProjectionCallback();
  void ParallelProjectionOn();
  void ParallelProjectionOff();
  
  // Description:
  // Callback for the triangle strips check button
  void TriangleStripsCallback();
  void SetUseTriangleStrips(int state);
  
  // Description:
  // Callback for the immediate mode rendering check button
  void ImmediateModeCallback();
  void SetUseImmediateMode(int state);
  
  // Description:
  // Get the triangle strips check button.
  vtkGetObjectMacro(TriangleStripsCheck, vtkKWCheckButton);
  
  // Description:
  // Get the immediate mode rendering check button.
  vtkGetObjectMacro(ImmediateModeCheck, vtkKWCheckButton);

  // Description:
  // A convience method to get the PVWindow.
  vtkPVWindow *GetPVWindow();

  // Description:
  // Get the widget that controls the interactor styles.
  vtkGetObjectMacro(ManipulatorControl3D, vtkPVInteractorStyleControl);
  vtkGetObjectMacro(ManipulatorControl2D, vtkPVInteractorStyleControl);

  // Description:
  // Get the widget that controls the camera.
  vtkGetObjectMacro(CameraControl, vtkPVCameraControl);
  
  // Description:
  // Get the size of the render window.
  int* GetRenderWindowSize();

  // Description:
  // Setup camera manipulators by populating the control and setting
  // initial values.
  void SetupCameraManipulators();

  // Description:
  // Update manipulators after they were added to control.
  void UpdateCameraManipulators();

  // Description:
  // Here so that sources get packed in the second frame.
  virtual vtkKWWidget *GetSourceParent();
  vtkKWSplitFrame *GetSplitFrame() {return this->SplitFrame;}

  // Description:
  // Sources now share a single notebook and information GUI.
  // This object does not seen like the best place to create
  // this notebook and GUI. But it is here now for legacy reasons.
  // Get SourceParent may not be needed anymore.
  vtkGetObjectMacro(SourceNotebook,vtkPVSourceNotebook);

  // Description:
  // This method is called when an event is called that PVRenderView
  // is interested in.
  void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);

  // Description:
  // Store current camera at a specified position. This stores all the
  // camera parameters and generates a small icon.
  void StoreCurrentCamera(int position);
  
  // Description:
  // Restore current camera from a specified position.
  void RestoreCurrentCamera(int position);  

  // Description:
  // Set the global variable for always displaying 3D widgets.
  void Display3DWidgetsCallback();
  void SetDisplay3DWidgets(int s);

  // Description:
  // Show the names in sources browser.
  void SetSourcesBrowserAlwaysShowName(int s);

  // Description:
  // Switch to the View Properties menu back and forth
  void SwitchBackAndForthToViewProperties();

  //BTX
  // Description:
  // Access to VTK renderer and render window.
  vtkRenderWindow *GetRenderWindow();
  vtkRenderer *GetRenderer();
  //ETX
  
  // Description:
  // This method resizes the render window size but the actuall window stays
  // the same
  void SetRenderWindowSize(int x, int y);

  // Description:
  // Enable the input 3D widget
  void Enable3DWidget(vtkInteractorObserver *o);

  // Description:
  // Callbacks for the orientation axes.
  void SetOrientationAxesVisibility(int val);
  int  GetOrientationAxesVisibility();
  void OrientationAxesCheckCallback();
  void SetOrientationAxesInteractivity(int val);
  void OrientationAxesInteractiveCallback();
  void SetOrientationAxesOutlineColor(double r, double g, double b);
  void SetOrientationAxesTextColor(double r, double g, double b);

  // Description:
  // Returns the UI created by the render module
  vtkGetObjectMacro(RenderModuleUI, vtkPVRenderModuleUI);

  // Description:
  // Handle the edit copy menu option.
  virtual void EditCopy();

  // Description:
  // Print the image. This may pop up a dialog box, etc.
  virtual void PrintView();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Block render requests. If the render requests come, they will be blocked
  // and when unblocked render will be called.
  void StartBlockingRender();
  void EndBlockingRender();
 
  // Description:
  // Access to these widgets from a script.
  vtkGetObjectMacro(StandardViewsFrame, vtkKWFrameWithLabel);
  vtkGetObjectMacro(CameraIconsFrame, vtkKWFrameWithLabel);
  vtkGetObjectMacro(CameraControlFrame, vtkKWFrameWithLabel);
  vtkGetObjectMacro(OrientationAxesFrame, vtkKWFrameWithLabel);
  //BTX
  vtkGetObjectMacro(OrientationAxes, vtkPVAxesWidget);
  //ETX


  // Description:
  // Set the value of default light
  void DefaultLightIntensityCallback();
  void DefaultLightIntensityEndCallback();
  void SetDefaultLightIntensity(double intensity);
  void SetDefaultLightIntensityNoTrace(double intensity);
  void SetDefaultLightAmbientColor(double r, double g, double b);
  void SetDefaultLightDiffuseColor(double r, double g, double b);
  void SetDefaultLightSpecularColor(double r, double g, double b);
  void DefaultLightSwitchCallback();
  void SetDefaultLightSwitch(int val);

  // Description:
  // Callback when modifying the light kit via a slider
  void              LightCallback(int type, int subtype);

  // Description:
  // Callback when done modifying the light kit (end of slider event)
  void              LightEndCallback(int type, int subtype);

  // Description:
  // Callback for setting/removing the luminance
  void              MaintainLuminanceCallback();
  void              SetMaintainLuminance(int s);

  // Description:
  // Callback for setting/remove the kit light from the renderer (usefull for the test suite)
  void              UseLightCallback();
  void              SetUseLight(int s);
  
  // Description:
  // Used for the callback / and the trace
  double            GetLight(int type, int subtype);
  void              SetLightNoTrace(int type, int subtype, double value);
  void              SetLight(int type, int subtype, double value);


  // Description:
  // Prevent actions when exiting
  vtkSetClampMacro(ExitMode, int, 0, 1);
  vtkBooleanMacro(ExitMode, int);
  vtkGetMacro(ExitMode, int);

protected:
  vtkPVRenderView();
  ~vtkPVRenderView();

  // Access to the overlay renderer.
  vtkRenderer *GetRenderer2D();

  vtkPVSourceNotebook* SourceNotebook;

  void CalculateBBox(char* name, int bbox[4]);
 
  vtkKWFrameWithLabel *StandardViewsFrame;
  vtkKWPushButton   *XMaxViewButton; 
  vtkKWPushButton   *XMinViewButton; 
  vtkKWPushButton   *YMaxViewButton; 
  vtkKWPushButton   *YMinViewButton; 
  vtkKWPushButton   *ZMaxViewButton; 
  vtkKWPushButton   *ZMinViewButton; 

  vtkKWFrameWithLabel *RenderParametersFrame;
  vtkKWCheckButton *TriangleStripsCheck;
  vtkKWCheckButton *ParallelProjectionCheck;
  vtkKWCheckButton *ImmediateModeCheck;

  vtkPVRenderModuleUI* RenderModuleUI;

  vtkKWFrameWithLabel *InterfaceSettingsFrame;
  vtkKWCheckButton *Display3DWidgets;

  // Default light
  vtkKWFrameWithLabel    *DefaultLightFrame;
  vtkKWChangeColorButton *DefaultLightAmbientColor;
  vtkKWChangeColorButton *DefaultLightSpecularColor;
  vtkKWChangeColorButton *DefaultLightDiffuseColor;
  vtkKWScaleWithEntry    *DefaultLightIntensity;
  vtkKWCheckButton       *DefaultLightSwitch;

  // Lighting stuff:
  vtkKWFrameWithLabel *LightParameterFrame;
  vtkKWCheckButton  *UseLightButton;
  vtkKWLabel        *KeyLightLabel;
  vtkKWLabel        *FillLightLabel;
  vtkKWLabel        *BackLightLabel;
  vtkKWLabel        *HeadLightLabel;
  vtkKWScaleWithEntry        *KeyLightScale[4];
  vtkKWScaleWithEntry        *FillLightScale[4];
  vtkKWScaleWithEntry        *BackLightScale[4];
  vtkKWScaleWithEntry        *HeadLightScale[4];
  vtkKWCheckButton  *MaintainLuminanceButton;
  // Main proxy to access the vtkLightKit

  vtkKWFrameWithLabel *OrientationAxesFrame;
  vtkKWCheckButton *OrientationAxesCheck;
  vtkKWCheckButton *OrientationAxesInteractiveCheck;
  vtkKWChangeColorButton *OrientationAxesOutlineColor;
  vtkKWChangeColorButton *OrientationAxesTextColor;
  vtkPVAxesWidget *OrientationAxes;

  vtkKWSplitFrame *SplitFrame;

  vtkKWFrameWithLabel* NavigationFrame;
  vtkPVSourcesNavigationWindow* NavigationWindow;
  vtkPVSourcesNavigationWindow* SelectionWindow;
  vtkKWRadioButton *NavigationWindowButton;
  vtkKWRadioButton *SelectionWindowButton;
  
  int BlockRender;

  int ShowSelectionWindow;
  int ShowNavigationWindow;

  // For the renderer in a separate toplevel window.
  vtkKWTopLevel *TopLevelRenderWindow;

  vtkPVInteractorStyleControl *ManipulatorControl2D;
  vtkPVInteractorStyleControl *ManipulatorControl3D;

  // Camera icons
  vtkKWFrameWithLabel* CameraIconsFrame;
  vtkPVCameraIcon* CameraIcons[6];
  
  // Camera controls (elevation, azimuth, roll)
  vtkKWFrameWithLabel *CameraControlFrame;
  vtkPVCameraControl *CameraControl;

  vtkKWPushButton *PropertiesButton;

  char *MenuLabelSwitchBackAndForthToViewProperties;
  vtkSetStringMacro(MenuLabelSwitchBackAndForthToViewProperties);
  
  vtkPVRenderViewObserver* Observer;
  vtkSMRenderModuleProxy* RenderModuleProxy;

  vtkTimerLog *RenderTimer;
  Tcl_TimerToken TimerToken;

  int ExitMode;
  
private:
  vtkPVRenderView(const vtkPVRenderView&); // Not implemented
  void operator=(const vtkPVRenderView&); // Not implemented
};


#endif
