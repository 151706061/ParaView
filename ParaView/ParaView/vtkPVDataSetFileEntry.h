/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataSetFileEntry.h
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
// .NAME vtkPVDataSetFileEntry - File entry that checks type.
// .SECTION Description
// This file entry will accept VTK files.  First time any VTK file
// will do.  Once a file is picked, then any following file has to have 
// the same data type.

#ifndef __vtkPVDataSetFileEntry_h
#define __vtkPVDataSetFileEntry_h

#include "vtkPVFileEntry.h"

class vtkPDataSetReader;

class VTK_EXPORT vtkPVDataSetFileEntry : public vtkPVFileEntry
{
public:
  static vtkPVDataSetFileEntry* New();
  vtkTypeRevisionMacro(vtkPVDataSetFileEntry, vtkPVFileEntry);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Adds a trace entry.  Side effect is to turn modified flag off.
  virtual void Accept();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVDataSetFileEntry* ClonePrototype(vtkPVSource* pvSource,
                                        vtkArrayMap<vtkPVWidget*, 
                                        vtkPVWidget*>* map);
//ETX

protected:
  vtkPVDataSetFileEntry();
  ~vtkPVDataSetFileEntry();
  
  vtkPDataSetReader *TypeReader;
  int Type;

  vtkPVDataSetFileEntry(const vtkPVDataSetFileEntry&); // Not implemented
  void operator=(const vtkPVDataSetFileEntry&); // Not implemented
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
