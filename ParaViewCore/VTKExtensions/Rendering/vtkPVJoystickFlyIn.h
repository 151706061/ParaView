/*=========================================================================

  Program:   ParaView
  Module:    vtkPVJoystickFlyIn.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVJoystickFlyIn - Rotates camera with xy mouse movement.
// .SECTION Description
// vtkPVJoystickFlyIn allows the user to interactively
// manipulate the camera, the viewpoint of the scene.

#ifndef vtkPVJoystickFlyIn_h
#define vtkPVJoystickFlyIn_h

#include "vtkPVJoystickFly.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVJoystickFlyIn : public vtkPVJoystickFly
{
public:
  static vtkPVJoystickFlyIn *New();
  vtkTypeMacro(vtkPVJoystickFlyIn, vtkPVJoystickFly);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkPVJoystickFlyIn();
  ~vtkPVJoystickFlyIn();

private:
  vtkPVJoystickFlyIn(const vtkPVJoystickFlyIn&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVJoystickFlyIn&) VTK_DELETE_FUNCTION;
};

#endif
