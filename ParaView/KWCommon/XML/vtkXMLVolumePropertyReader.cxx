/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkXMLVolumePropertyReader.h"

#include "vtkColorTransferFunction.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLColorTransferFunctionReader.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPiecewiseFunctionReader.h"
#include "vtkXMLVolumePropertyWriter.h"

vtkStandardNewMacro(vtkXMLVolumePropertyReader);
vtkCxxRevisionMacro(vtkXMLVolumePropertyReader, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLVolumePropertyReader::GetRootElementName()
{
  return "VolumeProperty";
}

//----------------------------------------------------------------------------
int vtkXMLVolumePropertyReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkVolumeProperty *obj = vtkVolumeProperty::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The VolumeProperty is not set!");
    return 0;
    }

  // Get attributes

  float fval;
  int ival;

  if (elem->GetScalarAttribute("InterpolationType", ival))
    {
    obj->SetInterpolationType(ival);
    }

  // Iterate over all components

  vtkXMLPiecewiseFunctionReader *xmlpfr =
    vtkXMLPiecewiseFunctionReader::New();

  vtkXMLColorTransferFunctionReader *xmlctfr = 
    vtkXMLColorTransferFunctionReader::New();

  int nb_nested_elems = elem->GetNumberOfNestedElements();
  for (int idx = 0; idx < nb_nested_elems; idx++)
    {
    vtkXMLDataElement *comp_elem = elem->GetNestedElement(idx);
    if (strcmp(comp_elem->GetName(), 
               vtkXMLVolumePropertyWriter::GetComponentElementName()))
      {
      continue;
      }

    int c_idx;
    if (!comp_elem->GetScalarAttribute("Index", c_idx) || 
        c_idx >= VTK_MAX_VRCOMP)
      {
      continue;
      }

    if (comp_elem->GetScalarAttribute("Shade", ival))
      {
      obj->SetShade(c_idx, ival);
      }

    if (comp_elem->GetScalarAttribute("Ambient", fval))
      {
      obj->SetAmbient(c_idx, fval);
      }

    if (comp_elem->GetScalarAttribute("Diffuse", fval))
      {
      obj->SetDiffuse(c_idx, fval);
      }

    if (comp_elem->GetScalarAttribute("Specular", fval))
      {
      obj->SetSpecular(c_idx, fval);
      }

    if (comp_elem->GetScalarAttribute("SpecularPower", fval))
      {
      obj->SetSpecularPower(c_idx, fval);
      }

    if (comp_elem->GetScalarAttribute("DisableGradientOpacity", ival))
      {
      obj->SetDisableGradientOpacity(c_idx, ival);
      }

    // Gray or Color Transfer Function
    
    int gtf_was_set = 0;
    if (xmlpfr->IsInNestedElement(
          comp_elem, 
          vtkXMLVolumePropertyWriter::GetGrayTransferFunctionElementName()))
      {
      vtkPiecewiseFunction *gtf = obj->GetGrayTransferFunction(c_idx);
      if (gtf)
        {
        xmlpfr->SetObject(gtf);
        xmlpfr->ParseInNestedElement(
          comp_elem,
          vtkXMLVolumePropertyWriter::GetGrayTransferFunctionElementName());
        gtf_was_set = 1;
        }
      }

    int rgbtf_was_set = 0;
    if (xmlctfr->IsInNestedElement(
          comp_elem, 
          vtkXMLVolumePropertyWriter::GetRGBTransferFunctionElementName()))
      {
      vtkColorTransferFunction *rgbtf = obj->GetRGBTransferFunction(c_idx);
      if (rgbtf)
        {
        xmlctfr->SetObject(rgbtf);
        xmlctfr->ParseInNestedElement(
          comp_elem,
          vtkXMLVolumePropertyWriter::GetRGBTransferFunctionElementName());
        rgbtf_was_set = 1;
        }
      }

    // If none of them was set, set it to NULL while keeping the same number
    // of channels

    if (!gtf_was_set && !rgbtf_was_set)
      {
      if (obj->GetColorChannels(c_idx) == 1)
        {
        obj->SetColor(c_idx, (vtkPiecewiseFunction *)NULL);
        }
      else
        {
        obj->SetColor(c_idx, (vtkColorTransferFunction *)NULL);
        }
      }
    
    // Scalar Opacity

    int sotf_was_set = 0;
    if (xmlpfr->IsInNestedElement(
          comp_elem, 
          vtkXMLVolumePropertyWriter::GetScalarOpacityElementName()))
      {
      vtkPiecewiseFunction *sotf = obj->GetScalarOpacity(c_idx);
      if (sotf)
        {
        xmlpfr->SetObject(sotf);
        xmlpfr->ParseInNestedElement(
          comp_elem,
          vtkXMLVolumePropertyWriter::GetScalarOpacityElementName());
        sotf_was_set = 1;
        }
      }
    if (!sotf_was_set)
      {
      obj->SetScalarOpacity(c_idx, NULL);
      }

    // Gradient Opacity

    int gotf_was_set = 0;
    if (xmlpfr->IsInNestedElement(
          comp_elem, 
          vtkXMLVolumePropertyWriter::GetGradientOpacityElementName()))
      {
      vtkPiecewiseFunction *gotf = obj->GetStoredGradientOpacity(c_idx);
      if (gotf)
        {
        xmlpfr->SetObject(gotf);
        xmlpfr->ParseInNestedElement(
          comp_elem,
          vtkXMLVolumePropertyWriter::GetGradientOpacityElementName());
        gotf_was_set = 1;
        }
      }
    if (!gotf_was_set)
      {
      obj->SetGradientOpacity(c_idx, NULL);
      }
    }

  xmlpfr->Delete();
  xmlctfr->Delete();
  
  return 1;
}


