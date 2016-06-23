/*=========================================================================

  Program:   ParaView
  Module:    vtkCompleteArrays.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompleteArrays - Filter adds arrays to empty partitions.
// .SECTION Description
// This is a temporary solution for fixing a writer bug.  When partition 0
// has no cells or points, it does not have arrays either.  The writers
// get confused.  This filter creates empty arrays on node zero if there
// are no cells or points in that partition.

#ifndef vtkCompleteArrays_h
#define vtkCompleteArrays_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkDataSetAlgorithm.h"

class vtkMultiProcessController;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkCompleteArrays : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkCompleteArrays,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkCompleteArrays *New();

  // Description:
  // The user can set the controller used for inter-process communication.
  void SetController(vtkMultiProcessController *controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkCompleteArrays();
  ~vtkCompleteArrays();

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  vtkMultiProcessController* Controller;
private:
  vtkCompleteArrays(const vtkCompleteArrays&);  // Not implemented.
  void operator=(const vtkCompleteArrays&);  // Not implemented.
};

#endif
