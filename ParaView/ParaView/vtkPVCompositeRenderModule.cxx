/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRenderModule.cxx
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
#include "vtkPVCompositeRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkPVTreeComposite.h"
#include "vtkPVApplication.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPVProcessModule.h"
#include "vtkCallbackCommand.h"
#include "vtkPVCompositePartDisplay.h"
#include "vtkPVLODPartDisplayInformation.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCompositeRenderModule);
vtkCxxRevisionMacro(vtkPVCompositeRenderModule, "1.6");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVCompositeRenderModule::vtkPVCompositeRenderModule()
{
  this->LocalRender = 1;
  this->CollectThreshold = 4.0;

  this->Composite                = 0;
  this->CompositeTclName    = 0;
  this->InteractiveCompositeTime = 0;
  this->StillCompositeTime       = 0;

  this->CollectionDecision = 1;
  this->LODCollectionDecision = 1;

  this->UseReductionFactor = 1;
}

//----------------------------------------------------------------------------
vtkPVCompositeRenderModule::~vtkPVCompositeRenderModule()
{
  vtkPVApplication *pvApp = this->PVApplication;

  if (this->Composite && this->AbortCheckTag)
    {
    this->Composite->RemoveObserver(this->AbortCheckTag);
    this->AbortCheckTag = 0;
    }
 
  // Tree Composite
  if (this->CompositeTclName && pvApp)
    {
    pvApp->BroadcastScript("%s Delete", this->CompositeTclName);
    this->SetCompositeTclName(NULL);
    this->Composite = NULL;
    }
  else if (this->Composite)
    {
    this->Composite->Delete();
    this->Composite = NULL;
    }
}


//----------------------------------------------------------------------------
vtkPVPartDisplay* vtkPVCompositeRenderModule::CreatePartDisplay()
{
  return vtkPVCompositePartDisplay::New();
}



//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::StillRender()
{
  vtkObject* object;
  vtkPVCompositePartDisplay* pDisp;
  vtkPVLODPartDisplayInformation* info;
  unsigned long totalMemory = 0;
  int localRender;

  // Find out whether we are going to render localy.
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp->GetVisibility())
      {
      // This updates if required (collection disabled).
      info = pDisp->GetInformation();
      totalMemory += info->GetGeometryMemorySize();
      }
    }
  localRender = 0;
  // If using a RenderingGroup (i.e. vtkAllToNPolyData), do not do
  // local rendering
  if (!this->PVApplication->GetUseRenderingGroup() &&
      (float)(totalMemory)/1000.0 < this->GetCollectThreshold())
    {
    localRender = 1;
    }
  // Change the collection flags and update.
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp->GetVisibility())
      {
      pDisp->SetCollectionDecision(localRender);
      pDisp->Update();
      }
    }

  // Switch the compositer to local/composite mode.
  if (this->LocalRender != localRender)
    {
    if (this->CompositeTclName)
      {
      if (localRender)
        {
        this->PVApplication->Script("%s UseCompositingOff", this->CompositeTclName);
        }
      else
        {
        this->PVApplication->Script("%s UseCompositingOn", this->CompositeTclName);
        }
      this->LocalRender = localRender;
      }
    }


  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  this->RenderWindow->SetDesiredUpdateRate(0.002);
  // this->GetPVWindow()->GetInteractor()->GetStillUpdateRate());

  this->GetPVApplication()->SetGlobalLODFlag(0);
  vtkTimerLog::MarkStartEvent("Still Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::InteractiveRender()
{
  vtkObject* object;
  vtkPVCompositePartDisplay* pDisp;
  vtkPVLODPartDisplayInformation* info;
  unsigned long totalGeoMemory = 0;
  unsigned long totalLODMemory = 0;
  unsigned long tmpMemory;
  int localRender;
  int useLOD;

  // Compute memory totals.
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp->GetVisibility())
      {
      // This updates if required (collection disabled).
      info = pDisp->GetInformation();
      totalGeoMemory += info->GetGeometryMemorySize();
      totalLODMemory += info->GetLODGeometryMemorySize();
      }
    }

  // Make LOD decision.
  if ((float)(totalGeoMemory)/1000.0 < this->GetLODThreshold())
    {
    useLOD = 0;
    tmpMemory = totalGeoMemory;
    this->GetPVApplication()->SetGlobalLODFlag(0);
    }
  else
    {
    useLOD = 1;
    tmpMemory = totalLODMemory;
    this->GetPVApplication()->SetGlobalLODFlag(1);
    }

  // MakeCollection Decision.
  localRender = 0;
  if (!this->PVApplication->GetUseRenderingGroup() &&
      (float)(tmpMemory)/1000.0 < this->GetCollectThreshold())
    {
    localRender = 1;
    }
  // Change the collection flags and update.
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp->GetVisibility())
      {
      if (useLOD)
        {
        pDisp->SetLODCollectionDecision(localRender);
        }
      else
        {
        pDisp->SetCollectionDecision(localRender);
        }
      pDisp->Update();
      }
    }

  // Switch the compositer to local/composite mode.
  if (this->LocalRender != localRender)
    {
    if (this->CompositeTclName)
      {
      if (localRender)
        {
        this->PVApplication->Script("%s UseCompositingOff", this->CompositeTclName);
        }
      else
        {
        this->PVApplication->Script("%s UseCompositingOn", this->CompositeTclName);
        }
      this->LocalRender = localRender;
      }
    }

  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  // This might be used for Reduction factor.
  this->RenderWindow->SetDesiredUpdateRate(5.0);
  // this->GetPVWindow()->GetInteractor()->GetStillUpdateRate());

  // Compute reduction factor. 
  if (this->Composite && ! localRender)
    {
    this->ComputeReductionFactor();
    }

  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");

  // These times are used to determine reduction factor.
  // Not needed for still rendering !!!
  if (this->Composite)
    {
    this->InteractiveRenderTime = this->Composite->GetMaxRenderTime();
    this->InteractiveCompositeTime = this->Composite->GetCompositeTime()
      + this->Composite->GetGetBuffersTime()
      + this->Composite->GetSetBuffersTime();
    }
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::ComputeReductionFactor()
{
  float renderTime = 1.0 / this->RenderWindow->GetDesiredUpdateRate();
  int *windowSize = this->RenderWindow->GetSize();
  int area, reducedArea, reductionFactor;
  float timePerPixel;
  float getBuffersTime, setBuffersTime, transmitTime;
  float newReductionFactor;
  float maxReductionFactor;
  
  // Tiled displays do not use pixel reduction LOD.
  // This is not necessary because to caller already checks,
  // but it clarifies the situation.
  if (this->Composite == NULL)
    {
    return;
    }

  if (!this->UseReductionFactor)
    {
    this->Composite->SetReductionFactor(1);
    return;
    }
  
  // Do not let the width go below 150.
  maxReductionFactor = windowSize[0] / 150.0;

  renderTime *= 0.5;
  area = windowSize[0] * windowSize[1];
  reductionFactor = this->Composite->GetReductionFactor();
  reducedArea = area / (reductionFactor * reductionFactor);
  getBuffersTime = this->Composite->GetGetBuffersTime();
  setBuffersTime = this->Composite->GetSetBuffersTime();
  transmitTime = this->Composite->GetCompositeTime();

  // Do not consider SetBufferTime because 
  //it is not dependent on reduction factor.,
  timePerPixel = (getBuffersTime + transmitTime) / reducedArea;
  newReductionFactor = sqrt(area * timePerPixel / renderTime);
  
  if (newReductionFactor > maxReductionFactor)
    {
    newReductionFactor = maxReductionFactor;
    }
  if (newReductionFactor < 1.0)
    {
    newReductionFactor = 1.0;
    }
  
  this->Composite->SetReductionFactor((int)newReductionFactor);
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::SetCollectThreshold(float threshold)
{
  this->CollectThreshold = threshold;

  // This will cause collection to be re evaluated.
  this->SetTotalVisibleMemorySizeValid(0);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::SetUseCompositeWithFloat(int val)
{
  if (this->Composite)
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseChar %d",
                                              this->CompositeTclName,
                                              !val);
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Get color buffers as floats.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Get color buffers as unsigned char.");
    }

}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::SetUseCompositeWithRGBA(int val)
{
  if (this->Composite)
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseRGB %d",
                                              this->CompositeTclName,
                                              !val);
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Use RGBA pixels to get color buffers.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Use RGB pixels to get color buffers.");
    }
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::SetUseCompositeCompression(int val)
{
  if (this->Composite)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    if (val)
      {
      pvApp->BroadcastScript("vtkCompressCompositer pvTemp");
      }
    else
      {
      pvApp->BroadcastScript("vtkTreeCompositer pvTemp");
      }
    pvApp->BroadcastScript("%s SetCompositer pvTemp", this->CompositeTclName);
    pvApp->BroadcastScript("pvTemp Delete");
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable compression when compositing.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable compression when compositing.");
    }
}

//-----------------------------------------------------------------------------
int vtkPVCompositeRenderModule::MakeCollectionDecision()
{
  vtkObject* object;
  vtkPVCompositePartDisplay* pDisp;
  int decision = 1;

  // Do I really need to store the TotalVisibleMemorySIze in the application???
  if (this->GetTotalVisibleMemorySizeValid())
    {
    return this->CollectionDecision;
    }

  this->ComputeTotalVisibleMemorySize();
  this->SetTotalVisibleMemorySizeValid(1);

  if (this->TotalVisibleGeometryMemorySize > 
      this->GetCollectThreshold()*1000)
    {
    decision = 0;
    }

  if (decision == this->CollectionDecision)
    {
    return decision;
    }
  this->CollectionDecision = decision;
    
  this->PartDisplays->InitTraversal();
  while ( (object=this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp)
      {
      pDisp->SetCollectionDecision(this->CollectionDecision);
      }
    }

  return this->CollectionDecision;
}


//-----------------------------------------------------------------------------
int vtkPVCompositeRenderModule::MakeLODCollectionDecision()
{
  vtkObject* object;
  vtkPVCompositePartDisplay* pDisp;
  int decision = 1;

  if (this->GetTotalVisibleMemorySizeValid())
    {
    return this->LODCollectionDecision;
    }

  this->ComputeTotalVisibleMemorySize();
  this->SetTotalVisibleMemorySizeValid(1);
  if (this->TotalVisibleLODMemorySize > 
      this->GetCollectThreshold()*1000)
    {
    decision = 0;
    }

  if (decision == this->LODCollectionDecision)
    {
    return decision;
    }
  this->LODCollectionDecision = decision;
    
  this->PartDisplays->InitTraversal();
  while ( (object=this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp)
      {
      pDisp->SetLODCollectionDecision(this->LODCollectionDecision);
      }
    }

  return this->LODCollectionDecision;
}


//----------------------------------------------------------------------------
float vtkPVCompositeRenderModule::GetZBufferValue(int x, int y)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->LocalRender)
    {
    return this->Superclass::GetZBufferValue(x, y);
    }

  // Only MPI has a pointer to a composite.
  if (this->Composite)
    {
    return this->Composite->GetZ(x, y);
    }

  // If client-server...
  if (pvApp->GetClientMode())
    {
    float z;
    this->PVApplication->Script("%s GetZBufferValue %d %d",
                                this->CompositeTclName, x, y);
    z = this->PVApplication->GetFloatResult(this->PVApplication);
    return z;
    }

  vtkErrorMacro("Unknown RenderModule mode.");
  return 0;
}



//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CollectThreshold: " << this->CollectThreshold << endl;

  os << indent << "UseReductionFactor: " << this->UseReductionFactor << endl;
  if (this->CompositeTclName)
    {
    os << indent << "CompositeTclName: " << this->CompositeTclName << endl;
    }

  os << indent << "InteractiveCompositeTime: " 
     << this->GetInteractiveCompositeTime() << endl;
  os << indent << "StillCompositeTime: " 
     << this->GetStillCompositeTime() << endl;
}

