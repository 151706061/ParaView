/*=========================================================================

  Program:   ParaView
  Module:    vtkSIProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIProxy.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVInstantiator.h"
#include "vtkPVSessionCore.h"
#include "vtkPVXMLElement.h"
#include "vtkSIProperty.h"
#include "vtkSIProxyDefinitionManager.h"
#include "vtkSmartPointer.h"
#include "vtkSMMessage.h"

#include <map>
#include <set>
#include <string>
#include <sstream>

//****************************************************************************
struct SubProxyInfo
{
  SubProxyInfo(std::string name, vtkTypeUInt32 id)
    {
    this->Name = name;
    this->GlobalID = id;
    }

  std::string Name;
  vtkTypeUInt32 GlobalID;
};
//****************************************************************************
class vtkSIProxy::vtkInternals
{
public:

  void ClearDependencies()
    {
    this->SIProperties.clear();
    this->SubSIProxies.clear();
    }

  typedef std::map<std::string, vtkSmartPointer<vtkSIProperty> >
    SIPropertiesMapType;
  SIPropertiesMapType SIProperties;

  typedef std::map<std::string, vtkSmartPointer<vtkSIProxy> >
    SubSIProxiesMapType;
  SubSIProxiesMapType SubSIProxies;

  typedef std::vector<SubProxyInfo> SubProxiesVectorType;
  SubProxiesVectorType SubProxyInfoVector;

  typedef std::map<std::string, std::string> SubProxyCommandMapType;
  SubProxyCommandMapType SubProxyCommands;
};

//****************************************************************************
vtkStandardNewMacro(vtkSIProxy);
//----------------------------------------------------------------------------
vtkSIProxy::vtkSIProxy()
{
  this->Internals = new vtkInternals();
  this->VTKObject = NULL;
  this->ObjectsCreated = false;

  this->XMLGroup = 0;
  this->XMLName = 0;
  this->XMLSubProxyName = 0;
  this->VTKClassName = 0;
  this->PostPush = 0;
  this->PostCreation = 0;
}

//----------------------------------------------------------------------------
vtkSIProxy::~vtkSIProxy()
{
  this->DeleteVTKObjects();

  delete this->Internals;
  this->Internals = 0;

  this->SetXMLGroup(0);
  this->SetXMLName(0);
  this->SetXMLSubProxyName(0);
  this->SetVTKClassName(0);
  this->SetPostPush(0);
  this->SetPostCreation(0);
}

//----------------------------------------------------------------------------
void vtkSIProxy::SetVTKObject(vtkObjectBase* obj)
{
  this->VTKObject = obj;
}

//----------------------------------------------------------------------------
void vtkSIProxy::Push(vtkSMMessage* message)
{
  if (!this->CreateVTKObjects(message))
    {
    return;
    }

  // Handle properties
  int cc = 0;
  int size = message->ExtensionSize(ProxyState::property);
  for (;cc < size; cc++)
    {
    const ProxyState_Property &propMsg =
      message->GetExtension(ProxyState::property, cc);

    // Convert state to interpretor stream
    vtkSIProperty* prop = this->GetSIProperty(propMsg.name().c_str());
    if (prop)
      {
      if (prop->Push(message, cc) == false)
        {
        vtkErrorMacro("Error pushing property state: " << propMsg.name());
        message->PrintDebugString();
        return;
        }
      }
    }

  // Execute post_push if any
  if(this->PostPush != NULL)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << this->PostPush
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    }
}

//----------------------------------------------------------------------------
void vtkSIProxy::Pull(vtkSMMessage* message)
{
  if (!this->ObjectsCreated)
    {
    return;
    }

  // Return a set of Pull only property (information_only props)
  // In fact Pushed Property can not be fetch at the same time as Pull
  // property with the current implementation
  std::set<std::string> prop_names;
  if (message->ExtensionSize(PullRequest::arguments) > 0)
    {
    const Variant *propList = &message->GetExtension(PullRequest::arguments, 0);
    for(int i=0; i < propList->txt_size(); ++i)
      {
      const char* propertyName = propList->txt(i).c_str();
      prop_names.insert(propertyName);
      }
    }

  message->ClearExtension(PullRequest::arguments);

  vtkInternals::SIPropertiesMapType::iterator iter;
  for (iter = this->Internals->SIProperties.begin(); iter !=
    this->Internals->SIProperties.end(); ++iter)
    {
    if (prop_names.size() == 0 ||
      prop_names.find(iter->first) != prop_names.end())
      {
      if(!iter->second->GetIsInternal())
        {
        if(message->req_def())
          {
          // We just want the cached push property
          if( !iter->second->GetInformationOnly() &&
              !iter->second->Pull(message))
            {
            vtkErrorMacro("Error pulling property state: " << iter->first);
            return;
            }
          }
        else if (!iter->second->Pull(message))
          {
          vtkErrorMacro("Error pulling property state: " << iter->first);
          return;
          }
        }
      }
    }

  if(message->req_def())
    {
    // Add definition
    message->SetExtension(ProxyState::xml_group, this->XMLGroup);
    message->SetExtension(ProxyState::xml_name, this->XMLName);
    if(this->XMLSubProxyName)
      {
      message->SetExtension(ProxyState::xml_sub_proxy_name, this->XMLSubProxyName);
      }

    // Add subproxy information to the message.
    vtkInternals::SubProxiesVectorType::iterator it2 =
        this->Internals->SubProxyInfoVector.begin();
    for( ; it2 != this->Internals->SubProxyInfoVector.end(); it2++)
      {
      ProxyState_SubProxy *subproxy = message->AddExtension(ProxyState::subproxy);
      subproxy->set_name(it2->Name.c_str());
      subproxy->set_global_id(it2->GlobalID);
      }
    }
}
//----------------------------------------------------------------------------
vtkSIProxyDefinitionManager* vtkSIProxy::GetProxyDefinitionManager()
{
  if (this->SessionCore)
    {
    return this->SessionCore->GetProxyDefinitionManager();
    }

  vtkWarningMacro("No valid session provided. "
    "This class may not have been initialized yet.");
  return NULL;
}

//----------------------------------------------------------------------------
vtkSIProperty* vtkSIProxy::GetSIProperty(const char* name)
{
  vtkInternals::SIPropertiesMapType::iterator iter =
    this->Internals->SIProperties.find(name);
  if (iter != this->Internals->SIProperties.end())
    {
    return iter->second.GetPointer();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSIProxy::AddSIProperty(const char* name, vtkSIProperty* property)
{
  this->Internals->SIProperties[name] = property;
}

//----------------------------------------------------------------------------
bool vtkSIProxy::CreateVTKObjects(vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }

  if (!message->HasExtension(ProxyState::xml_group) ||
    !message->HasExtension(ProxyState::xml_name))
    {
    vtkErrorMacro("Incorrect message received. "
                  << "Missing xml_group and xml_name information." << endl
                  << message->DebugString().c_str() << endl);
    return false;
    }

  // Store definition informations
  this->SetXMLGroup(message->GetExtension(ProxyState::xml_group).c_str());
  this->SetXMLName(message->GetExtension(ProxyState::xml_name).c_str());
  this->SetXMLSubProxyName(
      message->HasExtension(ProxyState::xml_sub_proxy_name) ?
      message->GetExtension(ProxyState::xml_sub_proxy_name).c_str() : NULL);

  vtkSIProxyDefinitionManager* pdm = this->GetProxyDefinitionManager();
  vtkPVXMLElement* element = pdm->GetCollapsedProxyDefinition(
    message->GetExtension(ProxyState::xml_group).c_str(),
    message->GetExtension(ProxyState::xml_name).c_str(),
    (message->HasExtension(ProxyState::xml_sub_proxy_name) ?
     message->GetExtension(ProxyState::xml_sub_proxy_name).c_str() :
     NULL));

  if (!element)
    {
    vtkErrorMacro("Definition not found for xml_group: "
                  << message->GetExtension(ProxyState::xml_group).c_str()
                  << " and xml_name: "
                  << message->GetExtension(ProxyState::xml_name).c_str()
                  << endl << message->DebugString().c_str() << endl );
    return false;
    }

  // Create and setup the VTK object, if needed before parsing the property
  // helpers. This is needed so that the property helpers can push their default
  // values as they are reading the xml-attributes.
  const char* className = element->GetAttribute("class");
  if (className && className[0])
    {
    this->SetVTKClassName(className);
    vtkObjectBase* obj = this->Interpreter->NewInstance(className);
    if (!obj)
      {
      vtkErrorMacro("Failed to create " << className
        << ". Aborting for debugging purposes.");
      abort();
      }
    this->VTKObject.TakeReference(obj);
    }

  if (this->VTKClassName && this->VTKClassName[0] != '\0')
    {
    vtkClientServerStream substream;
    substream << vtkClientServerStream::Invoke
              << vtkClientServerID(1)
              << "GetActiveProgressHandler"
              << vtkClientServerStream::End;
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << substream
           << "RegisterProgressEvent"
           << this->VTKObject
           << static_cast<int>(this->GetGlobalID())
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    }

  this->SetXMLGroup(message->GetExtension(ProxyState::xml_group).c_str());
  this->SetXMLName(message->GetExtension(ProxyState::xml_name).c_str());

  // Locate sub-proxies.
  for (int cc=0; cc < message->ExtensionSize(ProxyState::subproxy); cc++)
    {
    const ProxyState_SubProxy& subproxyMsg =
      message->GetExtension(ProxyState::subproxy, cc);
    vtkSIProxy* subproxy = vtkSIProxy::SafeDownCast(
      this->GetSIObject(subproxyMsg.global_id()));
    this->Internals->SubProxyInfoVector.push_back(
        SubProxyInfo(subproxyMsg.name(), subproxyMsg.global_id()));
    if (subproxy == NULL)
      {
      // This code has been commented to support ImplicitPlaneWidgetRepresentation
      // which as a widget as SubProxy which stay on the client side.
      // Therefore, when ParaView is running on Client/Server mode, that SubProxy
      // does NOT exist on the Server side. This case should not fail the current
      // proxy creation.

      // vtkErrorMacro("Failed to locate subproxy with global-id: "
      //                << subproxyMsg.global_id() << endl
      //                << message->DebugString().c_str());
      // return false;
      }
    else
      {
      this->Internals->SubSIProxies[subproxyMsg.name()] = subproxy;
      }
    }

  // Add hook for post_push and post_creation. This is processed before
  // ReadXMLAttributes() is even called.
  this->SetPostPush(element->GetAttribute("post_push"));
  this->SetPostCreation(element->GetAttribute("post_creation"));

  // Allow subclasses to do some initialization if needed. Note this is called
  // before properties are created.
  this->OnCreateVTKObjects();

  // Set the number of input ports
  // This will only work for vtkAlgorithm subclasses that explicitly expose the
  // otherwise protected method SetNumberOfInputPorts
  int numberOfInputPorts=0;
  if (element->GetScalarAttribute("input_ports", &numberOfInputPorts))
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << "SetNumberOfInputPorts"
           << numberOfInputPorts
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    }

  // Execute post-creation if any
  if(this->PostCreation != NULL)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << this->PostCreation
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    }

  // Process the XML and update properties etc.
  if (!this->ReadXMLAttributes(element))
    {
    this->DeleteVTKObjects();
    return false;
    }

  this->ObjectsCreated = true;
  return true;
}

//----------------------------------------------------------------------------
void vtkSIProxy::DeleteVTKObjects()
{
  this->VTKObject =  NULL;
}

//----------------------------------------------------------------------------
void vtkSIProxy::OnCreateVTKObjects()
{
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkSIProxy::GetVTKObject()
{
  return this->VTKObject.GetPointer();
}

//----------------------------------------------------------------------------
bool vtkSIProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  for(unsigned int i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* propElement = element->GetNestedElement(i);

    if (strcmp(propElement->GetName(), "SubProxy") == 0)
      {
      // read subproxy xml.
      if (!this->ReadXMLSubProxy(propElement))
        {
        return false;
        }
      }
    }

  // Process sub-proxy commands.
  for (vtkInternals::SubProxyCommandMapType::iterator iter=this->Internals->SubProxyCommands.begin();
    iter != this->Internals->SubProxyCommands.end(); ++iter)
    {
    if (vtkSIProxy* subProxy = this->GetSubSIProxy(iter->first.c_str()))
      {
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke
             << this->GetVTKObject()
             << iter->second.c_str()
             << subProxy->GetVTKObject()
             << vtkClientServerStream::End;
      this->Interpreter->ProcessStream(stream);
      }
    }

  for (unsigned int i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* propElement = element->GetNestedElement(i);
    // read property xml
    const char* name = propElement->GetAttribute("name");
    std::string tagName = propElement->GetName();
    if (name && tagName.find("Property") == (tagName.size()-8))
      {
      if (!this->ReadXMLProperty(propElement))
        {
        return false;
        }
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSIProxy::ReadXMLSubProxy(vtkPVXMLElement* subproxyElement)
{
  // Process "command" for the sub proxy, if any. These are used in
  // CreateVTKObjects() to pass the sub proxy's VTK object to this proxy's VTK
  // object.
  const char* command = subproxyElement? subproxyElement->GetAttribute("command") : NULL;
  if (command)
    {
    vtkPVXMLElement* proxyElement = subproxyElement->GetNestedElement(0);
    const char* name = proxyElement? proxyElement->GetAttribute("name") : NULL;
    if (name)
      {
      this->Internals->SubProxyCommands[name] = command;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSIProxy::ReadXMLProperty(vtkPVXMLElement* propElement)
{
  // Since the XML is "cleaned" out, we are assured that there are no duplicate
  // properties.
  std::string name = propElement->GetAttributeOrEmpty("name");
  assert(!name.empty() && this->GetSIProperty(name.c_str()) == NULL);

  // Patch XML to remove InformationHelper and set right si_class
  vtkSIProxyDefinitionManager::PatchXMLProperty(propElement);

  vtkSmartPointer<vtkObject> object;
  std::string classname;
  if (propElement->GetAttribute("si_class"))
    {
    classname = propElement->GetAttribute("si_class");
    }
  else
    {
    std::ostringstream cname;
    cname << "vtkSI" << propElement->GetName() << ends;
    classname = cname.str();
    }

  object.TakeReference(vtkPVInstantiator::CreateInstance(classname.c_str()));
  if (!object)
    {
    vtkErrorMacro("Failed to create helper for property: " << classname);
    return false;
    }
  vtkSIProperty* property = vtkSIProperty::SafeDownCast(object);
  if (!property)
    {
    vtkErrorMacro(<< classname << " must be a vtkSIProperty subclass.");
    return false;
    }

  if (!property->ReadXMLAttributes(this, propElement))
    {
    vtkErrorMacro("Could not parse property: " << name.c_str());
    return false;
    }

  this->AddSIProperty(name.c_str(), property);
  return true;
}

//----------------------------------------------------------------------------
vtkSIProxy* vtkSIProxy::GetSubSIProxy(const char* name)
{
  return this->Internals->SubSIProxies[name];
}

//----------------------------------------------------------------------------
unsigned int vtkSIProxy::GetNumberOfSubSIProxys()
{
  return static_cast<unsigned int>(this->Internals->SubSIProxies.size());
}
//----------------------------------------------------------------------------
vtkSIProxy* vtkSIProxy::GetSubSIProxy(unsigned int cc)
{
  if (cc >= this->GetNumberOfSubSIProxys())
    {
    return NULL;
    }

  unsigned int index=0;
  vtkInternals::SubSIProxiesMapType::iterator iter;
  for (iter = this->Internals->SubSIProxies.begin();
    iter != this->Internals->SubSIProxies.end();
    ++iter, ++index)
    {
    if (index == cc)
      {
      return iter->second;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSIProxy::AddInput(
  int inputPort, vtkAlgorithmOutput* connection, const char* method)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetVTKObject()
         << method;
  if (inputPort > 0)
    {
    stream << inputPort;
    }
  stream << connection << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
}

//----------------------------------------------------------------------------
void vtkSIProxy::CleanInputs(const char* method)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetVTKObject()
         << method
         << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
}

//----------------------------------------------------------------------------
void vtkSIProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkSIProxy::AboutToDelete()
{
  // Remove all proxy/input property that still old other SIProxy reference...
  this->Internals->ClearDependencies();
}
