/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImplicitPlaneRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImplicitPlaneRepresentation - extends vtkImplicitPlaneRepresentation
// .SECTION Description
// vtkPVImplicitPlaneRepresentation extends vtkImplicitPlaneRepresentation to
// add ParaView proper initialisation values

#ifndef vtkPVImplicitPlaneRepresentation_h
#define vtkPVImplicitPlaneRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkImplicitPlaneRepresentation.h"


class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVImplicitPlaneRepresentation
  : public vtkImplicitPlaneRepresentation
{
public:
  static vtkPVImplicitPlaneRepresentation* New();
  vtkTypeMacro(vtkPVImplicitPlaneRepresentation, vtkImplicitPlaneRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVImplicitPlaneRepresentation();
  ~vtkPVImplicitPlaneRepresentation();

private:
  vtkPVImplicitPlaneRepresentation(const vtkPVImplicitPlaneRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVImplicitPlaneRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
