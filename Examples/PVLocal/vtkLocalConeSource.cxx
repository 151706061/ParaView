/*=========================================================================
This source has no copyright.  It is intended to be copied by users
wishing to create their own ParaView plugin classes locally.
=========================================================================*/
#include "vtkLocalConeSource.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkLocalConeSource, "1.3");
vtkStandardNewMacro(vtkLocalConeSource);

//----------------------------------------------------------------------------
vtkLocalConeSource::vtkLocalConeSource(int /*res*/)
{
}

//----------------------------------------------------------------------------
vtkLocalConeSource::~vtkLocalConeSource()
{
}

//----------------------------------------------------------------------------
void vtkLocalConeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
