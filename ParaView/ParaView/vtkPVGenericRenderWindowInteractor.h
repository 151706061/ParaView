/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGenericRenderWindowInteractor.h
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
// .NAME vtkPVGenericRenderWindowInteractor
// .SECTION Description

#ifndef __vtkPVGenericRenderWindowInteractor_h
#define __vtkPVGenericRenderWindowInteractor_h

#include "vtkGenericRenderWindowInteractor.h"

class vtkPVRenderView;

class VTK_EXPORT vtkPVGenericRenderWindowInteractor : public vtkGenericRenderWindowInteractor
{
public:
  static vtkPVGenericRenderWindowInteractor *New();
  vtkTypeRevisionMacro(vtkPVGenericRenderWindowInteractor, vtkGenericRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetPVRenderView(vtkPVRenderView *view);
  vtkGetObjectMacro(PVRenderView, vtkPVRenderView);

  // Description:
  // Set the event onformation, but remember keys from before.
  void SetMoveEventInformationFlipY(int x, int y);

  virtual void Render();
  
  // Description:
  // Methods broadcasted to the satellites to synchronize 3D widgets.
  void SatelliteLeftPress(int x, int y, int control, int shift);
  void SatelliteMiddlePress(int x, int y, int control, int shift);
  void SatelliteRightPress(int x, int y, int control, int shift);
  void SatelliteLeftRelease(int x, int y, int control, int shift);
  void SatelliteMiddleRelease(int x, int y, int control, int shift);
  void SatelliteRightRelease(int x, int y, int control, int shift);
  void SatelliteMove(int x, int y);




protected:
  vtkPVGenericRenderWindowInteractor();
  ~vtkPVGenericRenderWindowInteractor();
  
  vtkPVRenderView *PVRenderView;
  int ReductionFactor;

private:
  vtkPVGenericRenderWindowInteractor(const vtkPVGenericRenderWindowInteractor&); // Not implemented
  void operator=(const vtkPVGenericRenderWindowInteractor&); // Not implemented
};

#endif
