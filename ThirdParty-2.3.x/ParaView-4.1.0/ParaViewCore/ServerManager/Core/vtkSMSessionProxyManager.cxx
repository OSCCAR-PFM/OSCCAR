/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSessionProxyManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSessionProxyManager.h"

#include "vtkCollection.h"
#include "vtkDebugLeaks.h"
#include "vtkEventForwarderCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // for PARAVIEW_VERSION_*
#include "vtkPVInstantiator.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkReservedRemoteObjectIds.h"
#include "vtkSmartPointer.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMDeserializerProtobuf.h"
#include "vtkSMDocumentation.h"
#include "vtkSMGlobalPropertiesLinkUndoElement.h"
#include "vtkSMPipelineState.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionClient.h"
#include "vtkSMStateLoader.h"
#include "vtkSMStateLocator.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMUndoStack.h"
#include "vtkStdString.h"
#include "vtkStringList.h"
#include "vtkVersion.h"

#include <map>
#include <set>
#include <vector>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>
#include <assert.h>

#include "vtkSMSessionProxyManagerInternals.h"

#if 0 // for debugging
class vtkSMProxyRegObserver : public vtkCommand
{
public:
  virtual void Execute(vtkObject*, unsigned long event, void* data)
    {
      vtkSMProxyManager::RegisteredProxyInformation* info =
        (vtkSMProxyManager::RegisteredProxyInformation*)data;
      cout << info->Proxy
           << " " << vtkCommand::GetStringFromEventId(event)
           << " " << info->GroupName
           << " " << info->ProxyName
           << endl;
    }
};
#endif


class vtkSMProxyManagerProxySet : public std::set<vtkSMProxy*> {};

//*****************************************************************************
class vtkSMProxyManagerObserver : public vtkCommand
{
public:
  static vtkSMProxyManagerObserver* New()
    { return new vtkSMProxyManagerObserver(); }

  void SetTarget(vtkSMSessionProxyManager* t)
    {
    this->Target = t;
    }

  virtual void Execute(vtkObject *obj, unsigned long event, void* data)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(obj, event, data);
      }
    }

protected:
  vtkSMProxyManagerObserver()
    {
    this->Target = 0;
    }
  vtkSMSessionProxyManager* Target;
};
//*****************************************************************************
class vtkSMProxyManagerForwarder : public vtkCommand
{
public:
  static vtkSMProxyManagerForwarder* New()
    { return new vtkSMProxyManagerForwarder(); }

  virtual void Execute(vtkObject*, unsigned long event, void* data)
    {
    if (vtkSMProxyManager::IsInitialized())
      {
      vtkSMProxyManager::GetProxyManager()->InvokeEvent(event, data);
      }
    }

protected:
  vtkSMProxyManagerForwarder()
    {
    }
};
//*****************************************************************************
//---------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMSessionProxyManager::New(vtkSMSession* session)
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkSMSessionProxyManager");
#endif
  return new vtkSMSessionProxyManager(session);
}

//---------------------------------------------------------------------------
vtkSMSessionProxyManager::vtkSMSessionProxyManager(vtkSMSession* session)
{
  this->Superclass::SetSession(session);

  this->StateUpdateNotification = true;
  this->UpdateInputProxies = 0;
  this->Internals = new vtkSMSessionProxyManagerInternals;
  this->Internals->ProxyManager = this;

  this->Observer = vtkSMProxyManagerObserver::New();
  this->Observer->SetTarget(this);
#if 0 // for debugging
  vtkSMProxyRegObserver* obs = new vtkSMProxyRegObserver;
  this->AddObserver(vtkCommand::RegisterEvent, obs);
  this->AddObserver(vtkCommand::UnRegisterEvent, obs);
#endif

  this->ProxyDefinitionManager = vtkSMProxyDefinitionManager::New();
  this->ProxyDefinitionManager->AddObserver(
    vtkCommand::RegisterEvent, this->Observer);
  this->ProxyDefinitionManager->AddObserver(
    vtkCommand::UnRegisterEvent, this->Observer);
  this->ProxyDefinitionManager->AddObserver(
    vtkSMProxyDefinitionManager::ProxyDefinitionsUpdated, this->Observer);
  this->ProxyDefinitionManager->AddObserver(
    vtkSMProxyDefinitionManager::CompoundProxyDefinitionsUpdated, this->Observer);
  this->ProxyDefinitionManager->SetSession(session);

  this->PipelineState = vtkSMPipelineState::New();
  this->PipelineState->SetSession(this->Session);

  // setup event forwarder so that it forwards all events fired by this class via
  // the global proxy manager.
  vtkNew<vtkSMProxyManagerForwarder> forwarder;
  this->AddObserver(vtkCommand::AnyEvent, forwarder.GetPointer());
}

//---------------------------------------------------------------------------
vtkSMSessionProxyManager::~vtkSMSessionProxyManager()
{
  // This is causing a PushState() when the object is being destroyed. This
  // causes errors since the ProxyManager is destroyed only when the session is
  // being deleted, thus the session cannot be valid at this point.
  //this->UnRegisterProxies();
  delete this->Internals;

  this->Observer->SetTarget(0);
  this->Observer->Delete();

  this->ProxyDefinitionManager->Delete();
  this->ProxyDefinitionManager = NULL;

  this->PipelineState->Delete();
  this->PipelineState = NULL;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMSessionProxyManager::GetReservedGlobalID()
{
  return vtkReservedRemoteObjectIds::RESERVED_PROXY_MANAGER_ID;
}

//----------------------------------------------------------------------------
void vtkSMSessionProxyManager::InstantiateGroupPrototypes(const char* groupName)
{
  if (!groupName)
    {
    return;
    }

  assert(this->ProxyDefinitionManager != 0);

  vtksys_ios::ostringstream newgroupname;
  newgroupname << groupName << "_prototypes" << ends;

  // Not a huge fan of this iterator API. Need to make it more consistent with
  // VTK.
  vtkPVProxyDefinitionIterator* iter =
      this->ProxyDefinitionManager->NewSingleGroupIterator(groupName);

  // Find the XML elements from which the proxies can be instantiated and
  // initialized
  for ( iter->InitTraversal(); !iter->IsDoneWithTraversal();
        iter->GoToNextItem())
    {
    const char* xml_name = iter->GetProxyName();
    if (this->GetProxy(newgroupname.str().c_str(), xml_name) == NULL)
      {
      vtkSMProxy* proxy = this->NewProxy(groupName, xml_name);
      if (proxy)
        {
        proxy->SetSession(NULL);
        proxy->SetLocation(0);
        proxy->SetPrototype(true);
        this->RegisterProxy(newgroupname.str().c_str(), xml_name, proxy);
        proxy->FastDelete();
        }
      }
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkSMSessionProxyManager::InstantiatePrototypes()
{
  assert(this->ProxyDefinitionManager != 0);
  vtkPVProxyDefinitionIterator* iter =
    this->ProxyDefinitionManager->NewIterator();

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextGroup())
    {
    this->InstantiateGroupPrototypes(iter->GetGroupName());
    }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::NewProxy(
  const char* groupName, const char* proxyName, const char* subProxyName)
{
  if (!groupName || !proxyName)
    {
    return 0;
    }
  // Find the XML element from which the proxy can be instantiated and
  // initialized
  vtkPVXMLElement* element = this->GetProxyElement( groupName, proxyName,
                                                    subProxyName);
  if (element)
    {
    return this->NewProxy(element, groupName, proxyName, subProxyName);
    }

  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::NewProxy(vtkPVXMLElement* pelement,
                                        const char* groupname,
                                        const char* proxyname,
                                        const char* subProxyName)
{
  vtkObject* object = 0;
  vtksys_ios::ostringstream cname;
  cname << "vtkSM" << pelement->GetName() << ends;
  object = vtkPVInstantiator::CreateInstance(cname.str().c_str());

  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(object);
  if (proxy)
    {
    // XMLName/XMLGroup should be set before ReadXMLAttributes so sub proxy
    // can be found based on their names when sent to the PM Side
    proxy->SetXMLGroup(groupname);
    proxy->SetXMLName(proxyname);
    proxy->SetXMLSubProxyName(subProxyName);
    proxy->SetSession(this->GetSession());
    proxy->ReadXMLAttributes(this, pelement);
    }
  else
    {
    vtkWarningMacro("Creation of new proxy " << cname.str() << " failed ("
                    << groupname << ", " << proxyname << ").");
    }
  return proxy;
}


//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSessionProxyManager::GetProxyDocumentation(
  const char* groupName, const char* proxyName)
{
  if (!groupName || !proxyName)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  return proxy? proxy->GetDocumentation() : NULL;
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSessionProxyManager::GetPropertyDocumentation(
  const char* groupName, const char* proxyName, const char* propertyName)
{
  if (!groupName || !proxyName || !propertyName)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  if (proxy)
    {
    vtkSMProperty* prop = proxy->GetProperty(propertyName);
    if (prop)
      {
      return prop->GetDocumentation();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSessionProxyManager::GetProxyElement(const char* groupName,
                                                    const char* proxyName,
                                                    const char* subProxyName)
{
  assert(this->ProxyDefinitionManager != 0);
  return this->ProxyDefinitionManager->GetCollapsedProxyDefinition( groupName,
                                                                    proxyName,
                                                                    subProxyName,
                                                                    true);
}

//---------------------------------------------------------------------------
unsigned int vtkSMSessionProxyManager::GetNumberOfProxies(const char* group)
{
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    size_t size = 0;
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); ++it2)
      {
      size += it2->second.size();
      }
    return static_cast<unsigned int>(size);
    }
  return 0;
}

//---------------------------------------------------------------------------
// No errors are raised if a proxy definition for the requested proxy is not
// found.
vtkSMProxy* vtkSMSessionProxyManager::GetPrototypeProxy(const char* groupname,
  const char* name)
{
  if (!this->ProxyDefinitionManager)
    {
    return NULL;
    }

  std::string protype_group = groupname;
  protype_group += "_prototypes";
  vtkSMProxy* proxy = this->GetProxy(protype_group.c_str(), name);
  if (proxy)
    {
    return proxy;
    }

  // silently ask for the definition. If not found return NULL.
  vtkPVXMLElement* xmlElement =
    this->ProxyDefinitionManager->GetCollapsedProxyDefinition(
      groupname, name, NULL, false);
  if (xmlElement == NULL)
    {
    // No definition was located for the requested proxy.
    // Cannot create the prototype.
    return 0;
    }

  proxy = this->NewProxy(groupname, name);
  if (!proxy)
    {
    return 0;
    }
  proxy->SetLocation(0);
  proxy->SetPrototype(true);
  // register the proxy as a prototype.
  this->RegisterProxy(protype_group.c_str(), name, proxy);
  proxy->Delete();
  return proxy;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RemovePrototype(
  const char* groupname, const char* proxyname)
{
  std::string prototype_group = groupname;
  prototype_group += "_prototypes";
  vtkSMProxy* proxy = this->GetProxy(prototype_group.c_str(), proxyname);
  if (proxy)
    {
    // prototype exists, so remove it.
    this->UnRegisterProxy(prototype_group.c_str(), proxyname, proxy);
    }
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::GetProxy(const char* group, const char* name)
{
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.find(name);
    if (it2 != it->second.end())
      {
      if (it2->second.begin() != it2->second.end())
        {
        return it2->second.front()->Proxy.GetPointer();
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::GetProxy(const char* name)
{
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.find(name);
    if (it2 != it->second.end())
      {
      if (it2->second.begin() != it2->second.end())
        {
        return it2->second.front()->Proxy.GetPointer();
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::GetProxies(const char* group,
  const char* name, vtkCollection* collection)
{
  collection->RemoveAllItems();
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if(it != this->Internals->RegisteredProxyMap.end())
    {
    if(name == NULL)
      {
      vtkSMProxyManagerProxyMapType::iterator it2 =
        it->second.begin();
      std::set<vtkTypeUInt32> ids;
      for (; it2 != it->second.end(); it2++)
        {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
        for (; it3 != it2->second.end(); ++it3)
          {
          if(ids.find(it3->GetPointer()->Proxy->GetGlobalID()) != ids.end())
            {
            ids.insert(it3->GetPointer()->Proxy->GetGlobalID());
            collection->AddItem(it3->GetPointer()->Proxy);
            }
          }
        }
      }
    else
      {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
      if (it2 != it->second.end())
        {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
        for (; it3 != it2->second.end(); ++it3)
          {
          collection->AddItem(it3->GetPointer()->Proxy);
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::GetProxyNames(const char* groupname,
                                      vtkSMProxy* proxy, vtkStringList* names)
{
  if (!names)
    {
    return;
    }
  names->RemoveAllItems();

  if (!groupname || !proxy)
    {
    return;
    }

  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      if (it2->second.Contains(proxy))
        {
        names->AddString(it2->first.c_str());
        }
      }
    }
}


//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetProxyName(const char* groupname,
                                            vtkSMProxy* proxy)
{
  if (!groupname || !proxy)
    {
    return 0;
    }

  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      if (it2->second.Contains(proxy))
        {
        return it2->first.c_str();
        }
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetProxyName(const char* groupname,
                                            unsigned int idx)
{
  if (!groupname)
    {
    return 0;
    }

  unsigned int counter=0;

  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      if (counter == idx)
        {
        return it2->first.c_str();
        }
      counter++;
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::IsProxyInGroup(vtkSMProxy* proxy,
                                              const char* groupname)
{
  if (!proxy || !groupname)
    {
    return 0;
    }
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      if (it2->second.Contains(proxy))
        {
        return it2->first.c_str();
        }
      }
    }
  return 0;
}

namespace
{
  struct vtkSMProxyManagerProxyInformation
    {
    std::string GroupName;
    std::string ProxyName;
    vtkSMProxy* Proxy;
    };
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxies()
{

  // Clear internal proxy containers
  std::vector<vtkSMProxyManagerProxyInformation> toUnRegister;
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  iter->SetModeToAll();
  iter->SetSessionProxyManager(this);
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxyManagerProxyInformation info;
    info.GroupName = iter->GetGroup();
    info.ProxyName = iter->GetKey();
    info.Proxy = iter->GetProxy();
    toUnRegister.push_back(info);
    }
  iter->Delete();

  std::vector<vtkSMProxyManagerProxyInformation>::iterator vIter =
    toUnRegister.begin();
  for (;vIter != toUnRegister.end(); ++vIter)
    {
    this->UnRegisterProxy(vIter->GroupName.c_str(), vIter->ProxyName.c_str(),
      vIter->Proxy);
    }

  this->Internals->ModifiedProxies.clear();
  this->Internals->RegisteredProxyTuple.clear();
  this->Internals->State.ClearExtension(PXMRegistrationState::registered_proxy);

  // Push state for undo/redo BUT only if it is not a clean up before deletion.
  if(this->PipelineState->GetSession())
    {
    this->TriggerStateUpdate();
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxy( const char* group, const char* name,
                                         vtkSMProxy* proxy)
{
  if (!group || !name)
    {
    return;
    }

  // Just in case a proxy ref is NOT held outside the ProxyManager iteself
  // Keep one during the full method call so the event could still have a valid
  // proxy object.
  vtkSmartPointer<vtkSMProxy> proxyHolder = proxy;
  std::string nameHolder(name);
  std::string groupHolder(group);

  // Do something only if the given tuple was found
  if(this->Internals->RemoveTuples(group, name, proxy))
    {
    vtkSMProxyManager::RegisteredProxyInformation info;
    info.Proxy = proxy;
    info.GroupName = groupHolder.c_str();
    info.ProxyName = nameHolder.c_str();
    info.Type = vtkSMProxyManager::RegisteredProxyInformation::PROXY;

    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
    this->UnMarkProxyAsModified(info.Proxy);

    // Push state for undo/redo
    this->TriggerStateUpdate();
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxy(const char* name)
{
  // Remove entries and keep them in a set
  std::set<vtkSMProxyManagerEntry> entriesToRemove;
  this->Internals->RemoveTuples(name, entriesToRemove);

  // Notify that some entries have been deleted
  std::set<vtkSMProxyManagerEntry>::iterator iter = entriesToRemove.begin();
  while(iter != entriesToRemove.end())
    {
    vtkSMProxyManager::RegisteredProxyInformation info;
    info.Proxy = iter->Proxy;
    info.GroupName = iter->Group.c_str();
    info.ProxyName = iter->Name.c_str();
    info.Type = vtkSMProxyManager::RegisteredProxyInformation::PROXY;

    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
    this->UnMarkProxyAsModified(info.Proxy);

    // Move forward
    iter++;
    }

  // Push new state only if changed occured
  if(entriesToRemove.size() > 0)
    {
    this->TriggerStateUpdate();
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxy(vtkSMProxy* proxy)
{
  // Find tuples
  std::set<vtkSMProxyManagerEntry> tuplesToRemove;
  this->Internals->FindProxyTuples(proxy, tuplesToRemove);

  // Remove tuples
  std::set<vtkSMProxyManagerEntry>::iterator iter = tuplesToRemove.begin();
  while(iter != tuplesToRemove.end())
    {
    this->UnRegisterProxy(iter->Group.c_str(), iter->Name.c_str(), iter->Proxy);
    iter++;
    }

  // Push new state only if changed occured
  if(tuplesToRemove.size() > 0)
    {
    this->TriggerStateUpdate();
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RegisterProxy(const char* groupname,
                                      const char* name,
                                      vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return;
    }

  vtkSMProxyManagerProxyListType &proxy_list =
    this->Internals->RegisteredProxyMap[groupname][name];
  if (proxy_list.Contains(proxy))
    {
    return;
    }

  // Add Tuple
  this->Internals->RegisteredProxyTuple.insert(
      vtkSMProxyManagerEntry( groupname, name, proxy ));

  vtkSMProxyManagerProxyInfo* proxyInfo = vtkSMProxyManagerProxyInfo::New();
  proxy_list.push_back(proxyInfo);
  proxyInfo->FastDelete();

  proxyInfo->Proxy = proxy;
  // Add observers to note proxy modification.
  proxyInfo->ModifiedObserverTag = proxy->AddObserver(
    vtkCommand::PropertyModifiedEvent, this->Observer);
  proxyInfo->StateChangedObserverTag = proxy->AddObserver(
    vtkCommand::StateChangedEvent, this->Observer);
  proxyInfo->UpdateObserverTag = proxy->AddObserver(vtkCommand::UpdateEvent,
    this->Observer);
  proxyInfo->UpdateInformationObserverTag = proxy->AddObserver(
    vtkCommand::UpdateInformationEvent, this->Observer);
  // Note, these observer will be removed in the destructor of proxyInfo.

  // Update state
  if(proxy->GetLocation() != 0 && !proxy->IsPrototype()) // Not a prototype !!!
    {
    proxy->CreateVTKObjects(); // Make sure an ID has been assigned to it

    // Do not put prototype groups in state
    vtksys::RegularExpression prototypesRe("_prototypes$");
    if (!prototypesRe.find(groupname))
      {
      PXMRegistrationState_Entry *entry =
          this->Internals->State.AddExtension(PXMRegistrationState::registered_proxy);
      entry->set_group(groupname);
      entry->set_name(name);
      entry->set_global_id(proxy->GetGlobalID());

      // Push state for undo/redo
      this->TriggerStateUpdate();
      }
    }

  // Fire event.
  vtkSMProxyManager::RegisteredProxyInformation info;
  info.Proxy = proxy;
  info.GroupName = groupname;
  info.ProxyName = name;
  info.Type = vtkSMProxyManager::RegisteredProxyInformation::PROXY;
  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UpdateRegisteredProxies(const char* groupname,
  int modified_only /*=1*/)
{
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
        {
        // Check is proxy is in the modified set.
        if (!modified_only ||
          this->Internals->ModifiedProxies.find(it3->GetPointer()->Proxy.GetPointer())
          != this->Internals->ModifiedProxies.end())
          {
          it3->GetPointer()->Proxy.GetPointer()->UpdateVTKObjects();
          it3->GetPointer()->Proxy.GetPointer()->UpdatePipelineInformation();
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UpdateRegisteredProxies(int modified_only /*=1*/)
{
  vtksys::RegularExpression prototypesRe("_prototypes$");

  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    if (prototypesRe.find(it->first))
      {
      // skip the prototypes.
      continue;
      }

    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      // Check is proxy is in the modified set.
      vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
        {
        // Check if proxy is in the modified set.
        if (!modified_only ||
          this->Internals->ModifiedProxies.find(it3->GetPointer()->Proxy.GetPointer())
          != this->Internals->ModifiedProxies.end())
          {
          it3->GetPointer()->Proxy.GetPointer()->UpdateVTKObjects();
          it3->GetPointer()->Proxy.GetPointer()->UpdatePipelineInformation();
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UpdateRegisteredProxiesInOrder(int modified_only/*=1*/)
{
  this->UpdateInputProxies = 1;
  this->UpdateRegisteredProxies(modified_only);
  this->UpdateInputProxies = 0;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UpdateProxyInOrder(vtkSMProxy* proxy)
{
  this->UpdateInputProxies = 1;
  proxy->UpdateVTKObjects();
  this->UpdateInputProxies = 0;
}

//---------------------------------------------------------------------------
int vtkSMSessionProxyManager::GetNumberOfLinks()
{
  return static_cast<int>(this->Internals->RegisteredLinkMap.size());
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetLinkName(int idx)
{
  int numlinks = this->GetNumberOfLinks();
  if(idx >= numlinks)
    {
    return NULL;
    }
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.begin();
  for(int i=0; i<idx; i++)
    {
    it++;
    }
  return it->first.c_str();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RegisterLink(const char* name, vtkSMLink* link)
{
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.find(name);
  if (it != this->Internals->RegisteredLinkMap.end())
    {
    vtkWarningMacro("Replacing previously registered link with name " << name);
    }
  this->Internals->RegisteredLinkMap[name] = link;

  // PXM state management
  link->SetSession(this->GetSession());
  link->PushStateToSession();
  this->Internals->UpdateLinkState();
  this->TriggerStateUpdate();

  vtkSMProxyManager::RegisteredProxyInformation info;
  info.Proxy = 0;
  info.GroupName = 0;
  info.ProxyName = name;
  info.Type = vtkSMProxyManager::RegisteredProxyInformation::LINK;
  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
}

//---------------------------------------------------------------------------
vtkSMLink* vtkSMSessionProxyManager::GetRegisteredLink(const char* name)
{
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.find(name);
  if (it != this->Internals->RegisteredLinkMap.end())
    {
    return it->second.GetPointer();
    }
  return NULL;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterLink(const char* name)
{
  std::string nameHolder = (name? name : "");
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.find(name);
  if (it != this->Internals->RegisteredLinkMap.end())
    {
    this->Internals->RegisteredLinkMap.erase(it);

    // PXM state management
    this->Internals->UpdateLinkState();
    this->TriggerStateUpdate();

    vtkSMProxyManager::RegisteredProxyInformation info;
    info.Proxy = 0;
    info.GroupName = 0;
    info.ProxyName = nameHolder.c_str();
    info.Type = vtkSMProxyManager::RegisteredProxyInformation::LINK;
    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterAllLinks()
{
  this->Internals->RegisteredLinkMap.clear();

  // PXM state management
  this->Internals->UpdateLinkState();
  this->TriggerStateUpdate();
}


//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::ExecuteEvent(vtkObject* obj, unsigned long event,
  void* data)
{
  // Check source object
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(obj);

  // Manage ProxyDefinitionManager Events
  if(obj == this->ProxyDefinitionManager)
    {
    vtkSIProxyDefinitionManager::RegisteredDefinitionInformation* defInfo;
    switch(event)
      {
      case vtkCommand::RegisterEvent:
      case vtkCommand::UnRegisterEvent:
         defInfo = reinterpret_cast<
           vtkSIProxyDefinitionManager::RegisteredDefinitionInformation*>(data);
         if(defInfo->CustomDefinition)
           {
           vtkSMProxyManager::RegisteredProxyInformation info;
           info.Proxy = 0;
           info.GroupName = defInfo->GroupName;
           info.ProxyName = defInfo->ProxyName;
           info.Type = vtkSMProxyManager::RegisteredProxyInformation::COMPOUND_PROXY_DEFINITION;
           this->InvokeEvent(event, &info);
           }
         // Both these events imply that the definition may have somehow
         // changed. If so, we need to ensure that any old prototypes we have
         // for this proxy type are removed, otherwise we may end up using
         // obsolete prototypes.
         this->RemovePrototype(defInfo->GroupName, defInfo->ProxyName);
         break;

      default:
         this->InvokeEvent(event, data);
         break;
      }
    }
  // Manage proxy modification call back to mark proxy dirty...
  else if (proxy)
    {
    switch (event)
      {
    case vtkCommand::PropertyModifiedEvent:
        {
        // Some property on the proxy has been modified.
        this->MarkProxyAsModified(proxy);
        vtkSMProxyManager::ModifiedPropertyInformation info;
        info.Proxy = proxy;
        info.PropertyName = reinterpret_cast<const char*>(data);
        if (info.PropertyName)
          {
          this->InvokeEvent(vtkCommand::PropertyModifiedEvent,
            &info);
          }
        }
      break;

    case vtkCommand::StateChangedEvent:
        {
        // I wonder if I need to mark the proxy modified. Cause when state
        // changes, the properties are pushed as well so ideally we should call
        // MarkProxyAsModified() and then UnMarkProxyAsModified() :).
        // this->MarkProxyAsModified(proxy);

        vtkSMProxyManager::StateChangedInformation info;
        info.Proxy = proxy;
        info.StateChangeElement = reinterpret_cast<vtkPVXMLElement*>(data);
        if (info.StateChangeElement)
          {
          this->InvokeEvent(vtkCommand::StateChangedEvent, &info);
          }
        }
      break;

    case vtkCommand::UpdateInformationEvent:
      this->InvokeEvent(vtkCommand::UpdateInformationEvent, proxy);
      break;

    case vtkCommand::UpdateEvent:
      // Proxy has been updated.
      this->UnMarkProxyAsModified(proxy);
      break;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::MarkProxyAsModified(vtkSMProxy* proxy)
{
  this->Internals->ModifiedProxies.insert(proxy);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnMarkProxyAsModified(vtkSMProxy* proxy)
{
  vtkSMSessionProxyManagerInternals::SetOfProxies::iterator it =
    this->Internals->ModifiedProxies.find(proxy);
  if (it != this->Internals->ModifiedProxies.end())
    {
    this->Internals->ModifiedProxies.erase(it);
    }
}
//---------------------------------------------------------------------------
int vtkSMSessionProxyManager::AreProxiesModified()
{
  return (this->Internals->ModifiedProxies.size() > 0)? 1: 0;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::LoadXMLState( const char* filename,
                                      vtkSMStateLoader* loader/*=NULL*/)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename);
  parser->Parse();

  this->LoadXMLState(parser->GetRootElement(), loader);
  parser->Delete();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::LoadXMLState( vtkPVXMLElement* rootElement,
                                             vtkSMStateLoader* loader/*=NULL*/,
                                             bool keepOriginalIds/*=false*/)
{
  if (!rootElement)
    {
    return;
    }
  vtkSmartPointer<vtkSMStateLoader> spLoader;

  if (!loader)
    {
    spLoader = vtkSmartPointer<vtkSMStateLoader>::New();
    spLoader->SetSessionProxyManager(this);
    }
  else
    {
    spLoader = loader;
    }
  if (spLoader->LoadState(rootElement, keepOriginalIds))
    {
    vtkSMProxyManager::LoadStateInformation info;
    info.RootElement = rootElement;
    info.ProxyLocator = spLoader->GetProxyLocator();
    this->InvokeEvent(vtkCommand::LoadStateEvent, &info);
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::SaveXMLState(const char* filename)
{
  vtkPVXMLElement* rootElement = this->SaveXMLState();
  ofstream os(filename, ios::out);
  rootElement->PrintXML(os, vtkIndent());
  rootElement->Delete();
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSessionProxyManager::SaveXMLState()
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("GenericParaViewApplication");
  this->AddInternalState(root);

  vtkSMProxyManager::LoadStateInformation info;
  info.RootElement = root;
  info.ProxyLocator = NULL;
  this->InvokeEvent(vtkCommand::SaveStateEvent, &info);
  return root;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::CollectReferredProxies(
  vtkSMProxyManagerProxySet& setOfProxies, vtkSMProxy* proxy)
{
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(proxy->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      iter->GetProperty());
    for (unsigned int cc=0; pp && (pp->GetNumberOfProxies() > cc); cc++)
      {
      vtkSMProxy* referredProxy = pp->GetProxy(cc);
      if (referredProxy)
        {
        setOfProxies.insert(referredProxy);
        this->CollectReferredProxies(setOfProxies, referredProxy);
        }
      }
    }
}


//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSessionProxyManager::AddInternalState(vtkPVXMLElement *parentElem)
{
  vtkPVXMLElement* rootElement = vtkPVXMLElement::New();
  rootElement->SetName("ServerManagerState");

  // Set version number on the state element.
  vtksys_ios::ostringstream version_string;
  version_string << vtkSMProxyManager::GetVersionMajor() << "."
                 << vtkSMProxyManager::GetVersionMinor() << "."
                 << vtkSMProxyManager::GetVersionPatch();
  rootElement->AddAttribute("version", version_string.str().c_str());


  std::set<vtkSMProxy*> visited_proxies; // set of proxies already added.

  // First save the state of all proxies
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();

    // Do not save the state of prototypes.
    const char* protstr = "_prototypes";
    const char* colname = it->first.c_str();
    int do_group = 1;
    if (strlen(colname) > strlen(protstr))
      {
      const char* newstr = colname + strlen(colname) -
        strlen(protstr);
      if (strcmp(newstr, protstr) == 0)
        {
        do_group = 0;
        }
      }
    else if ( colname[0] == '_' )
      {
      do_group = 0;
      }
    if (!do_group)
      {
      continue;
      }

    // save the states of all proxies in this group.
    for (; it2 != it->second.end(); it2++)
      {
      vtkSMProxyManagerProxyListType::iterator it3
        = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
        {
        if (visited_proxies.find(it3->GetPointer()->Proxy.GetPointer())
          != visited_proxies.end())
          {
          // proxy has been saved.
          continue;
          }
        it3->GetPointer()->Proxy.GetPointer()->SaveXMLState(rootElement);
        visited_proxies.insert(it3->GetPointer()->Proxy.GetPointer());
        }
      }
    }

  // Save the proxy collections. This is done seprately because
  // one proxy can be in more than one group.
  it = this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    // Do not save the state of prototypes.
    const char* protstr = "_prototypes";
    int do_group = 1;
    if (strlen(it->first.c_str()) > strlen(protstr))
      {
      const char* newstr = it->first.c_str() + strlen(it->first.c_str()) -
        strlen(protstr);
      if (strcmp(newstr, protstr) == 0)
        {
        do_group = 0;
        }
      }
    if (do_group)
      {
      vtkPVXMLElement* collectionElement = vtkPVXMLElement::New();
      collectionElement->SetName("ProxyCollection");
      collectionElement->AddAttribute("name", it->first.c_str());
      vtkSMProxyManagerProxyMapType::iterator it2 =
        it->second.begin();
      bool some_proxy_added = false;
      for (; it2 != it->second.end(); it2++)
        {
        vtkSMProxyManagerProxyListType::iterator it3 =
          it2->second.begin();
        for (; it3 != it2->second.end(); ++it3)
          {
          if (visited_proxies.find(it3->GetPointer()->Proxy.GetPointer()) != visited_proxies.end())
            {
            vtkPVXMLElement* itemElement = vtkPVXMLElement::New();
            itemElement->SetName("Item");
            itemElement->AddAttribute("id",
              it3->GetPointer()->Proxy->GetGlobalID());
            itemElement->AddAttribute("name", it2->first.c_str());
            collectionElement->AddNestedElement(itemElement);
            itemElement->Delete();
            some_proxy_added = true;
            }
          }
        }
      // Avoid addition of empty groups.
      if (some_proxy_added)
        {
        rootElement->AddNestedElement(collectionElement);
        }
      collectionElement->Delete();
      }
    }

  // TODO: Save definitions for only referred compound proxy definitions
  // when saving state for subset of proxies.
  vtkPVXMLElement* defs = vtkPVXMLElement::New();
  defs->SetName("CustomProxyDefinitions");
  this->SaveCustomProxyDefinitions(defs);
  rootElement->AddNestedElement(defs);
  defs->Delete();

  // Save links
  vtkPVXMLElement* links = vtkPVXMLElement::New();
  links->SetName("Links");
  this->SaveRegisteredLinks(links);
  rootElement->AddNestedElement(links);
  links->Delete();

  vtkPVXMLElement* globalProps = vtkPVXMLElement::New();
  globalProps->SetName("GlobalPropertiesManagers");
  this->SaveGlobalPropertiesManagers(globalProps);
  rootElement->AddNestedElement(globalProps);
  globalProps->Delete();

  if(parentElem)
    {
    parentElem->AddNestedElement(rootElement);
    rootElement->FastDelete();
    }

  return rootElement;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterCustomProxyDefinitions()
{
  assert(this->ProxyDefinitionManager != 0);
  this->ProxyDefinitionManager->ClearCustomProxyDefinitions();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterCustomProxyDefinition(
  const char* group, const char* name)
{
  assert(this->ProxyDefinitionManager != 0);
  this->ProxyDefinitionManager->RemoveCustomProxyDefinition(group, name);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RegisterCustomProxyDefinition(
  const char* group, const char* name, vtkPVXMLElement* top)
{
  assert(this->ProxyDefinitionManager != 0);
  this->ProxyDefinitionManager->AddCustomProxyDefinition(group, name, top);
}

//---------------------------------------------------------------------------
// Does not raise an error if definition is not found.
vtkPVXMLElement* vtkSMSessionProxyManager::GetProxyDefinition(
  const char* group, const char* name)
{
  if (!name || !group)
    {
    return 0;
    }


  assert(this->ProxyDefinitionManager != 0);
  return this->ProxyDefinitionManager->GetProxyDefinition(group, name, false);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::LoadCustomProxyDefinitions(vtkPVXMLElement* root)
{
  assert(this->ProxyDefinitionManager != 0);
  this->ProxyDefinitionManager->LoadCustomProxyDefinitions(root);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::LoadCustomProxyDefinitions(const char* filename)
{
  assert(this->ProxyDefinitionManager != 0);
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename);
  if (!parser->Parse())
    {
    vtkErrorMacro("Failed to parse file : " << filename);
    return;
    }
  this->ProxyDefinitionManager->LoadCustomProxyDefinitions(parser->GetRootElement());
  parser->Delete();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::SaveCustomProxyDefinitions(
  vtkPVXMLElement* rootElement)
{
  assert("Definition Manager should exist" && this->ProxyDefinitionManager != 0);
  this->ProxyDefinitionManager->SaveCustomProxyDefinitions(rootElement);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::SaveRegisteredLinks(vtkPVXMLElement* rootElement)
{
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.begin();
  for (; it != this->Internals->RegisteredLinkMap.end(); ++it)
    {
    it->second.GetPointer()->SaveXMLState(it->first.c_str(), rootElement);
    }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::SaveGlobalPropertiesManagers(vtkPVXMLElement* root)
{
  vtkSMProxyManager::GetProxyManager()->SaveGlobalPropertiesManagers(root);
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSessionProxyManager::GetProxyHints(
  const char* groupName, const char* proxyName)
{
  if (!groupName || !proxyName)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  return proxy? proxy->GetHints() : NULL;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSessionProxyManager::GetPropertyHints(
  const char* groupName, const char* proxyName, const char* propertyName)
{
  if (!groupName || !proxyName || !propertyName)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  if (proxy)
    {
    vtkSMProperty* prop = proxy->GetProperty(propertyName);
    if (prop)
      {
      return prop->GetHints();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::SetGlobalPropertiesManager(const char* name,
    vtkSMGlobalPropertiesManager* mgr)
{
  vtkSMProxyManager::GetProxyManager()->SetGlobalPropertiesManager(name, mgr);
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetGlobalPropertiesManagerName(
  vtkSMGlobalPropertiesManager* mgr)
{
  return vtkSMProxyManager::GetProxyManager()->GetGlobalPropertiesManagerName(mgr);
}

//---------------------------------------------------------------------------
vtkSMGlobalPropertiesManager* vtkSMSessionProxyManager::GetGlobalPropertiesManager(
  const char* name)
{
  return vtkSMProxyManager::GetProxyManager()->GetGlobalPropertiesManager(name);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RemoveGlobalPropertiesManager(const char* name)
{
  vtkSMProxyManager::GetProxyManager()->RemoveGlobalPropertiesManager(name);
}

//---------------------------------------------------------------------------
unsigned int vtkSMSessionProxyManager::GetNumberOfGlobalPropertiesManagers()
{
  return vtkSMProxyManager::GetProxyManager()->GetNumberOfGlobalPropertiesManagers();
}

//---------------------------------------------------------------------------
vtkSMGlobalPropertiesManager* vtkSMSessionProxyManager::GetGlobalPropertiesManager(
  unsigned int index)
{
  return vtkSMProxyManager::GetProxyManager()->GetGlobalPropertiesManager(index);
}

//---------------------------------------------------------------------------
bool vtkSMSessionProxyManager::LoadConfigurationXML(const char* xml)
{
  assert(this->ProxyDefinitionManager != 0);
  return this->ProxyDefinitionManager->LoadConfigurationXMLFromString(xml);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent <<  "UpdateInputProxies: " <<  this->UpdateInputProxies << endl;
}
//---------------------------------------------------------------------------
const vtkSMMessage* vtkSMSessionProxyManager::GetFullState()
{
  if(!this->Internals->State.has_global_id())
    {
    this->Internals->State.set_global_id(vtkSMSessionProxyManager::GetReservedGlobalID());
    this->Internals->State.set_location(vtkProcessModule::PROCESS_DATA_SERVER);
    this->Internals->State.SetExtension(DefinitionHeader::client_class, "");
    this->Internals->State.SetExtension(DefinitionHeader::server_class, "vtkSIObject");
    this->Internals->State.SetExtension(ProxyState::xml_group, "");
    this->Internals->State.SetExtension(ProxyState::xml_name, "");
    }

  return &this->Internals->State;
}
//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator)
{
  // Disable state push
  bool previous = this->StateUpdateNotification;
  this->StateUpdateNotification = false;

  // Need to compute differences and just call Register/UnRegister for those items
  std::set<vtkSMProxyManagerEntry> tuplesToUnregister;
  std::set<vtkSMProxyManagerEntry> tuplesToRegister;
  std::set<vtkSMProxyManagerEntry>::iterator iter;

  // Fill delta sets
  this->Internals->ComputeDelta(msg, locator, tuplesToRegister, tuplesToUnregister);

  // Register new ones
  iter = tuplesToRegister.begin();
  while( iter != tuplesToRegister.end() )
    {
    this->RegisterProxy(iter->Group.c_str(), iter->Name.c_str(), iter->Proxy);
    iter++;
    }

  // Unregister old ones
  iter = tuplesToUnregister.begin();
  while( iter != tuplesToUnregister.end() )
    {
    this->UnRegisterProxy(iter->Group.c_str(), iter->Name.c_str(), iter->Proxy);
    iter++;
    }

  // Manage ProxySelectionModel state
  for(int i = 0,
      size = msg->ExtensionSize(PXMRegistrationState::registered_selection_model);
      i < size && this->Session;
      i++)
    {
    vtkTypeUInt32 remoteObjectId =
        msg->GetExtension(PXMRegistrationState::registered_selection_model, i).global_id();
    const char* name =
        msg->GetExtension(PXMRegistrationState::registered_selection_model, i).name().c_str();

    vtkSMProxySelectionModel* model = this->GetSelectionModel(name);

    // Make sure the expected selection model exist by creating a new empty one
    // that will be filled by the LoadState
    if(model == NULL)
      {
      model = vtkSMProxySelectionModel::New();
      this->RegisterSelectionModel(name, model);
      model->FastDelete();
      }

    if(!model->HasGlobalID())
      {
      vtkSMMessage msgTmp;
      msgTmp.set_global_id(remoteObjectId);
      msgTmp.set_location(vtkPVSession::DATA_SERVER_ROOT);
      this->Session->PullState(&msgTmp);

      model->LoadState(&msgTmp, locator);
      }
    }

  // Manage Link state
  std::set<std::string> linkNameToKeep;
  for(int i = 0,
      size = msg->ExtensionSize(PXMRegistrationState::registered_link);
      i < size && this->Session;
      i++)
    {
    vtkTypeUInt32 remoteObjectId =
        msg->GetExtension(PXMRegistrationState::registered_link, i).global_id();
    const char* name =
        msg->GetExtension(PXMRegistrationState::registered_link, i).name().c_str();
    linkNameToKeep.insert(name);

    vtkSMLink* link = this->GetRegisteredLink(name);
    if(link)
      {
      // Do nothing as we already know about it
      }
    else if ((link =
              vtkSMLink::SafeDownCast(
                  this->Session->GetRemoteObject(remoteObjectId))) != NULL)
      {
      // The link exist but is not registered
      this->RegisterLink(name, link);
      }
    else
      {
      // We need to create that link, load its state and register it
      vtkSMMessage msgTmp;
      msgTmp.set_global_id(remoteObjectId);
      msgTmp.set_location(vtkPVSession::DATA_SERVER_ROOT);
      this->Session->PullState(&msgTmp);

      // Create the concreate class
      vtkObject* object;
      const char* className = msgTmp.GetExtension(DefinitionHeader::client_class).c_str();
      object = vtkPVInstantiator::CreateInstance(className);
      if (!object)
        {
        vtkErrorMacro("Did not create Link concreate class of " << className);
        abort();
        }
      link = vtkSMLink::SafeDownCast(object);
      if(link)
        {
        link->LoadState(&msgTmp, locator);
        this->RegisterLink(name, link);
        }
      object->Delete();
      }
    }
  // Remove Link that have disapeared...
  for(int i = 0; i < this->GetNumberOfLinks(); i++)
    {
    const char* currentLinkName = this->GetLinkName(i);
    if(linkNameToKeep.find(currentLinkName) == linkNameToKeep.end())
      {
      this->UnRegisterLink(currentLinkName);
      i--;
      }
    }

  // Update state
  this->Internals->UpdateProxySelectionModelState();
  this->Internals->UpdateLinkState();

  // Share it
  this->StateUpdateNotification = previous;
  this->TriggerStateUpdate();
}

//---------------------------------------------------------------------------
bool vtkSMSessionProxyManager::HasDefinition( const char* groupName,
                                       const char* proxyName )
{
  return this->ProxyDefinitionManager &&
      this->ProxyDefinitionManager->HasDefinition(groupName, proxyName);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RegisterSelectionModel(
  const char* name, vtkSMProxySelectionModel* model)
{
  if (!model)
    {
    vtkErrorMacro("Cannot register a null model.");
    return;
    }
  if (!name)
    {
    vtkErrorMacro("Cannot register model with no name.");
    return;
    }

  vtkSMProxySelectionModel* curmodel = this->GetSelectionModel(name);
  if (curmodel && curmodel == model)
    {
    // already registered.
    return;
    }

  if (curmodel)
    {
    vtkWarningMacro("Replacing existing selection model: " << name);
    }
  model->SetSession(this->GetSession());
  this->Internals->SelectionModels[name] = model;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterSelectionModel( const char* name)
{
  this->Internals->SelectionModels.erase(name);
}

//---------------------------------------------------------------------------
vtkSMProxySelectionModel* vtkSMSessionProxyManager::GetSelectionModel(
  const char* name)
{
  vtkSMSessionProxyManagerInternals::SelectionModelsType::iterator iter =
    this->Internals->SelectionModels.find(name);
  if (iter == this->Internals->SelectionModels.end())
    {
    return 0;
    }

  return iter->second;
}
//---------------------------------------------------------------------------
vtkIdType vtkSMSessionProxyManager::GetNumberOfSelectionModel()
{
  return static_cast<vtkIdType>(this->Internals->SelectionModels.size());
}

//---------------------------------------------------------------------------
vtkSMProxySelectionModel* vtkSMSessionProxyManager::GetSelectionModelAt(int idx)
{
  vtkSMSessionProxyManagerInternals::SelectionModelsType::iterator iter =
      this->Internals->SelectionModels.begin();
  for(int i=0;i<idx;i++)
    {
    if(iter == this->Internals->SelectionModels.end())
      {
      // Out of range
      return NULL;
      }
    iter++;
    }

  return iter->second;
}

//----------------------------------------------------------------------------
void vtkSMSessionProxyManager::UpdateFromRemote()
{
    if (this->Session)
    {
        // For collaboration purpose at connection time we synchronize our
        // ProxyManager state with the server
        if(this->Session->IsMultiClients())
        {
            vtkSMMessage msg;
            msg.set_global_id(vtkSMSessionProxyManager::GetReservedGlobalID());
            msg.set_location(vtkPVSession::DATA_SERVER_ROOT);
            this->Session->PullState(&msg);
            if(msg.ExtensionSize(PXMRegistrationState::registered_proxy) > 0)
            {
                // We take the parent locator to always refer to the server while loading
                // the state. This prevent us from getting a creation state instead of
                // full update state when we split the creation/update action in 2
                // separate call.
                // Moreover, we don't want any existing states to be pushed again to
                // the server.
                bool previousValue = this->Session->StartProcessingRemoteNotification();
                
                // Setup server only state/proxy Locator
                vtkNew<vtkSMDeserializerProtobuf> deserializer;
                deserializer->SetStateLocator(this->Session->GetStateLocator()->GetParentLocator());
                deserializer->SetSessionProxyManager(this);
                
                vtkNew<vtkSMProxyLocator> serverLocator;
                serverLocator->SetDeserializer(deserializer.GetPointer());
                serverLocator->UseSessionToLocateProxy(true);
                serverLocator->SetSession(this->Session);
                
                // Load and update
                vtkSMProxyProperty::EnableProxyCreation();
                this->LoadState( &msg, serverLocator.GetPointer());
                this->UpdateRegisteredProxies(0);
                vtkSMProxyProperty::DisableProxyCreation();
                
                this->Session->StopProcessingRemoteNotification(previousValue);
            }
        }
    }
}
//---------------------------------------------------------------------------
bool vtkSMSessionProxyManager::IsStateUpdateNotificationEnabled()
{
    return this->StateUpdateNotification;
}
//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::DisableStateUpdateNotification()
{
    this->StateUpdateNotification = false;
}
//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::EnableStateUpdateNotification()
{
    this->StateUpdateNotification = true;
}
//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::TriggerStateUpdate()
{
    if(this->PipelineState && this->StateUpdateNotification)
    {
        this->Internals->UpdateProxySelectionModelState();
        this->PipelineState->ValidateState();
    }
}
