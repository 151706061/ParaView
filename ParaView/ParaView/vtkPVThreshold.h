/*=========================================================================

  Program:   ParaView
  Module:    vtkPVThreshold.h
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
// .NAME vtkPVThreshold - A class to handle the UI for vtkThreshold
// .SECTION Description


#ifndef __vtkPVThreshold_h
#define __vtkPVThreshold_h

#include "vtkPVSource.h"

class vtkKWOptionMenu;
class vtkKWWidget;
class vtkKWLabel;
class vtkKWOptionMenu;
class vtkPVMinMax;
class vtkPVLabeledToggle;

class VTK_EXPORT vtkPVThreshold : public vtkPVSource
{
public:
  static vtkPVThreshold* New();
  vtkTypeRevisionMacro(vtkPVThreshold, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Tcl callback for the attribute mode option menu
  void ChangeAttributeMode(const char* newMode);

  // Description:
  // Save this source to a file.
  void SaveInTclScript(ofstream *file, int interactiveFlag, int vtkFlag=0);

  virtual void UpdateScalars();

  // Description:
  // Access to AttributeModeMenu from tcl
  vtkGetObjectMacro(AttributeModeMenu, vtkKWOptionMenu);
  
  // Description:
  // Set the widget values to reflect the vtk object.
  virtual void UpdateParameterWidgets();

protected:
  vtkPVThreshold();
  ~vtkPVThreshold();

  void UpdateMinMaxScale();


  vtkKWWidget* AttributeModeFrame;
  vtkKWLabel* AttributeModeLabel;
  vtkKWOptionMenu* AttributeModeMenu;
  vtkPVMinMax *MinMaxScale;
  vtkPVLabeledToggle* AllScalarsCheck;
  
  int TraceInitialized;

  vtkPVThreshold(const vtkPVThreshold&); // Not implemented
  void operator=(const vtkPVThreshold&); // Not implemented
};

#endif
