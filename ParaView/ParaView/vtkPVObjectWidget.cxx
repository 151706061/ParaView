/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVObjectWidget.cxx
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
#include "vtkPVObjectWidget.h"

#include "vtkArrayMap.txx"
#include "vtkKWSerializer.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkString.h"

vtkCxxRevisionMacro(vtkPVObjectWidget, "1.10");

//----------------------------------------------------------------------------
vtkPVObjectWidget::vtkPVObjectWidget()
{
  this->ObjectTclName = NULL;
  this->VariableName = NULL;
}

//----------------------------------------------------------------------------
vtkPVObjectWidget::~vtkPVObjectWidget()
{
  this->SetObjectTclName(NULL);
  this->SetVariableName(NULL);
}

//----------------------------------------------------------------------------
void vtkPVObjectWidget::SetObjectVariable(const char* objName, 
                                          const char* varName)
{
  this->SetObjectTclName(objName);
  this->SetVariableName(varName);
}

//----------------------------------------------------------------------------
void vtkPVObjectWidget::SaveInTclScript(ofstream *file)
{
  char *result;
  
  if (this->ObjectTclName == NULL || this->VariableName == NULL)
    {
    vtkErrorMacro(<< this->GetClassName() << " must not have SaveInTclScript method.");
    return;
    } 

  *file << "\t" << this->ObjectTclName << " Set" << this->VariableName;
  this->Script("set tempValue [%s Get%s]", 
               this->ObjectTclName, this->VariableName);
  result = this->Application->GetMainInterp()->result;
  *file << " " << result << "\n";
}

//----------------------------------------------------------------------------
vtkPVObjectWidget* vtkPVObjectWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVObjectWidget::SafeDownCast(clone);
}

//-----------------------------------------------------------------------------
void vtkPVObjectWidget::CopyProperties(vtkPVWidget* clone, 
                                       vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVObjectWidget* pvow = vtkPVObjectWidget::SafeDownCast(clone);
  if (pvow)
    {
    pvow->SetVariableName(this->VariableName);
    pvow->SetObjectTclName(pvSource->GetVTKSourceTclName());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVObjectWidget.");
    }
}

//----------------------------------------------------------------------------
int vtkPVObjectWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                          vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  const char* variable = element->GetAttribute("variable");
  if(variable)
    {
    this->SetVariableName(variable);
    }
  return 1;
}


//-----------------------------------------------------------------------------
void vtkPVObjectWidget::SerializeRevision(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeRevision(os,indent);
  os << indent << "vtkPVObjectWidget ";
  this->ExtractRevision(os,"$Revision: 1.10 $");
}

//----------------------------------------------------------------------------
void vtkPVObjectWidget::SerializeSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeSelf(os, indent);
  if ( this->ObjectTclName && this->VariableName )
    {
    os << indent << "Variable " << this->VariableName << " \""
       << this->Script("%s Get%s", this->ObjectTclName, this->VariableName)
       << " \"" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVObjectWidget::SerializeToken(istream& is, const char token[1024])
{
  if ( vtkString::Equals(token, "Variable") )
    {
    char varname[1024];
    char ntoken[1024];
    varname[0] = 0;
    if ( !( is >> varname ) )
      {
      vtkErrorMacro("Problem parsing variable name");
      return;
      }
    vtkKWSerializer::GetNextToken(&is,ntoken);
    //cout << "Variable: " << varname << " Value: " << ntoken << endl;
    this->Script("%s Set%s %s", this->ObjectTclName, varname, ntoken);
    }
  else
    {
    //cout << "Unknown Token for " << this->GetClassName() << ": " 
    //     << token << endl;
    this->Superclass::SerializeToken(is,token);  
    }
}

//----------------------------------------------------------------------------
void vtkPVObjectWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "ObjectTclName: " << (this->ObjectTclName?this->ObjectTclName:"none")
     << endl;
  os << "VariableName: " << (this->VariableName?this->VariableName:"none") 
     << endl;
}
