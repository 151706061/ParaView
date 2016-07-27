/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMoleculeRepresentation.h"

#include "vtkView.h"
#include "vtkActor.h"
#include "vtkMolecule.h"
#include "vtkRenderer.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVRenderView.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkMoleculeMapper.h"
#include "vtkInformationVector.h"

vtkStandardNewMacro(vtkMoleculeRepresentation)

vtkMoleculeRepresentation::vtkMoleculeRepresentation()
{
  // setup mapper
  this->Mapper = vtkMoleculeMapper::New();
  this->Mapper->UseBallAndStickSettings();

  // setup actor
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);

  // initialize cache:
  this->CacheKeeper->SetInputData(this->DummyMolecule.Get());
}

vtkMoleculeRepresentation::~vtkMoleculeRepresentation()
{
  this->Actor->Delete();
  this->Mapper->Delete();
}

void vtkMoleculeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkMoleculeRepresentation::SetMoleculeRenderMode(int mode)
{
  if(mode == 0)
    {
    this->Mapper->UseBallAndStickSettings();
    }
  else if(mode == 1)
    {
    this->Mapper->UseVDWSpheresSettings();
    }
  else if(mode == 2)
    {
    this->Mapper->UseLiquoriceStickSettings();
    }
}

void vtkMoleculeRepresentation::MarkModified()
{
  if (!this->GetUseCache())
    {
    // Clear cache when caching is turned off:
    this->CacheKeeper->RemoveAllCaches();
    }
  this->Superclass::MarkModified();
}

int vtkMoleculeRepresentation::ProcessViewRequest(
    vtkInformationRequestKey *request_type, vtkInformation *inInfo,
    vtkInformation *outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
    {
    return 0;
    }

  if (request_type == vtkPVView::REQUEST_UPDATE())
    {
    vtkPVRenderView::SetPiece(inInfo, this, this->CacheKeeper->GetOutput());
    vtkPVRenderView::SetDeliverToClientAndRenderingProcesses(inInfo, this,
                                                             true, false);
    }
  else if (request_type == vtkPVView::REQUEST_RENDER())
    {
    vtkAlgorithmOutput *producerPort =
        vtkPVRenderView::GetPieceProducer(inInfo, this);

    this->Mapper->SetInputConnection(producerPort);
    }

  return 1;
}

void vtkMoleculeRepresentation::SetVisibility(bool value)
{
  this->Superclass::SetVisibility(value);
  this->Actor->SetVisibility(value);
}

int vtkMoleculeRepresentation::FillInputPortInformation(int vtkNotUsed(port),
                                                        vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMolecule");

  // Saying INPUT_IS_OPTIONAL() is essential, since representations don't have
  // any inputs on client-side (in client-server, client-render-server mode) and
  // render-server-side (in client-render-server mode).
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

int vtkMoleculeRepresentation::RequestData(vtkInformation *request,
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkMolecule* input =
      vtkMolecule::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    this->DummyMolecule->ShallowCopy(input);
    }

  this->DummyMolecule->Modified();
  this->CacheKeeper->Update();

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

bool vtkMoleculeRepresentation::AddToView(vtkView *view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);
    return this->Superclass::AddToView(view);
    }
  return false;
}

bool vtkMoleculeRepresentation::RemoveFromView(vtkView *view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return this->Superclass::RemoveFromView(view);
    }
  return false;
}

bool vtkMoleculeRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}
