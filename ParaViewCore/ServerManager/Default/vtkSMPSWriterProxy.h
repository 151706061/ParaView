/*=========================================================================

Program:   ParaView
Module:    vtkSMPSWriterProxy.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPSWriterProxy - proxy for the parallel-serial writer.
// .SECTION Description
// vtkSMPSWriterProxy is the proxy for all vtkParallelSerialWriter
// objects. It is responsible of setting the internal writer that is
// configured as a sub-proxy.

#ifndef vtkSMPSWriterProxy_h
#define vtkSMPSWriterProxy_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMPWriterProxy.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMPSWriterProxy : public vtkSMPWriterProxy
{
public:
  static vtkSMPSWriterProxy* New();
  vtkTypeMacro(vtkSMPSWriterProxy, vtkSMPWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMPSWriterProxy();
  ~vtkSMPSWriterProxy();

private:
  vtkSMPSWriterProxy(const vtkSMPSWriterProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMPSWriterProxy&) VTK_DELETE_FUNCTION;
};


#endif

