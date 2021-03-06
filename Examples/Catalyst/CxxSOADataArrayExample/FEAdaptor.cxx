#include <iostream>
#include "FEAdaptor.h"
#include "FEDataStructures.h"

#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>

#include "vtkSOADataArrayTemplate.h"

namespace
{
  vtkCPProcessor* Processor = NULL;
  vtkUnstructuredGrid* VTKGrid;

  void BuildVTKGrid(Grid& grid)
  {
    // create the points information
    vtkSOADataArrayTemplate<double>* pointArray = vtkSOADataArrayTemplate<double>::New();
    pointArray->SetNumberOfComponents(3);
    pointArray->SetNumberOfTuples(grid.GetNumberOfPoints());
    pointArray->SetArray(0, grid.GetPointsArray(), grid.GetNumberOfPoints(), false, true);
    pointArray->SetArray(1, grid.GetPointsArray()+grid.GetNumberOfPoints(), grid.GetNumberOfPoints(), false, true);
    pointArray->SetArray(2, grid.GetPointsArray()+2*grid.GetNumberOfPoints(), grid.GetNumberOfPoints(), false, true);

    vtkNew<vtkPoints> points;
    points->SetData(pointArray);
    pointArray->Delete();
    VTKGrid->SetPoints(points.GetPointer());

    // create the cells
    size_t numCells = grid.GetNumberOfCells();
    VTKGrid->Allocate(static_cast<vtkIdType>(numCells*9));
    for(size_t cell=0;cell<numCells;cell++)
      {
      unsigned int* cellPoints = grid.GetCellPoints(cell);
      vtkIdType tmp[8] = {cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3],
                          cellPoints[4], cellPoints[5], cellPoints[6], cellPoints[7]};
      VTKGrid->InsertNextCell(VTK_HEXAHEDRON, 8, tmp);
      }
  }

  void UpdateVTKAttributes(Grid& grid, Attributes& attributes)
  {
    if(VTKGrid->GetPointData()->GetNumberOfArrays() == 0)
      {
      // velocity array
      vtkSOADataArrayTemplate<double>* velocity = vtkSOADataArrayTemplate<double>::New();
      velocity->SetNumberOfComponents(3);
      velocity->SetNumberOfTuples(grid.GetNumberOfPoints());
      velocity->SetName("velocity");
      VTKGrid->GetPointData()->AddArray(velocity);
      velocity->Delete();
      }
    vtkSOADataArrayTemplate<double>* velocity =
      vtkSOADataArrayTemplate<double>::SafeDownCast(VTKGrid->GetPointData()->GetArray("velocity"));
    velocity->SetArray(0, attributes.GetVelocityArray(), grid.GetNumberOfPoints(), false, true);
    velocity->SetArray(1, attributes.GetVelocityArray()+grid.GetNumberOfPoints(), grid.GetNumberOfPoints(), false, true);
    velocity->SetArray(2, attributes.GetVelocityArray()+2*grid.GetNumberOfPoints(), grid.GetNumberOfPoints(), false, true);

    if(VTKGrid->GetCellData()->GetNumberOfArrays() == 0)
      {
      // pressure array
      vtkNew<vtkFloatArray> pressure;
      pressure->SetName("pressure");
      pressure->SetNumberOfComponents(1);
      VTKGrid->GetCellData()->AddArray(pressure.GetPointer());
      }
    vtkFloatArray* pressure = vtkFloatArray::SafeDownCast(
      VTKGrid->GetCellData()->GetArray("pressure"));
    // The pressure array is a scalar array so we can reuse
    // memory as long as we ordered the points properly.
    float* pressureData = attributes.GetPressureArray();
    pressure->SetArray(pressureData, static_cast<vtkIdType>(grid.GetNumberOfCells()), 1);
  }

  void BuildVTKDataStructures(Grid& grid, Attributes& attributes)
  {
    if(VTKGrid == NULL)
      {
      // The grid structure isn't changing so we only build it
      // the first time it's needed. If we needed the memory
      // we could delete it and rebuild as necessary.
      VTKGrid = vtkUnstructuredGrid::New();
      BuildVTKGrid(grid);
      }
    UpdateVTKAttributes(grid, attributes);
  }
}

namespace FEAdaptor
{

  void Initialize(int numScripts, char* scripts[])
  {
    if(Processor == NULL)
      {
      Processor = vtkCPProcessor::New();
      Processor->Initialize();
      }
    else
      {
      Processor->RemoveAllPipelines();
      }
    for(int i=1;i<numScripts;i++)
      {
      vtkNew<vtkCPPythonScriptPipeline> pipeline;
      pipeline->Initialize(scripts[i]);
      Processor->AddPipeline(pipeline.GetPointer());
      }
  }

  void Finalize()
  {
    if(Processor)
      {
      Processor->Delete();
      Processor = NULL;
      }
    if(VTKGrid)
      {
      VTKGrid->Delete();
      VTKGrid = NULL;
      }
  }

  void CoProcess(Grid& grid, Attributes& attributes, double time,
                 unsigned int timeStep, bool lastTimeStep)
  {
    vtkNew<vtkCPDataDescription> dataDescription;
    dataDescription->AddInput("input");
    dataDescription->SetTimeData(time, timeStep);
    if(lastTimeStep == true)
      {
      // assume that we want to all the pipelines to execute if it
      // is the last time step.
      dataDescription->ForceOutputOn();
      }
    if(Processor->RequestDataDescription(dataDescription.GetPointer()) != 0)
      {
      BuildVTKDataStructures(grid, attributes);
      dataDescription->GetInputDescriptionByName("input")->SetGrid(VTKGrid);
      Processor->CoProcess(dataDescription.GetPointer());
      }
  }
} // end of Catalyst namespace
