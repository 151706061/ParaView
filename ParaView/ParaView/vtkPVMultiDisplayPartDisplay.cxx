/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayPartDisplay.cxx
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
#include "vtkPVMultiDisplayPartDisplay.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkMultiProcessController.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiDisplayPartDisplay);
vtkCxxRevisionMacro(vtkPVMultiDisplayPartDisplay, "1.5");


//----------------------------------------------------------------------------
vtkPVMultiDisplayPartDisplay::vtkPVMultiDisplayPartDisplay()
{
}

//----------------------------------------------------------------------------
vtkPVMultiDisplayPartDisplay::~vtkPVMultiDisplayPartDisplay()
{
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::SetLODCollectionDecision(int)
{
  // Always colect LOD.
  this->Superclass::SetLODCollectionDecision(1);
}



//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  this->Superclass::CreateParallelTclObjects(pvApp);

  // Special case when TileDisplay is not running client server.
  if ( ! pvApp->GetClientMode())
    {
    pvApp->BroadcastScript("%s ZeroEmptyOn; %s ZeroEmptyOn",
                          this->CollectTclName,
                          this->LODCollectTclName);

    // Broadcast for subclasses.  
    pvApp->BroadcastScript("%s SetUpdateNumberOfPieces [expr {[[$Application GetProcessModule] GetNumberOfPartitions]-1}]",
                          this->LODUpdateSuppressorTclName);
    pvApp->BroadcastScript("%s SetUpdatePiece [expr {[[$Application GetProcessModule] GetPartitionId]-1}]",
                          this->LODUpdateSuppressorTclName);
    pvApp->BroadcastScript("%s SetUpdateNumberOfPieces [expr {[[$Application GetProcessModule] GetNumberOfPartitions]-1}]",
                          this->UpdateSuppressorTclName);
    pvApp->BroadcastScript("%s SetUpdatePiece [expr {[[$Application GetProcessModule] GetPartitionId]-1}]",
                          this->UpdateSuppressorTclName);
    // Local pipeline has no data, or all of the data?
    }
  else
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    pvApp->Script("%s SetClientFlag 1; %s SetClientFlag 1",
                  this->CollectTclName,
                  this->LODCollectTclName);
    }   

  int* dims = pvApp->GetTileDimensions();
  int numProcs = pvApp->GetController()->GetNumberOfProcesses();
  pvApp->BroadcastScript("%s InitializeSchedule %d %d; %s InitializeSchedule %d %d", 
                         this->CollectTclName, numProcs, dims[0]*dims[1],
                         this->LODCollectTclName, numProcs, dims[0]*dims[1]);
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


  



