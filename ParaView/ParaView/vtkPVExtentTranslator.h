/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtentTranslator.h
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
// .NAME vtkPVExtentTranslator - Uses alternative source for whole extent.
// .SECTION Description
// vtkPVExtentTranslator is like extent translator, but it uses an 
// alternative source as a whole extent. The whole extent passed is assumed 
// to be a subextent of the original source.  we simple take the intersection 
// of the split extent and the whole extdent passed in.  We are attempting to
// make branning piplines request consistent extents with the same piece 
// requests.  

// .SECTION Caveats
// This object is still under development.

#ifndef __vtkPVExtentTranslator_h
#define __vtkPVExtentTranslator_h

#include "vtkExtentTranslator.h"

class vtkDataSet;

class VTK_EXPORT vtkPVExtentTranslator : public vtkExtentTranslator
{
public:
  static vtkPVExtentTranslator *New();

  vtkTypeRevisionMacro(vtkPVExtentTranslator,vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the original upstream data set
  virtual void SetOriginalSource(vtkDataSet*);
  vtkGetObjectMacro(OriginalSource,vtkDataSet);

  virtual int ThreadSafePieceToExtent(int piece, int numPieces, 
                                      int ghostLevel, int *wholeExtent, 
                                      int *resultExtent, int splitMode, 
                                      int byPoints);

protected:
  vtkPVExtentTranslator();
  ~vtkPVExtentTranslator();

  vtkDataSet *OriginalSource;

  vtkPVExtentTranslator(const vtkPVExtentTranslator&); // Not implemented
  void operator=(const vtkPVExtentTranslator&); // Not implemented
};

#endif
