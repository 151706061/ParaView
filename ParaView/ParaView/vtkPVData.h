/*=========================================================================

  Program:   ParaView
  Module:    vtkPVData.h
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
// .NAME vtkPVData - Object to represent the output of a PVSource.
// .SECTION Description
// This object combines methods for accessing parallel VTK data, and also an 
// interface for changing the view of the data.  The interface used to be in a 
// superclass called vtkPVActorComposite.  I want to separate the interface 
// from this object, but a superclass is not the way to do it.

#ifndef __vtkPVData_h
#define __vtkPVData_h


#include "vtkKWObject.h"

class vtkCubeAxesActor2D;
class vtkDataSet;
class vtkKWBoundsDisplay;
class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkKWCheckButton;
class vtkKWEntry;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWLabeledEntry;
class vtkKWLabeledFrame;
class vtkKWOptionMenu;
class vtkKWPushButton;
class vtkKWScale;
class vtkKWWidget;
class vtkPVApplication;
class vtkPVColorMap;
class vtkPVRenderView;
class vtkPVSource;
class vtkPVSource;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;

class VTK_EXPORT vtkPVData : public vtkKWObject
{
public:
  static vtkPVData* New();
  vtkTypeRevisionMacro(vtkPVData, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This also creates the parallel vtk objects for the composite.
  // (actor, mapper, ...)
  void SetPVApplication(vtkPVApplication *pvApp);
  
  void SetApplication(vtkKWApplication *)
    {
      vtkErrorMacro("vtkPVData::SetApplication should not be used. Use SetPVApplcation instead.");
    }
  
  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  virtual vtkPVData *MakeObject(){vtkErrorMacro("No MakeObject");return NULL;}
  
  // Description:
  // DO NOT CALL THIS IF YOU ARE NOT A PVSOURCE!
  // The composite sets this so this data widget will know who owns it.
  void SetPVSource(vtkPVSource *source);
  vtkGetObjectMacro(PVSource, vtkPVSource);
  
  // Description:
  // This is for setting up the links between VTK objects and PV object.
  // This call also sets the input to the mapper.
  // SetVTKData should be called after the application has been set, but before
  // PVData is used as input a filter or output of a source.
  // We could change the object so that it creates its own data (during initialization), 
  // but then we would have to tell it what type of data to create.
  void SetVTKData(vtkDataSet *data, const char *name);
  vtkGetObjectMacro(VTKData,vtkDataSet);  

  // Description:
  // The tcl name of the vtk data object.  This should be the primary method of 
  // manipulating the data since it exists on all processes.
  vtkGetStringMacro(VTKDataTclName);  
  
  // Description:
  // This method collects the bounds from all processes.
  // It expects the data to be up to date.
  void GetBounds(float bounds[6]);

  // Description:
  // This method collects the number of cells from all processes.
  // It expects the data to be up to date.
  int GetNumberOfCells();
  
  // Description:
  // This method collects the number of points from all processes.
  // It expects the data to be up to date.
  int GetNumberOfPoints();

  // Description:
  // Get the number of consumers
  vtkGetMacro(NumberOfPVConsumers, int);
  
  // Description:
  // Add, remove, get, or check a consumer.
  void AddPVConsumer(vtkPVSource *c);
  void RemovePVConsumer(vtkPVSource *c);
  vtkPVSource *GetPVConsumer(int i);
  int IsPVConsumer(vtkPVSource *c);
  
  // Description:
  // This methiod updates the piece that has been assinged to this process.
  void Update();

  // Description:
  // This method is called on creation.  If the data object is unstructured and 
  // has a maximum number of pieces, then a extract piece filter is inserted
  // before the data object.  This will give parallel pipelines at the
  // expense of initial generation (reading) of the data.
  void InsertExtractPiecesIfNecessary();
  
  //===================

  // Description:
  // Translate the actor to the specified location. Also modify the
  // entry widget that controles the translation.
  void SetActorTranslate(float* p);
  void SetActorTranslate(float x, float y, float z);
  void GetActorTranslate(float* p);
  void SetActorTranslate();
  
  // Description:
  // Scale the actor. Also modify the entry widget that controles the scaling.
  void SetActorScale(float* p);
  void SetActorScale(float x, float y, float z);
  void GetActorScale(float* p);
  void SetActorScale();
  
  // Description:
  // Set the transparency of the actor.
  void SetOpacity(float f);
  void OpacityChangedCallback();

  // Description:
  // Create the properties object, called by UpdateProperties.
  void CreateProperties();
  void UpdateProperties();
  
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  void CreateParallelTclObjects(vtkPVApplication *pvApp);
      
  // Description:
  // This method is meant to setup the actor/mapper
  // to best disply it input.  This will involve setting the scalar range,
  // and possibly other properties. 
  void Initialize();

  // Description:
  // This flag turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void SetVisibility(int v);
  void SetVisibilityInternal(int v);
  int GetVisibility();
  vtkBooleanMacro(Visibility, int);
  void VisibilityCheckCallback();
    
  // Description:
  // Sets the color range of all the mappers (all procs) and updates
  // the user interface as well.
  void SetColorRange(float min, float max);

  // Description:
  // This computes the union of the range of the data (current color by)
  // across all process. Of cousre it returns the range.
  void GetColorRange(float range[2]);
  
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  vtkGetObjectMacro(Mapper, vtkPolyDataMapper);
  
  // Description:
  // to change the ambient component of the light
  void AmbientChanged();
  void SetAmbient(float ambient);

  // Description:
  // Tcl name of the actor across all processes.
  vtkGetStringMacro(PropTclName);  
  
  // Description:
  // Methods called when item chosen from RepresentationMenu
  void SetRepresentation(const char*);
  void DrawWireframe();
  void DrawSurface();
  void DrawPoints();
  
  // Description:
  // Methods called when item chosen from RepresentationMenu
  void SetInterpolation(const char*);
  void SetInterpolationToFlat();
  void SetInterpolationToGouraud();

  // Description:
  // Get the representation menu.
  vtkGetObjectMacro(RepresentationMenu, vtkKWOptionMenu);

  // Description:
  // Get the interpolation menu.
  vtkGetObjectMacro(InterpolationMenu, vtkKWOptionMenu);
    
  // Description:
  // Callback for the ResetColorRange button.
  void ResetColorRange();

  // Set the color range from the entry widgets.
  void ColorRangeEntryCallback();
  
  // Description:
  // Called when the user presses the "Edit Color Map" button.
  void EditColorMapCallback();

  void SetScalarBarVisibility(int val);  
  void ScalarBarCheckCallback();
  vtkGetObjectMacro(ScalarBarCheck, vtkKWCheckButton);

  void SetCubeAxesVisibility(int val);
  void CubeAxesCheckCallback();

  vtkGetObjectMacro(CubeAxesCheck, vtkKWCheckButton);

  void CenterCamera();
  
  // Description:
  // Save out the mapper and actor to a file.
  void SaveInTclScript(ofstream *file, int interactiveFlag, int vtkFlag = 0);
  
  // Description:
  // Callback for the change color button.
  void ChangeActorColor(float r, float g, float b);
  
  // Description:
  // I would like to get rid of this reference if possible.
  // Needed to render. Also needed to get the default values for deci lod
  void SetPVRenderView(vtkPVRenderView *view);
  vtkPVRenderView *GetPVRenderView();
  
  // Description:
  // Get the name of the cube axes actor.
  vtkGetStringMacro(CubeAxesTclName);

  // Description:
  // Access to pointSize for scripting.
  void SetPointSize(int size);
  void SetLineWidth(int width);
  
  // Description:
  // Callbacks for point size and line width sliders.
  void ChangePointSize();
  void ChangeLineWidth();

  // Description:
  // Access to option menus for scripting.
  vtkGetObjectMacro(ColorMenu, vtkKWOptionMenu);

  // Description:
  // Callback methods when item chosen from ColorMenu
  void ColorByProperty();
  void ColorByPointFieldComponent(const char *name, int comp);
  void ColorByCellFieldComponent(const char *name, int comp);

  // Description:
  // Get the tcl name of the vtkPVGeometryFilter.
  vtkGetStringMacro(GeometryTclName);

  // Description:
  // Get the tcl name of the mapper
  vtkGetStringMacro(MapperTclName);
  vtkGetStringMacro(LODMapperTclName);
  
  vtkPVRenderView *PVRenderView;

  // Description:
  // This allows you to set the propertiesParent to any widget you like.  
  // If you do not specify a parent, then the views->PropertyParent is used.  
  // If the composite does not have a view, then a top level window is created.
  void SetPropertiesParent(vtkKWWidget *parent);
  vtkGetObjectMacro(PropertiesParent, vtkKWWidget);
  vtkKWWidget *PropertiesParent;

  // Description:
  // Returns true if CreateProperties() has been called.
  vtkGetMacro(PropertiesCreated, int);

  // Description:
  // This method returns the range of an arrays component.
  // It gathers range from partitions on all processes.
  void GetArrayComponentRange(float *range, int pointDataFlag,
                              const char *arrayName, int component);

  void ForceUpdate(vtkPVApplication* pvApp);

  // Description:
  // Called by vtkPVSource::DeleteCallback().
  void DeleteCallback();
  
  // Description:
  // Set the resolution of the decimation LOD.
  // Resulting decimation uses dim^3 volume.
  void SetLODResolution(int dim);
  vtkGetMacro(LODResolution, int);

  // Description:
  // Set the collection threshold.  The units of this threshold are 
  // in MegaBytes.  Defaults to 2 MB.  Threshold is compared against
  // the size of the total (across all processes) data set size.
  void SetCollectThreshold(float size);
  vtkGetMacro(CollectThreshold, float)

protected:
  vtkPVData();
  ~vtkPVData();

  virtual void SerializeRevision(ostream& os, vtkIndent indent);
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is, const char token[1024]);
  
  vtkDataSet *VTKData;
  char *VTKDataTclName;
  
  // This points to the source widget that owns this data widget.
  vtkPVSource *PVSource;

  // Keep a list of sources that are using this data.
  vtkPVSource **PVConsumers;
  int NumberOfPVConsumers;

  // If we are the last source to unregister a color map,
  // this method will turn its scalar bar visibility off.
  void SetPVColorMap(vtkPVColorMap *colorMap);

  //==================================================================
  // Internal versions that do not add to the trace.
  void ColorByPropertyInternal();
  void ColorByPointFieldComponentInternal(const char *name, int comp);
  void ColorByCellFieldComponentInternal(const char *name, int comp);
  void SetColorRangeInternal(float min, float max);
  void SetActorColor(float r, float g, float b);

  // Problems with vtkLODActor led me to use these.
  vtkProperty *Property;
  vtkProp *Prop;
  
  // Not properties does not mean the same thing as vtk.
  vtkKWFrame *Properties;
  vtkKWFrame *InformationsFrame;
  vtkKWLabel *NumCellsLabel;
  vtkKWLabel *NumPointsLabel;
  vtkKWBoundsDisplay *BoundsDisplay;
  
  vtkKWScale *AmbientScale;
  
  vtkKWLabeledFrame *ScalarBarFrame;
  vtkKWLabeledFrame *ColorFrame;
  vtkKWLabeledFrame *DisplayStyleFrame;
  vtkKWLabeledFrame *StatsFrame;
  vtkKWLabeledFrame *ViewFrame;
  
  vtkKWLabel *ColorMenuLabel;
  vtkKWOptionMenu *ColorMenu;

  vtkKWChangeColorButton *ColorButton;
  vtkKWPushButton *EditColorMapButton;
  
  vtkKWWidget *RepresentationMenuFrame;
  vtkKWLabel *RepresentationMenuLabel;
  vtkKWOptionMenu *RepresentationMenu;
  vtkKWWidget *InterpolationMenuFrame;
  vtkKWLabel *InterpolationMenuLabel;
  vtkKWOptionMenu *InterpolationMenu;

  vtkKWWidget *DisplayScalesFrame;
  vtkKWLabel *PointSizeLabel;
  vtkKWScale *PointSizeScale;
  vtkKWLabel *LineWidthLabel;
  vtkKWScale *LineWidthScale;
  
  vtkKWCheckButton *VisibilityCheck;
    
  char *PropTclName;
  vtkSetStringMacro(PropTclName);
  
  char *PropertyTclName;
  vtkSetStringMacro(PropertyTclName);
  
  char *MapperTclName;
  vtkSetStringMacro(MapperTclName);

  char *LODMapperTclName;
  vtkSetStringMacro(LODMapperTclName);
  
  char *LODDeciTclName;
  vtkSetStringMacro(LODDeciTclName);
  
  char *OutlineTclName;
  vtkSetStringMacro(OutlineTclName);
  
  char *GeometryTclName;
  vtkSetStringMacro(GeometryTclName);

  char *UpdateSuppressorTclName;
  vtkSetStringMacro(UpdateSuppressorTclName);
  
  char *LODUpdateSuppressorTclName;
  vtkSetStringMacro(LODUpdateSuppressorTclName);
  
  char *CollectTclName;
  vtkSetStringMacro(CollectTclName);

  char *LODCollectTclName;
  vtkSetStringMacro(LODCollectTclName);
  
  // Here to create unique names.
  int InstanceCount;

  // If the data changes, we need to change to.
  vtkTimeStamp UpdateTime;

  // True if CreateProperties() has been called.
  int PropertiesCreated;

  vtkKWWidget *ScalarBarCheckFrame;
  vtkKWCheckButton *ScalarBarCheck;
  
  // Stuff for setting the range of the color map.
  vtkKWWidget *ColorRangeFrame;
  vtkKWPushButton *ColorRangeResetButton;
  vtkKWLabeledEntry *ColorRangeMinEntry;
  vtkKWLabeledEntry *ColorRangeMaxEntry;

  // For translating actor
  vtkKWLabeledFrame* ActorControlFrame;
  vtkKWLabel* TranslateLabel;
  vtkKWEntry* TranslateEntry[3];
  vtkKWLabel* ScaleLabel;
  vtkKWEntry* ScaleEntry[3];
  vtkKWScale* Opacity;
  vtkKWLabel* OpacityLabel;

  vtkKWCheckButton *CubeAxesCheck;
  char* CubeAxesTclName;
  vtkSetStringMacro(CubeAxesTclName);

  vtkKWPushButton *ResetCameraButton;

  float PreviousAmbient;
  float PreviousDiffuse;
  float PreviousSpecular;
  int PreviousWasSolid;

  vtkPolyDataMapper *Mapper;

  vtkPVColorMap *PVColorMap;

  int LODResolution;
  float CollectThreshold;

  vtkPVData(const vtkPVData&); // Not implemented
  void operator=(const vtkPVData&); // Not implemented
};

#endif
