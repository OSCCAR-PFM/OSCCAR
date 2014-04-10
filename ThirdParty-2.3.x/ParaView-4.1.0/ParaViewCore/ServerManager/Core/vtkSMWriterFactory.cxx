/*=========================================================================

  Program:   ParaView
  Module:    vtkSMWriterFactory.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMWriterFactory.h"

#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMWriterProxy.h"
#include "vtkStringList.h"

#include <list>
#include <set>
#include <string>
#include <vector>
#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>
#include <assert.h>

class vtkSMWriterFactory::vtkInternals
{
public:
  struct vtkValue
    {
    std::string Group;
    std::string Name;
    std::set<std::string> Extensions;
    std::string Description;

    void FillInformation(vtkSMSession* session)
      {
      vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
      vtkSMProxy* prototype = pxm->GetPrototypeProxy( this->Group.c_str(),
                                                      this->Name.c_str());
      if (!prototype || !prototype->GetHints())
        {
        return;
        }
      vtkPVXMLElement* rfHint =
        prototype->GetHints()->FindNestedElementByName("WriterFactory");
      if (!rfHint)
        {
        return;
        }

      this->Extensions.clear();
      const char* exts = rfHint->GetAttribute("extensions");
      if (exts)
        {
        std::vector<std::string> exts_v;
        vtksys::SystemTools::Split(exts, exts_v,' ');
        this->Extensions.insert(exts_v.begin(), exts_v.end());
        }
      this->Description = rfHint->GetAttribute("file_description");
      }

    // Returns true is a prototype proxy can be created on the given connection.
    // For now, the connection is totally ignored since ServerManager doesn't
    // support that.
    bool CanCreatePrototype(vtkSMSourceProxy* source)
      {
      vtkSMSessionProxyManager* pxm = source->GetSession()->GetSessionProxyManager();
      return (pxm->GetPrototypeProxy(
        this->Group.c_str(), this->Name.c_str()) != NULL);
      }

    // Returns true if the data from the given output port can be written.
    bool CanWrite(vtkSMSourceProxy* source, unsigned int port)
      {
      vtkSMSessionProxyManager* pxm = source->GetSession()->GetSessionProxyManager();
      vtkSMProxy* prototype = pxm->GetPrototypeProxy(
        this->Group.c_str(), this->Name.c_str());
      if (!prototype || !source)
        {
        return false;
        }
      vtkSMWriterProxy* writer = vtkSMWriterProxy::SafeDownCast(prototype);
      // If it's not a vtkSMWriterProxy, then we assume that it can
      // always work in parallel.
      if (writer)
        {
        if (source->GetSession()->GetNumberOfProcesses(source->GetLocation()) > 1)
          {
          if (!writer->GetSupportsParallel())
            {
            return false;
            }
          }
        else
          {
          if (writer->GetParallelOnly())
            {
            return false;
            }
          }
        }
      vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
        prototype->GetProperty("Input"));
      if (!pp)
        {
        vtkGenericWarningMacro(<< prototype->GetXMLGroup()
          << " : " << prototype->GetXMLName()
          << " has no input property.");
        return false;
        }
      pp->RemoveAllUncheckedProxies();
      pp->AddUncheckedInputConnection(source, port);
      bool status = pp->IsInDomains() != 0;
      pp->RemoveAllUncheckedProxies();
      return status;
      }

    // Returns true if a file with the given extension can be written by this
    // writer. \c extension should not include the starting ".".
    bool ExtensionTest(const char* extension)
      {
      if (!extension || extension[0] == 0)
        {
        return false;
        }
      return (this->Extensions.find(extension) != this->Extensions.end());
      }
    };

  // we use a map here instead of a set because I'm avoiding const
  // correctness of the methods of vtkValue. The key is a
  // combination of the prototype name and group.
  typedef std::map<std::string, vtkValue> PrototypesType;
  PrototypesType Prototypes;
  std::string SupportedFileTypes;
  // The set of groups that are searched for writers. By default "writers" is
  // included.
  std::set<std::string> Groups;
};

vtkStandardNewMacro(vtkSMWriterFactory);
//----------------------------------------------------------------------------
vtkSMWriterFactory::vtkSMWriterFactory()
{
  this->Internals = new vtkInternals();
  this->Internals->Groups.insert("writers");
}

//----------------------------------------------------------------------------
vtkSMWriterFactory::~vtkSMWriterFactory()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::Initialize()
{
  this->Internals->Prototypes.clear();
  this->Internals->Groups.clear();
  this->Internals->Groups.insert("writers");
}

//----------------------------------------------------------------------------
unsigned int vtkSMWriterFactory::GetNumberOfRegisteredPrototypes()
{
  return static_cast<unsigned int>(this->Internals->Prototypes.size());
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::AddGroup(const char* groupName)
{
  if(groupName)
    {
    this->Internals->Groups.insert(groupName);
    }
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::RemoveGroup(const char* groupName)
{
  if(groupName)
    {
    this->Internals->Groups.erase(groupName);
    }
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::GetGroups(vtkStringList* groups)
{
  if(groups)
    {
    groups->RemoveAllItems();
    for(std::set<std::string>::iterator group=this->Internals->Groups.begin();
        group!=this->Internals->Groups.end();group++)
      {
      groups->AddString(group->c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::RegisterPrototype(const char* xmlgroup, const char* xmlname)
{
  vtkInternals::vtkValue value;
  value.Group = xmlgroup;
  value.Name = xmlname;
  std::string key = value.Name+value.Group;

  this->Internals->Prototypes[key] = value;
}

void vtkSMWriterFactory::UpdateAvailableWriters()
{
  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  // when we change the server we may not have a session yet. that's ok
  // since we'll come back here after the proxy definitions are loaded
  // from that session.
  if(vtkSMSession* session = proxyManager->GetActiveSession())
    {
    vtkSMSessionProxyManager* sessionProxyManager = session->GetSessionProxyManager();
    vtkSMProxyDefinitionManager* pdm = sessionProxyManager->GetProxyDefinitionManager();

    for(std::set<std::string>::iterator group=this->Internals->Groups.begin();
        group!=this->Internals->Groups.end();group++)
      {
      vtkPVProxyDefinitionIterator* iter =
        pdm->NewSingleGroupIterator(group->c_str());
      for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
        vtkPVXMLElement* hints = sessionProxyManager->GetProxyHints(
          iter->GetGroupName(), iter->GetProxyName());
        if (hints && hints->FindNestedElementByName("WriterFactory"))
          {
          this->RegisterPrototype(iter->GetGroupName(), iter->GetProxyName());
          }
        }
      iter->Delete();
      }
    }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMWriterFactory::CreateWriter(
  const char* filename, vtkSMSourceProxy* source, unsigned int outputport)
{
  if (!filename || filename[0] == 0)
    {
    vtkErrorMacro("No filename. Cannot create any writer.");
    return NULL;
    }

  std::string extension =
    vtksys::SystemTools::GetFilenameExtension(filename);
  if (extension.size() > 0)
    {
    // Find characters after last "."
    std::string::size_type found = extension.find_last_of(".");
    if (found != std::string::npos)
      {
      extension = extension.substr(found+1);
      }
    else
      {
      vtkErrorMacro("No extension. Cannot determine writer to create.");
      return NULL;
      }
    
    }
  else
    {
    vtkErrorMacro("No extension. Cannot determine writer to create.");
    return NULL;
    }

  // Get ProxyManager
  vtkSMSessionProxyManager* pxm = source->GetSession()->GetSessionProxyManager();

  // Make sure the source is in an expected state (BUG #13172)
  source->UpdatePipeline();

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    iter->second.FillInformation(source->GetSession());
    if (iter->second.CanCreatePrototype(source) &&
        iter->second.ExtensionTest(extension.c_str()) &&
        iter->second.CanWrite(source, outputport))
      {
      vtkSMProxy* proxy = pxm->NewProxy(
        iter->second.Group.c_str(),
        iter->second.Name.c_str());
      vtkSMPropertyHelper(proxy, "FileName").Set(filename);
      vtkSMPropertyHelper(proxy, "Input").Set(source, outputport);
      return proxy;
      }
    }

  vtkErrorMacro("No matching writer found for extension: " << extension );
  return NULL;
}

//----------------------------------------------------------------------------
static std::string vtkJoin(
  const std::set<std::string> exts, const char* prefix,
  const char* suffix)
{
  vtksys_ios::ostringstream stream;
  std::set<std::string>::const_iterator iter;
  for (iter = exts.begin(); iter != exts.end(); ++iter)
    {
    stream << prefix << *iter << suffix;
    }
  return stream.str();
}

//----------------------------------------------------------------------------
const char* vtkSMWriterFactory::GetSupportedFileTypes(
  vtkSMSourceProxy* source, unsigned int outputport)
{
  std::set<std::string> sorted_types;

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->second.CanCreatePrototype(source) &&
      iter->second.CanWrite(source, outputport))
      {
      iter->second.FillInformation(source->GetSession());
      if (iter->second.Extensions.size() > 0)
        {
        std::string ext_join = ::vtkJoin(iter->second.Extensions, "*.", " ");
        vtksys_ios::ostringstream stream;
        stream << iter->second.Description << "(" << ext_join << ")";
        sorted_types.insert(stream.str());
        }
      }
    }
  
  vtksys_ios::ostringstream all_types;
  std::set<std::string>::iterator iter2;
  for (iter2 = sorted_types.begin(); iter2 != sorted_types.end(); ++iter2)
    {
    if (iter2 != sorted_types.begin())
      {
      all_types << ";;";
      }
    all_types << (*iter2);
    }
  this->Internals->SupportedFileTypes = all_types.str();
  return this->Internals->SupportedFileTypes.c_str();
}

//----------------------------------------------------------------------------
bool vtkSMWriterFactory::CanWrite(vtkSMSourceProxy* source, unsigned int outputport)
{
  if (!source)
    {
    return false;
    }

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->second.CanCreatePrototype(source) &&
      iter->second.CanWrite(source, outputport))
      {
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
