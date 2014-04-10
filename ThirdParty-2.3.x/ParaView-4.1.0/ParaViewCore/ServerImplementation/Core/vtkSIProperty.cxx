/*=========================================================================

  Program:   ParaView
  Module:    vtkSIProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIProperty.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSIProxy.h"
#include "vtkSMMessage.h"
#include "vtkPVXMLElement.h"

//****************************************************************************/
//                    Internal Classes and typedefs
//****************************************************************************/
class vtkSIProperty::vtkInternals
{
public:
  vtkInternals()
    {
    this->CacheValue = NULL;
    }
  ~vtkInternals()
    {
    this->ClearCache();
    }

  void ClearCache()
    {
    if(this->CacheValue)
      {
      delete this->CacheValue;
      this->CacheValue = NULL;
      }
    }

  bool HasCache()
    {
    return this->CacheValue != NULL;
    }

  void SaveToCache(const ProxyState_Property *newValue)
    {
    this->ClearCache();
    this->CacheValue = new ProxyState_Property();
    this->CacheValue->CopyFrom(*newValue);
    }

  ProxyState_Property *CacheValue;
};

//****************************************************************************/
vtkStandardNewMacro(vtkSIProperty);
//----------------------------------------------------------------------------
vtkSIProperty::vtkSIProperty()
{
  this->Command = NULL;
  this->XMLName = NULL;
  this->IsInternal = false;
  this->InformationOnly = false;
  this->Repeatable = false;
  this->SIProxyObject = NULL;
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkSIProperty::~vtkSIProperty()
{
  this->SetCommand(NULL);
  this->SetXMLName(NULL);
  delete this->Internals;
}

//----------------------------------------------------------------------------
bool vtkSIProperty::ReadXMLAttributes(
  vtkSIProxy* proxyhelper, vtkPVXMLElement* element)
{
  this->SIProxyObject = proxyhelper;

  const char* xmlname = element->GetAttribute("name");
  if(xmlname)
    {
    this->SetXMLName(xmlname);
    }

  const char* command = element->GetAttribute("command");
  if (command)
    {
    this->SetCommand(command);
    }

  int repeatable;
  if (element->GetScalarAttribute("repeatable", &repeatable))
    {
    this->Repeatable = (repeatable != 0);
    }

  // Yup, both mean the same thing :).
  int repeat_command;
  if (element->GetScalarAttribute("repeat_command", &repeat_command))
    {
    this->Repeatable = (repeat_command != 0);
    }

  int information_only;
  if (element->GetScalarAttribute("information_only", &information_only))
    {
    this->InformationOnly = (information_only != 0);
    }

  int is_internal;
  if (element->GetScalarAttribute("is_internal", &is_internal))
    {
    this->SetIsInternal(is_internal != 0);
    }

  return true;
}

//----------------------------------------------------------------------------
vtkSIObject* vtkSIProperty::GetSIObject(vtkTypeUInt32 globalid)
{
  if (this->SIProxyObject)
    {
    return this->SIProxyObject->GetSIObject(globalid);
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSIProperty::ProcessMessage(vtkClientServerStream& stream)
{
  if (this->SIProxyObject && this->SIProxyObject->GetVTKObject())
    {
    return this->SIProxyObject->GetInterpreter()->ProcessStream(stream) != 0;
    }
  return this->SIProxyObject ? true : false;
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkSIProperty::GetVTKObject()
{
  if (this->SIProxyObject)
    {
    return this->SIProxyObject->GetVTKObject();
    }
  return NULL;
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkSIProperty::GetLastResult()
{
  if (this->SIProxyObject)
    {
    return this->SIProxyObject->GetInterpreter()->GetLastResult();
    }

  static vtkClientServerStream stream;
  return stream;
}

//----------------------------------------------------------------------------
void vtkSIProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
// CAUTION: This method should only be called for Command property only
//          and not for value property.
bool vtkSIProperty::Push(vtkSMMessage*, int)
{
  if (this->InformationOnly || !this->Command || this->GetVTKObject() == NULL)
    {
    return true;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke;
  stream << this->GetVTKObject() << this->Command;
  stream << vtkClientServerStream::End;
  return this->ProcessMessage(stream);
}
//----------------------------------------------------------------------------
// CAUTION: This method should only be called to retreive the cache value of the
//          property.
bool vtkSIProperty::Pull(vtkSMMessage* msg)
{
  if(!this->InformationOnly && this->Internals->HasCache())
    {
    ProxyState_Property *newProp = msg->AddExtension(ProxyState::property);
    newProp->CopyFrom(*this->Internals->CacheValue);
    }

  return true;
}
//----------------------------------------------------------------------------
void vtkSIProperty::SaveValueToCache(vtkSMMessage* message, int offset)
{
  const ProxyState_Property *prop =
      &message->GetExtension(ProxyState::property, offset);
  this->Internals->SaveToCache(prop);
}
