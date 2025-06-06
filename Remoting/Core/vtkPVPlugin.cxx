// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVPlugin.h"

#include "vtkPVLogger.h"
#include "vtkPVPluginTracker.h"
#include "vtkProcessModule.h"
#include <vtksys/SystemTools.hxx>

#include <cassert>
#include <sstream>

vtkPVPlugin::EULAConfirmationCallback vtkPVPlugin::EULAConfirmationCallbackPtr = nullptr;

//-----------------------------------------------------------------------------
bool vtkPVPlugin::ImportPlugin(vtkPVPlugin* plugin)
{
  std::ostringstream msg;
  bool status = false;
  if (plugin)
  {

    auto indent = vtkIndent().GetNextIndent();
    msg << "----------------------------------------------------------" << endl
        << "Importing plugin: **" << plugin->GetPluginName() << "**" << endl
        << indent << "name: " << plugin->GetPluginName() << endl
        << indent << "version: " << plugin->GetPluginVersionString() << endl
        << indent << "filename: " << (plugin->GetFileName() ? plugin->GetFileName() : "(nullptr)")
        << endl
        << indent << "required-on-server: " << plugin->GetRequiredOnServer() << endl
        << indent << "required-on-client: " << plugin->GetRequiredOnClient() << endl
        << indent << "has-eula: " << (plugin->GetEULA() != nullptr) << endl;

    // If plugin has an EULA and an on load check function callback, confirm it before proceeding.
    if ((plugin->GetEULA() == nullptr || vtkPVPlugin::ConfirmEULA(plugin)) &&
      plugin->OnLoadCheckCallbackExecute())
    {
      // Register the plugin with the plugin manager on the current process. That
      // will kick in the code to process the plugin e.g. initialize CSInterpreter,
      // load XML etc.
      vtkPVPluginTracker::GetInstance()->RegisterPlugin(plugin);
      status = true;
    }
    else
    {
      msg << "  Plugin has EULA and was not accepted. Plugin won't be imported." << endl;
    }
  }

  vtkVLogIfF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), (!msg.str().empty()), "Import status: %s \n%s",
    (status ? "success" : "failure"), msg.str().c_str());
  return status;
}

//-----------------------------------------------------------------------------
vtkPVPlugin::vtkPVPlugin()
{
  this->FileName = nullptr;
  this->OnLoadCheckCallbackPtr = nullptr;
}

//-----------------------------------------------------------------------------
vtkPVPlugin::~vtkPVPlugin()
{
  delete[] this->FileName;
  this->FileName = nullptr;
}

//-----------------------------------------------------------------------------
void vtkPVPlugin::SetFileName(const char* filename)
{
  delete[] this->FileName;
  this->FileName = nullptr;
  this->FileName = vtksys::SystemTools::DuplicateString(filename);
}

//-----------------------------------------------------------------------------
void vtkPVPlugin::GetBinaryResources(std::vector<std::string>&) {}

//-----------------------------------------------------------------------------
void vtkPVPlugin::SetEULAConfirmationCallback(vtkPVPlugin::EULAConfirmationCallback ptr)
{
  vtkPVPlugin::EULAConfirmationCallbackPtr = ptr;
}

//-----------------------------------------------------------------------------
bool vtkPVPlugin::ConfirmEULA(vtkPVPlugin* plugin)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->GetPartitionId() == 0 && plugin->GetEULA() != nullptr)
  {
    if (vtkPVPlugin::EULAConfirmationCallbackPtr != nullptr)
    {
      return vtkPVPlugin::EULAConfirmationCallbackPtr(plugin);
    }

    std::ostringstream str;
    str << "-----------------------------------------------------" << endl
        << "  By loading the '" << plugin->GetPluginName()
        << "' plugin you have accepted the EULA shipped with it." << endl
        << "  If that is not acceptable, please restart the application without loading " << endl
        << "  the '" << plugin->GetPluginName() << "' plugin." << endl;
    str << "-----------------------------------------------------" << endl;
    vtkOutputWindowDisplayText(str.str().c_str());
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPVPlugin::SetOnLoadCheckCallbackFunction(OnLoadCheckCallback callback)
{
  this->OnLoadCheckCallbackPtr = callback;
}

//-----------------------------------------------------------------------------
vtkPVPlugin::OnLoadCheckCallback vtkPVPlugin::GetOnLoadCheckCallbackFunction() const
{
  return this->OnLoadCheckCallbackPtr;
}

//-----------------------------------------------------------------------------
bool vtkPVPlugin::OnLoadCheckCallbackExecute()
{
  // In case no callback function has been specified, we return true immediately.
  if (this->OnLoadCheckCallbackPtr == nullptr)
  {
    return true;
  }

  bool callbackResult = this->OnLoadCheckCallbackPtr();
  return callbackResult;
}
