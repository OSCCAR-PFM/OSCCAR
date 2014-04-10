/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPluginManager.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPluginsInformation.h"
#include "vtkPVPluginTracker.h"
#include "vtkSmartPointer.h"
#include "vtkSMPluginLoaderProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include <map>

class vtkSMPluginManager::vtkInternals
{
public:
  typedef std::map<vtkSMSession*,
          vtkSmartPointer<vtkPVPluginsInformation> > RemoteInfoMapType;
  RemoteInfoMapType RemoteInformations;
};

namespace
{
  class vtkFlagStateUpdated
    {
    bool Prev;
    bool &Flag;

  public:
    vtkFlagStateUpdated(bool &flag, bool new_val=true) : Prev(flag), Flag(flag)
      {
      this->Flag = new_val;
      }
    ~vtkFlagStateUpdated()
      {
      this->Flag = this->Prev;
      }

  private:
    vtkFlagStateUpdated(const vtkFlagStateUpdated&); // Not implemented
    void operator=(const vtkFlagStateUpdated&); // Not implemented
  };
}

vtkStandardNewMacro(vtkSMPluginManager);
//----------------------------------------------------------------------------
vtkSMPluginManager::vtkSMPluginManager()
{
  this->Internals = new vtkInternals();

  this->InLoadPlugin = false;

  // Setup and update local plugins information.
  this->LocalInformation = vtkPVPluginsInformation::New();
  this->LocalInformation->CopyFromObject(NULL);

  // When a plugin is register with the local tracker (either at buildtime or
  // at runtime, RegisterEvent is fired. We ensure that the PluginLoadedEvent
  // get fired, especially for plugins brought in a buildtime.
  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  tracker->AddObserver(vtkCommand::RegisterEvent,
    this, &vtkSMPluginManager::OnPluginRegistered);
}

//----------------------------------------------------------------------------
vtkSMPluginManager::~vtkSMPluginManager()
{
  delete this->Internals;
  this->Internals = NULL;

  this->LocalInformation->Delete();
  this->LocalInformation = NULL;
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::RegisterSession(vtkSMSession* session)
{
  assert(session != NULL);

  if (this->Internals->RemoteInformations.find(session) !=
    this->Internals->RemoteInformations.end())
    {
    vtkWarningMacro("Session already registered!!!");
    }
  else
    {
    vtkPVPluginsInformation* remoteInfo = vtkPVPluginsInformation::New();
    this->Internals->RemoteInformations[session].TakeReference(remoteInfo);
    session->GatherInformation(vtkPVSession::DATA_SERVER_ROOT, remoteInfo, 0);
    }

  // Also update the local information. This is generally unnecessary, but for
  // statically linked in plugins, this provides a cleaner opportunity to bring
  // in what's linked in.

  // we use Update so that any auto-load state changes done in
  // this->LocalInformation are preserved.
  vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
  temp->CopyFromObject(NULL);
  this->LocalInformation->Update(temp);
  temp->Delete();
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::UnRegisterSession(vtkSMSession* session)
{
  this->Internals->RemoteInformations.erase(session);
}

//----------------------------------------------------------------------------
vtkPVPluginsInformation* vtkSMPluginManager::GetRemoteInformation(
  vtkSMSession* session)
{
  return (session)? this->Internals->RemoteInformations[session] : NULL;
}

//----------------------------------------------------------------------------
bool vtkSMPluginManager::LoadLocalPlugin(const char* filename)
{
  vtkFlagStateUpdated stateUpdater(this->InLoadPlugin);

  vtkPVPluginLoader* loader = vtkPVPluginLoader::New();
  bool ret_val = loader->LoadPlugin(filename);
  loader->Delete();
  if (ret_val)
    {
    // Update local-plugin information.
    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    temp->CopyFromObject(NULL);
    // we use Update so that any auto-load state changes done in
    // this->LocalInformation are preserved.
    this->LocalInformation->Update(temp);
    temp->Delete();

    this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
    this->InvokeEvent(vtkSMPluginManager::LocalPluginLoadedEvent,(void*) filename);
    }

  // We don't report the error here if ret_val == false since vtkPVPluginLoader
  // already reports those errors using vtkErrorMacro.
  return ret_val;
}

//----------------------------------------------------------------------------
bool vtkSMPluginManager::LoadRemotePlugin(const char* filename,
  vtkSMSession* session)
{
  assert("Session cannot be NULL" && session != NULL);

  vtkFlagStateUpdated stateUpdater(this->InLoadPlugin);

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSMPluginLoaderProxy* proxy =
      vtkSMPluginLoaderProxy::SafeDownCast(pxm->NewProxy("misc", "PluginLoader"));
  proxy->UpdateVTKObjects();
  bool status = proxy->LoadPlugin(filename);
  if (!status)
    {
    vtkErrorMacro("Plugin load failed: " <<
      vtkSMPropertyHelper(proxy, "ErrorString").GetAsString());
    }
  proxy->Delete();

  // Refresh definitions since those may have changed.
  pxm->GetProxyDefinitionManager()->SynchronizeDefinitions();

  if (status)
    {
    // Refresh the remote plugin information
    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    session->GatherInformation(
      vtkPVSession::DATA_SERVER_ROOT, temp, 0);
    this->Internals->RemoteInformations[session]->Update(temp);
    temp->Delete();
    this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
    this->InvokeEvent(vtkSMPluginManager::RemotePluginLoadedEvent,(void*) filename);
    }
  return status;
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::LoadPluginConfigurationXMLFromString(
  const char* xmlcontents, vtkSMSession* session, bool remote)
{
  vtkFlagStateUpdated stateUpdater(this->InLoadPlugin);
  if (remote)
    {
    assert("Session should already be set" && (session != NULL));
    vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
    vtkSMPluginLoaderProxy* proxy =
        vtkSMPluginLoaderProxy::SafeDownCast(pxm->NewProxy("misc", "PluginLoader"));
    proxy->UpdateVTKObjects();
    proxy->LoadPluginConfigurationXMLFromString(xmlcontents);
    proxy->Delete();

    // Refresh definitions since those may have changed.
    pxm->GetProxyDefinitionManager()->SynchronizeDefinitions();

    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    session->GatherInformation(vtkPVSession::DATA_SERVER_ROOT, temp, 0);
    this->Internals->RemoteInformations[session]->Update(temp);
    temp->Delete();
    }
  else
    {
    vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLFromString(
      xmlcontents);

    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    temp->CopyFromObject(NULL);
    this->LocalInformation->Update(temp);
    temp->Delete();
    }

  this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::OnPluginRegistered()
{
  if (this->InLoadPlugin)
    {
    return;
    }
  // Update local-plugin information.
  vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
  temp->CopyFromObject(NULL);
  // we use Update so that any auto-load state changes done in
  // this->LocalInformation are preserved.
  this->LocalInformation->Update(temp);
  temp->Delete();

  this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
}

//----------------------------------------------------------------------------
const char* vtkSMPluginManager::GetLocalPluginSearchPaths()
{
  return this->LocalInformation->GetSearchPaths();
}

//----------------------------------------------------------------------------
const char* vtkSMPluginManager::GetRemotePluginSearchPaths(vtkSMSession* session)
{
  return this->Internals->RemoteInformations[session]->GetSearchPaths();
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
