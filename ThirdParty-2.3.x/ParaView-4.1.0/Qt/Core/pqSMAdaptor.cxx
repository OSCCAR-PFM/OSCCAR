/*=========================================================================

   Program: ParaView
   Module:    pqSMAdaptor.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// self includes
#include "pqSMAdaptor.h"

// qt includes
#include <QString>
#include <QtDebug>
#include <QVariant>

// vtk includes
#include "vtkConfigure.h"   // for 64-bitness
#include "vtkStringList.h"
#include "vtkSmartPointer.h"

// server manager includes
#include "vtkSMArrayListDomain.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMExtentDomain.h"
#include "vtkSMFileListDomain.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSILDomain.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMVectorProperty.h"

// ParaView includes
#include "pqSMProxy.h"

#include <QStringList>

static const int metaId = qRegisterMetaType<QList<QList<QVariant> > >();

pqSMAdaptor::pqSMAdaptor()
{
}

pqSMAdaptor::~pqSMAdaptor()
{
}

pqSMAdaptor::PropertyType pqSMAdaptor::getPropertyType(vtkSMProperty* Property)
{

  pqSMAdaptor::PropertyType type = pqSMAdaptor::UNKNOWN;
  if(!Property)
    {
    return type;
    }

  vtkSMProxyProperty* proxy = vtkSMProxyProperty::SafeDownCast(Property);
  vtkSMVectorProperty* VectorProperty = 
    vtkSMVectorProperty::SafeDownCast(Property);
  
  if(proxy)
    {
    vtkSMInputProperty* input = vtkSMInputProperty::SafeDownCast(Property);
    if(input && input->GetMultipleInput())
      {
      type = pqSMAdaptor::PROXYLIST;
      }
    type = pqSMAdaptor::PROXY;
    if (vtkSMProxyListDomain::SafeDownCast(Property->GetDomain("proxy_list")))
      {
      type = pqSMAdaptor::PROXYSELECTION;
      }
    }
  else if(Property->GetDomain("field_list"))
    {
    type = pqSMAdaptor::FIELD_SELECTION;
    }
  else
    {
    vtkSMBooleanDomain* booleanDomain = NULL;
    vtkSMEnumerationDomain* enumerationDomain = NULL;
    vtkSMProxyGroupDomain* proxyGroupDomain = NULL;
    vtkSMFileListDomain* fileListDomain = NULL;
    vtkSMStringListDomain* stringListDomain = NULL;
    vtkSMCompositeTreeDomain* compositeTreeDomain = NULL;
    vtkSMSILDomain* silDomain = NULL;
    
    vtkSMDomainIterator* iter = Property->NewDomainIterator();
    for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      if (!silDomain)
        {
        silDomain = vtkSMSILDomain::SafeDownCast(iter->GetDomain());
        }
      if(!booleanDomain)
        {
        booleanDomain = vtkSMBooleanDomain::SafeDownCast(iter->GetDomain());
        }
      if(!enumerationDomain)
        {
        enumerationDomain = vtkSMEnumerationDomain::SafeDownCast(iter->GetDomain());
        }
      if(!proxyGroupDomain)
        {
        proxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(iter->GetDomain());
        }
      if(!fileListDomain)
        {
        fileListDomain = vtkSMFileListDomain::SafeDownCast(iter->GetDomain());
        }
      if(!stringListDomain)
        {
        stringListDomain = vtkSMStringListDomain::SafeDownCast(iter->GetDomain());
        }
      if (!compositeTreeDomain)
        {
        compositeTreeDomain = vtkSMCompositeTreeDomain::SafeDownCast(iter->GetDomain());
        }
      }
    iter->Delete();

    if(fileListDomain)
      {
      type = pqSMAdaptor::FILE_LIST;
      }
    else if (compositeTreeDomain)
      {
      type = pqSMAdaptor::COMPOSITE_TREE;
      }
    else if (silDomain)
      {
      type = pqSMAdaptor::SIL;
      }
    else if(!silDomain && (
      (VectorProperty && VectorProperty->GetRepeatCommand() && 
       (stringListDomain || enumerationDomain))))
      {
      type = pqSMAdaptor::SELECTION;
      }
    else if(booleanDomain || enumerationDomain || 
            proxyGroupDomain || stringListDomain)
      {
      type = pqSMAdaptor::ENUMERATION;
      }
    else 
      {
      if(VectorProperty && 
        (VectorProperty->GetNumberOfElements() > 1 || VectorProperty->GetRepeatCommand()))
        {
        type = pqSMAdaptor::MULTIPLE_ELEMENTS;
        }
      else if(VectorProperty && VectorProperty->GetNumberOfElements() == 1)
        {
        type = pqSMAdaptor::SINGLE_ELEMENT;
        }
      }
    }

  return type;
}

pqSMProxy pqSMAdaptor::getProxyProperty(vtkSMProperty *Property,
                                        PropertyValueType Type)
{
  pqSMAdaptor::PropertyType propertyType = pqSMAdaptor::getPropertyType(Property);
  if( propertyType == pqSMAdaptor::PROXY || propertyType == pqSMAdaptor::PROXYSELECTION)
    {
    vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);

    if(Type == CHECKED)
      {
      if(proxyProp->GetNumberOfProxies())
        {
        return pqSMProxy(proxyProp->GetProxy(0));
        }
      }
    else if(Type == UNCHECKED)
      {
      if(proxyProp->GetNumberOfUncheckedProxies())
        {
        return pqSMProxy(proxyProp->GetUncheckedProxy(0));
        }
      }
    }
  return pqSMProxy(NULL);
}

void pqSMAdaptor::removeProxyProperty(vtkSMProperty* Property, pqSMProxy Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    proxyProp->RemoveProxy(Value);
    }
}

//-----------------------------------------------------------------------------
void pqSMAdaptor::addInputProperty(vtkSMProperty* Property, 
                               pqSMProxy Value, int opport)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(Property);
  if (ip)
    {
    ip->AddInputConnection(Value, opport);
    }
}

//-----------------------------------------------------------------------------
void pqSMAdaptor::setInputProperty(vtkSMProperty* Property, 
                                   pqSMProxy Value, int opport)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(Property);
  if (ip)
    {
    if (ip->GetNumberOfProxies() == 1)
      {
      ip->SetInputConnection(0, Value, opport);
      }
    else
      {
      ip->RemoveAllProxies();
      ip->AddInputConnection(Value, opport);
      }
    }
}

//-----------------------------------------------------------------------------
void pqSMAdaptor::addProxyProperty(vtkSMProperty* Property, 
                                   pqSMProxy Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    proxyProp->AddProxy(Value);
    }
}

//-----------------------------------------------------------------------------
void pqSMAdaptor::setProxyProperty(vtkSMProperty* Property, 
                                   pqSMProxy Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    if (proxyProp->GetNumberOfProxies() == 1)
      {
      proxyProp->SetProxy(0, Value);
      }
    else
      {
      proxyProp->RemoveAllProxies();
      proxyProp->AddProxy(Value);
      }
    }
}

void pqSMAdaptor::setUncheckedProxyProperty(vtkSMProperty* Property,
                                   pqSMProxy Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    proxyProp->RemoveAllUncheckedProxies();
    proxyProp->AddUncheckedProxy(Value);
    }
}

QList<pqSMProxy> pqSMAdaptor::getProxyListProperty(vtkSMProperty* Property)
{
  QList<pqSMProxy> value;
  if(pqSMAdaptor::getPropertyType(Property) == pqSMAdaptor::PROXYLIST)
    {
    vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
    unsigned int num = proxyProp->GetNumberOfProxies();
    for(unsigned int i=0; i<num; i++)
      {
      value.append(proxyProp->GetProxy(i));
      }
    }
  return value;
}

void pqSMAdaptor::setProxyListProperty(vtkSMProperty* Property, 
                                       QList<pqSMProxy> Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if (proxyProp)
    {
    proxyProp->RemoveAllProxies();
    foreach(pqSMProxy p, Value)
      {
      proxyProp->AddProxy(p);
      }
    }
}

QList<pqSMProxy> pqSMAdaptor::getProxyPropertyDomain(vtkSMProperty* Property)
{
  QList<pqSMProxy> proxydomain;
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    vtkSMSessionProxyManager* pm = Property->GetParent()->GetSessionProxyManager();
    
    // get group domain of this property 
    // and add all proxies in those groups to our list
    vtkSMProxyGroupDomain* gd;
    vtkSMProxyListDomain* ld;
    ld = vtkSMProxyListDomain::SafeDownCast(Property->GetDomain("proxy_list"));
    gd = vtkSMProxyGroupDomain::SafeDownCast(Property->GetDomain("groups"));
    if (ld)
      {
      unsigned int numProxies = ld->GetNumberOfProxies();
      for (unsigned int cc=0; cc < numProxies; cc++)
        {
        proxydomain.append(ld->GetProxy(cc));
        }
      }
    else if (gd)
      {
      unsigned int numGroups = gd->GetNumberOfGroups();
      for(unsigned int i=0; i<numGroups; i++)
        {
        const char* group = gd->GetGroup(i);
        unsigned int numProxies = pm->GetNumberOfProxies(group);
        for(unsigned int j=0; j<numProxies; j++)
          {
          pqSMProxy p = pm->GetProxy(group, pm->GetProxyName(group, j));
          proxydomain.append(p);
          }
        }
      }
    }
  return proxydomain;
}


QList<QList<QVariant> > pqSMAdaptor::getSelectionProperty(vtkSMProperty* Property,
                                                          PropertyValueType Type)
{
  QList<QList<QVariant> > ret;

  if(!Property)
    {
    return ret;
    }
  
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  int numSelections = 0;
  if(EnumerationDomain)
    {
    numSelections = EnumerationDomain->GetNumberOfEntries();
    }
  else if(StringListDomain)
    {
    numSelections = StringListDomain->GetNumberOfStrings();
    }

  for(int i=0; i<numSelections; i++)
    {
    QList<QVariant> tmp;
    tmp = pqSMAdaptor::getSelectionProperty(Property, i, Type);
    ret.append(tmp);
    }

  return ret;
}

QList<QVariant> pqSMAdaptor::getSelectionProperty(vtkSMProperty* Property, 
                                                  unsigned int Index,
                                                  PropertyValueType Type)
{
  QList<QVariant> ret;
  
  if(!Property)
    {
    return ret;
    }
  
  vtkSMArraySelectionDomain* StringDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringDomain)
      {
      StringDomain = vtkSMArraySelectionDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  vtkSMStringVectorProperty* StringProperty = NULL;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(Property);
  if(StringProperty && StringDomain)
    {
    QString StringName = StringDomain->GetString(Index);
    if(!StringName.isNull())
      {
      ret.append(StringName);
      QVariant value;

      int numElements = Type == UNCHECKED ? StringProperty->GetNumberOfUncheckedElements() :
                                            StringProperty->GetNumberOfElements();
      if(numElements % 2 == 0)
        {
        for(int i=0; i<numElements; i+=2)
          {
          if(Type == UNCHECKED)
            {
            if(StringName == StringProperty->GetUncheckedElement(i))
              {
              value = StringProperty->GetUncheckedElement(i+1);
              break;
              }
            }
          else if(Type == CHECKED)
            {
            if(StringName == StringProperty->GetElement(i))
              {
              value = StringProperty->GetElement(i+1);
              break;
              }
            }
          }
        }

      vtkSMStringVectorProperty* infoSP = vtkSMStringVectorProperty::SafeDownCast(
        StringProperty->GetInformationProperty());
      if (!value.isValid() && infoSP)
        {
        // check if the information property is giving us the status for the
        // array selection.

        numElements = Type == UNCHECKED ? infoSP->GetNumberOfUncheckedElements() :
                                          infoSP->GetNumberOfElements();
        for(int i=0; (i+1)<numElements; i+=2)
          {
          if(Type == UNCHECKED)
            {
            if(StringName == infoSP->GetUncheckedElement(i))
              {
              value = infoSP->GetUncheckedElement(i+1);
              break;
              }
            }
          else if(Type == CHECKED)
            {
            if(StringName == infoSP->GetElement(i))
              {
              value = infoSP->GetElement(i+1);
              break;
              }
            }
          }
        }

      // make up a zero
      if(!value.isValid())
        {
        qWarning("had to make up a value for selection\n");
        value = "0";
        }
      ret.append(value);
      }
    }
  else if(StringListDomain)
    {
    QList<QVariant> values =
      pqSMAdaptor::getMultipleElementProperty(Property, Type);
    
    if(Index < StringListDomain->GetNumberOfStrings())
      {
      QVariant whichDomain = StringListDomain->GetString(Index);
      ret.append(whichDomain);
      if(values.contains(whichDomain))
        {
        ret.append(1);
        }
      else
        {
        ret.append(0);
        }
      }
    else
      {
      qWarning("index out of range for arraylist domain\n");
      }
    }
  else if(EnumerationDomain)
    {
    QList<QVariant> values =
      pqSMAdaptor::getMultipleElementProperty(Property, Type);

    if(Index < EnumerationDomain->GetNumberOfEntries())
      {
      ret.append(EnumerationDomain->GetEntryText(Index));
      if(values.contains(EnumerationDomain->GetEntryValue(Index)))
        {
        ret.append(1);
        }
      else
        {
        ret.append(0);
        }
      }
    else
      {
      qWarning("index out of range for enumeration domain\n");
      }
    }

  return ret;
}

void pqSMAdaptor::setSelectionProperty(vtkSMProperty* Property,
                                   QList<QVariant> value,
                                   PropertyValueType Type)
{
  if(!Property)
    {
    return;
    }

  vtkSMArraySelectionDomain* StringDomain = NULL;

  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringDomain)
      {
      StringDomain = vtkSMArraySelectionDomain::SafeDownCast(d);
      break;
      }
    iter->Next();
    }
  iter->Delete();
  if (!StringDomain)
    {
    // unlike the other overload of setSelectionProperty(), this can only work
    // with vtkSMArraySelectionDomain and not vtkSMStringListDomain or
    // vtkSMEnumerationDomain. That's because for those domains we need the full
    // list of elements to be updated correctly.
    qCritical() << "Only vtkSMArraySelectionDomain are supported.";
    return;
    }

  if (value.size() != 2)
    {
    qCritical() << "Method expected a list of pairs. Incorrect API." << endl;
    return;
    }

  QList<QVariant> current_value = pqSMAdaptor::getMultipleElementProperty(
    Property, Type);

  QString name = value[0].toString();
  QVariant status = value[1];
  if (status.type() == QVariant::Bool)
    {
    status = status.toInt();
    }

  bool name_found = false;
  for (int cc=0; (cc+1) < current_value.size(); cc++)
    {
    if (current_value[cc].toString() == name)
      {
      current_value[cc+1] = status;
      name_found = true;
      break;
      }
    }
  if (!name_found)
    {
    current_value.push_back(name);
    current_value.push_back(status);
    }

  pqSMAdaptor::setMultipleElementProperty(Property, current_value, Type);
}

void pqSMAdaptor::setSelectionProperty(vtkSMProperty* Property,
                                       QList<QList<QVariant> > Value,
                                       PropertyValueType Type)
{
  if(!Property)
    {
    return;
    }

  QList<QVariant> sm_value;

  vtkSMArraySelectionDomain* StringDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;

  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringDomain)
      {
      StringDomain = vtkSMArraySelectionDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();

  foreach(QList<QVariant> value, Value)
    {
    if (value.size() != 2)
      {
      qCritical() << "Method expected a list of pairs. Incorrect API." << endl;
      }

    if (StringDomain)
      {
      QString name = value[0].toString();
      QVariant status = value[1];
      if (status.type() == QVariant::Bool)
        {
        status = status.toInt();
        }
      sm_value.push_back(name);
      sm_value.push_back(status);
      }
    else if(EnumerationDomain)
      {
      QList<QVariant> domainStrings =
        pqSMAdaptor::getEnumerationPropertyDomain(Property);
      int idx = domainStrings.indexOf(value[0]);
      if (value[1].toInt() && idx != -1)
        {
        sm_value.push_back(EnumerationDomain->GetEntryValue(idx));
        }
      }
    else if(StringListDomain)
      {
      if (value[1].toInt())
        {
        sm_value.push_back(value[0]);
        }
      }
    }

  pqSMAdaptor::setMultipleElementProperty(Property, sm_value, Type);
}

QList<QVariant> pqSMAdaptor::getSelectionPropertyDomain(vtkSMProperty* Property)
{
  QList<QVariant> ret;
  if(!Property)
    {
    return ret;
    }

  vtkSMVectorProperty* VProperty = vtkSMVectorProperty::SafeDownCast(Property);

  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;

  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();

  if(EnumerationDomain && VProperty->GetRepeatCommand())
    {
    unsigned int numEntries = EnumerationDomain->GetNumberOfEntries();
    for(unsigned int i=0; i<numEntries; i++)
      {
      ret.append(EnumerationDomain->GetEntryText(i));
      }
    }
  else if(StringListDomain && VProperty->GetRepeatCommand())
    {
    unsigned int numEntries = StringListDomain->GetNumberOfStrings();
    for(unsigned int i=0; i<numEntries; i++)
      {
      ret.append(StringListDomain->GetString(i));
      }
    }
  
  return ret;
}

QVariant pqSMAdaptor::getEnumerationProperty(vtkSMProperty* Property,
                                             PropertyValueType Type)
{
  QVariant var;
  if(!Property)
    {
    return var;
    }

  vtkSMBooleanDomain* BooleanDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMProxyGroupDomain* ProxyGroupDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!BooleanDomain)
      {
      BooleanDomain = vtkSMBooleanDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!ProxyGroupDomain)
      {
      ProxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  vtkSMIntVectorProperty* ivp;
  vtkSMStringVectorProperty* svp;
  vtkSMProxyProperty* pp;
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);
  pp = vtkSMProxyProperty::SafeDownCast(Property);

  unsigned int ivpElementCount = 0;
  unsigned int svpElementCount = 0;
  unsigned int ppProxyCount = 0;

  if(Type == CHECKED)
    {
    ivpElementCount = ivp ? ivp->GetNumberOfElements() : 0;
    svpElementCount = svp ? svp->GetNumberOfElements() : 0;
    ppProxyCount = pp ? pp->GetNumberOfProxies() : 0;
    }
  else if(Type == UNCHECKED)
    {
    ivpElementCount = ivp ? ivp->GetNumberOfUncheckedElements() : 0;
    svpElementCount = svp ? svp->GetNumberOfUncheckedElements() : 0;
    ppProxyCount = pp ? pp->GetNumberOfUncheckedProxies() : 0;
    }

  if(BooleanDomain && ivpElementCount > 0)
    {
    if(Type == CHECKED)
      {
      var = (ivp->GetElement(0)) == 0 ? false : true;
      }
    else if(Type == UNCHECKED)
      {
      var = (ivp->GetUncheckedElement(0)) == 0 ? false : true;
      }
    }
  else if(EnumerationDomain && ivpElementCount > 0)
    {
    int val = 0;

    if(Type == CHECKED)
      {
      val = ivp->GetElement(0);
      }
    else if(Type == UNCHECKED)
      {
      val = ivp->GetUncheckedElement(0);
      }

    for (unsigned int i = 0; i < EnumerationDomain->GetNumberOfEntries(); i++)
      {
      if (EnumerationDomain->GetEntryValue(i) == val)
        {
        var = EnumerationDomain->GetEntryText(i);
        break;
        }
      }
    }
  else if(StringListDomain && svp)
    {
    for (unsigned int i=0; i < svpElementCount ; i++)
      {
      if (svp->GetElementType(i) == vtkSMStringVectorProperty::STRING)
        {
        if(Type == CHECKED)
          {
          var = svp->GetElement(i);
          break;
          }
        else if(Type == UNCHECKED)
          {
          var = svp->GetUncheckedElement(i);
          }
        }
      }
    }
  else if (ProxyGroupDomain && ppProxyCount > 0)
    {
    vtkSMProxy* p = 0;

    if(Type == CHECKED)
      {
      p = pp->GetProxy(0);
      }
    else if(Type == UNCHECKED)
      {
      p = pp->GetUncheckedProxy(0);
      }

    if(p)
      {
      var = ProxyGroupDomain->GetProxyName(p);
      }
    }

  return var;
}

void pqSMAdaptor::setEnumerationProperty(vtkSMProperty* Property,
                                         QVariant Value,
                                         PropertyValueType Type)
{
  if(!Property)
    {
    return;
    }

  vtkSMBooleanDomain* BooleanDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMProxyGroupDomain* ProxyGroupDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!BooleanDomain)
      {
      BooleanDomain = vtkSMBooleanDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!ProxyGroupDomain)
      {
      ProxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  vtkSMIntVectorProperty* ivp;
  vtkSMStringVectorProperty* svp;
  vtkSMProxyProperty* pp;
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);
  pp = vtkSMProxyProperty::SafeDownCast(Property);

  if(BooleanDomain && ivp && ivp->GetNumberOfElements() > 0)
    {
    bool ok = true;
    int v = Value.toInt(&ok);
    if(ok)
      {
      if (Type == CHECKED) { ivp->SetElement(0, v); }
      else { ivp->SetUncheckedElement(0, v); }
      }
    }
  else if(EnumerationDomain && ivp)
    {
    QString str = Value.toString();
    unsigned int numEntries = EnumerationDomain->GetNumberOfEntries();
    for(unsigned int i=0; i<numEntries; i++)
      {
      if(str == EnumerationDomain->GetEntryText(i))
        {
        if(Type == CHECKED)
          {
          ivp->SetElement(0, EnumerationDomain->GetEntryValue(i));
          }
        else if(Type == UNCHECKED)
          {
          ivp->SetUncheckedElement(0, EnumerationDomain->GetEntryValue(i));
          }
        }
      }
    }
  else if(StringListDomain && svp)
    {
    unsigned int nos = svp->GetNumberOfElements();
    for (unsigned int i=0; i < nos ; i++)
      {
      if (svp->GetElementType(i) == vtkSMStringVectorProperty::STRING)
        {
        if(Type == CHECKED)
          {
          svp->SetElement(i, Value.toString().toAscii().data());
          }
        else if(Type == UNCHECKED)
          {
          svp->SetUncheckedElement(i, Value.toString().toAscii().data());
          }
        }
      }
    if(Type == UNCHECKED)
      {
      }
    }
  else if (ProxyGroupDomain && pp)
    {
    QString str = Value.toString();
    vtkSMProxy* toadd = ProxyGroupDomain->GetProxy(str.toAscii().data());
    if (pp->GetNumberOfProxies() < 1)
      {
      if(Type == CHECKED)
        {
        pp->AddProxy(toadd);
        }
      else if(Type == UNCHECKED)
        {
        pp->AddUncheckedProxy(toadd);
        }
      }
    else
      {
      if(Type == CHECKED)
        {
        pp->SetProxy(0, toadd);
        }
      else if(Type == UNCHECKED)
        {
        pp->SetUncheckedProxy(0, toadd);
        }
      }
    }
}

QList<QVariant> pqSMAdaptor::getEnumerationPropertyDomain(
                                          vtkSMProperty* Property)
{
  QList<QVariant> enumerations;
  if(!Property)
    {
    return enumerations;
    }

  vtkSMBooleanDomain* BooleanDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMProxyGroupDomain* ProxyGroupDomain = NULL;
  vtkSMArrayListDomain* ArrayListDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!BooleanDomain)
      {
      BooleanDomain = vtkSMBooleanDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!ArrayListDomain)
      {
      ArrayListDomain = vtkSMArrayListDomain::SafeDownCast(d);
      }
    if(!ProxyGroupDomain)
      {
      ProxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();

  if(BooleanDomain)
    {
    enumerations.push_back(false);
    enumerations.push_back(true);
    }
  else if(ArrayListDomain)
    {
    unsigned int numEntries = ArrayListDomain->GetNumberOfStrings();
    for(unsigned int i=0; i<numEntries; i++)
      {
      enumerations.push_back(ArrayListDomain->GetString(i));
      }
    }
  else if(EnumerationDomain)
    {
    unsigned int numEntries = EnumerationDomain->GetNumberOfEntries();
    for(unsigned int i=0; i<numEntries; i++)
      {
      enumerations.push_back(EnumerationDomain->GetEntryText(i));
      }
    }
  else if(ProxyGroupDomain)
    {
    unsigned int numEntries = ProxyGroupDomain->GetNumberOfProxies();
    for(unsigned int i=0; i<numEntries; i++)
      {
      enumerations.push_back(ProxyGroupDomain->GetProxyName(i));
      }
    }
  else if(StringListDomain)
    {
    unsigned int numEntries = StringListDomain->GetNumberOfStrings();
    for(unsigned int i=0; i<numEntries; i++)
      {
      enumerations.push_back(StringListDomain->GetString(i));
      }
    }
  
  return enumerations;
}

QVariant pqSMAdaptor::getElementProperty(vtkSMProperty* Property, PropertyValueType Type)
{
  return pqSMAdaptor::getMultipleElementProperty(Property, 0, Type);
}

void pqSMAdaptor::setElementProperty(vtkSMProperty* Property, QVariant Value, PropertyValueType Type)
{
  pqSMAdaptor::setMultipleElementProperty(Property, 0, Value, Type);
}

QList<QVariant> pqSMAdaptor::getElementPropertyDomain(vtkSMProperty* Property)
{
  return pqSMAdaptor::getMultipleElementPropertyDomain(Property, 0);
}
  
QList<QVariant> pqSMAdaptor::getMultipleElementProperty(vtkSMProperty* Property,
                                                        PropertyValueType Type)
{
  QList<QVariant> props;
  
  vtkSMVectorProperty* VectorProperty;
  VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(!VectorProperty)
    {
    return props;
    }

  int num = 0;

  if(Type == CHECKED)
    {
    num = VectorProperty->GetNumberOfElements();
    }
  else if(Type == UNCHECKED)
    {
    num = VectorProperty->GetNumberOfUncheckedElements();
    }

  for(int i=0; i<num; i++)
    {
    props.push_back(pqSMAdaptor::getMultipleElementProperty(Property,
                                                            i,
                                                            Type));
    }

  return props;
}

void pqSMAdaptor::setMultipleElementProperty(vtkSMProperty* Property, 
                                             QList<QVariant> Value,
                                             PropertyValueType Type)
{
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;
  vtkSMIdTypeVectorProperty* idvp;
  vtkSMStringVectorProperty* svp;
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(Property);
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  idvp = vtkSMIdTypeVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  int num = Value.size();

  if(dvp)
    {
    double *dvalues = new double[num+1];
    for(int i=0; i<num; i++)
      {
      bool ok = true;
      double v = Value[i].toDouble(&ok);
      dvalues[i] = ok? v : 0.0;
      }

    if(Type == CHECKED)
      {
      if (num > 0)
        {
        dvp->SetElements(dvalues, num);
        }
      }
    else if(Type == UNCHECKED)
      {
      if (num > 0)
        {
        dvp->SetUncheckedElements(dvalues, num);
        }
      }

    delete[] dvalues;
    }
  else if(ivp)
    {
    int *ivalues = new int[num+1];
    for(int i=0; i<num; i++)
      {
      bool ok = true;
      int v = Value[i].toInt(&ok);
      ivalues[i] = ok? v : 0;
      }

    if(Type == CHECKED)
      {
      if (num>0)
        {
        ivp->SetElements(ivalues, num);
        }
      }
    else if(Type == UNCHECKED)
      {
      if(num > 0)
        {
        ivp->SetUncheckedElements(ivalues, num);
        }
      }

    delete []ivalues;
    }
  else if(svp)
    {
    const char** cvalues = new const char*[num];
    std::string *str_values= new std::string[num];
    for (int cc=0; cc < num; cc++)
      {
      str_values[cc] = Value[cc].toString().toAscii().data();
      cvalues[cc] = str_values[cc].c_str();
      }

    if(Type == CHECKED)
      {
      svp->SetElements(cvalues, num);
      }
    else if(Type == UNCHECKED)
      {
      svp->SetUncheckedElements(cvalues, num);
      }

    delete []cvalues;
    delete []str_values;
    }
  else if(idvp)
    {
    vtkIdType* idvalues = new vtkIdType[num+1];
    for(int i=0; i<num; i++)
      {
      bool ok = true;
      vtkIdType v;
#if defined(VTK_USE_64BIT_IDS)
      v = Value[i].toLongLong(&ok);
#else
      v = Value[i].toInt(&ok);
#endif
      idvalues[i] = ok? v : 0;
      }

    if(Type == CHECKED)
      {
      if (num>0)
        {
        idvp->SetElements(idvalues, num);
        }
      }
    else if(Type == UNCHECKED)
      {
      if (num>0)
        {
        idvp->SetUncheckedElements(idvalues, num);
        }
      }

    delete[] idvalues;
    }

  if(Type == UNCHECKED)
    {
    }
}

QList<QList<QVariant> > pqSMAdaptor::getMultipleElementPropertyDomain(vtkSMProperty* Property)
{
  QList< QList<QVariant> > domains;
  if(!Property)
    {
    return domains;
    }
  
  vtkSMDoubleRangeDomain* DoubleDomain = NULL;
  vtkSMIntRangeDomain* IntDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd() && !DoubleDomain && !IntDomain)
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!DoubleDomain)
      {
      DoubleDomain = vtkSMDoubleRangeDomain::SafeDownCast(d);
      }
    if(!IntDomain)
      {
      IntDomain = vtkSMIntRangeDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();

  if(DoubleDomain)
    {
    vtkSMDoubleVectorProperty* dvp;
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(Property);
    vtkSMArrayRangeDomain* arrayDomain;
    arrayDomain = vtkSMArrayRangeDomain::SafeDownCast(DoubleDomain);

    unsigned int numElems = dvp->GetNumberOfElements();
    for(unsigned int i=0; i<numElems; i++)
      {
      QList<QVariant> domain;
      int exists1, exists2;
      int which = i;
      if(arrayDomain)
        {
        which = 0;
        }
      double min = DoubleDomain->GetMinimum(which, exists1);
      double max = DoubleDomain->GetMaximum(which, exists2);
      domain.push_back(exists1 ? min : QVariant());
      domain.push_back(exists2 ? max : QVariant());
      domains.push_back(domain);
      }
    }
  else if(IntDomain)
    {
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(Property);

    unsigned int numElems = ivp->GetNumberOfElements();
    vtkSMExtentDomain* extDomain = vtkSMExtentDomain::SafeDownCast(IntDomain);
    
    for(unsigned int i=0; i<numElems; i++)
      {
      int which = i;
      if(extDomain)
        {
        which /= 2;
        }
      else
        {  
        // one min/max for all elements
        which = 0;
        }
      QList<QVariant> domain;
      int exists1, exists2;
      int min = IntDomain->GetMinimum(which, exists1);
      int max = IntDomain->GetMaximum(which, exists2);
      domain.push_back(exists1 ? min : QVariant());
      domain.push_back(exists2 ? max : QVariant());
      domains.push_back(domain);
      }
    }

  return domains;
}

QVariant pqSMAdaptor::getMultipleElementProperty(vtkSMProperty* Property,
                                                 unsigned int Index,
                                                 PropertyValueType Type)
{
  vtkVariant variant;

  if(Type == CHECKED)
    {
    if (vtkSMPropertyHelper(Property).GetNumberOfElements() > Index)
      {
      variant = vtkSMPropertyHelper(Property).GetAsVariant(Index);
      }
    else
      {
      return QVariant();
      }
    }
  else if(Type == UNCHECKED)
    {
    if (vtkSMUncheckedPropertyHelper(Property).GetNumberOfElements() > Index)
      {
      variant = vtkSMUncheckedPropertyHelper(Property).GetAsVariant(Index);
      }
    else
      {
      return QVariant();
      }
    }

  return convertToQVariant(variant);
}

void pqSMAdaptor::setMultipleElementProperty(vtkSMProperty* Property, 
                                             unsigned int Index,
                                             QVariant Value,
                                             PropertyValueType Type)
{
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;
  vtkSMIdTypeVectorProperty* idvp;
  vtkSMStringVectorProperty* svp;
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(Property);
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  idvp = vtkSMIdTypeVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(dvp)
    {
    bool ok = true;
    double v = Value.toDouble(&ok);
    if(ok)
      {
      if(Type == CHECKED)
        {
        dvp->SetElement(Index, v);
        }
      else if(Type == UNCHECKED)
        {
        dvp->SetUncheckedElement(Index, v);
        }
      }
    }
  else if(ivp)
    {
    bool ok = true;
    int v = Value.toInt(&ok);
    if (!ok && Value.canConvert(QVariant::Bool))
      {
      v = Value.toBool()? 1 : 0;
      ok = true;
      }
    if(ok)
      {
      if(Type == CHECKED)
        {
        ivp->SetElement(Index, v);
        }
      else if(Type == UNCHECKED)
        {
        ivp->SetUncheckedElement(Index, v);
        }
      }
    }
  else if(svp)
    {
    QString v = Value.toString();
    if(!v.isNull())
      {
      if(Type == CHECKED)
        {
        svp->SetElement(Index, v.toAscii().constData());
        }
      else if(Type == UNCHECKED)
        {
        svp->SetUncheckedElement(Index, v.toAscii().constData());
        }
      }
    }
  else if(idvp)
    {
    bool ok = true;
    vtkIdType v;
#if defined(VTK_USE_64BIT_IDS)
    v = Value.toLongLong(&ok);
#else
    v = Value.toInt(&ok);
#endif
    if(ok)
      {
      if(Type == CHECKED)
        {
        idvp->SetElement(Index, v);
        }
      else if(Type == UNCHECKED)
        {
        idvp->SetUncheckedElement(Index, v);
        }
      }
    }
}

QList<QVariant> pqSMAdaptor::getMultipleElementPropertyDomain(
                        vtkSMProperty* Property, unsigned int Index)
{
  QList<QVariant> domain;
  if(!Property)
    {
    return domain;
    }
  
  vtkSMDoubleRangeDomain* DoubleDomain = NULL;
  vtkSMIntRangeDomain* IntDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!DoubleDomain)
      {
      DoubleDomain = vtkSMDoubleRangeDomain::SafeDownCast(d);
      }
    if(!IntDomain)
      {
      IntDomain = vtkSMIntRangeDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();

  int which = 0;
  vtkSMExtentDomain* extDomain = vtkSMExtentDomain::SafeDownCast(IntDomain);
  if(extDomain)
    {
    which = Index/2;
    }

  if(DoubleDomain)
    {
    int exists1, exists2;
    double min = DoubleDomain->GetMinimum(which, exists1);
    double max = DoubleDomain->GetMaximum(which, exists2);
    domain.push_back(exists1 ? min : QVariant());
    domain.push_back(exists2 ? max : QVariant());
    }
  else if(IntDomain)
    {
    int exists1, exists2;
    int min = IntDomain->GetMinimum(which, exists1);
    int max = IntDomain->GetMaximum(which, exists2);
    domain.push_back(exists1 ? min : QVariant());
    domain.push_back(exists2 ? max : QVariant());
    }

  return domain;
}

QStringList pqSMAdaptor::getFileListProperty(vtkSMProperty* Property,
                                             PropertyValueType Type)
{
  QStringList files;

  vtkSMStringVectorProperty* svp;
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if (svp)
    {
    unsigned int elementCount = 0;
    if(Type == CHECKED)
      {
      elementCount = svp->GetNumberOfElements();
      }
    else if(Type == UNCHECKED)
      {
      elementCount = svp->GetNumberOfUncheckedElements();
      }

    for (unsigned int i = 0; i < elementCount; i++)
      {
      if(Type == CHECKED)
        {
        files.append(svp->GetElement(i));
        }
      else if(Type == UNCHECKED)
        {
        files.append(svp->GetUncheckedElement(i));
        }
      }
    }

  return files;
}

void pqSMAdaptor::setFileListProperty(vtkSMProperty* Property,
                                      QStringList Value,
                                      PropertyValueType Type)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if (!svp)
    {
    return;
    }

  unsigned int i = 0;
  foreach (const QString &file, Value)
    {
    unsigned int elementCount = 0;
    if(Type == CHECKED)
      {
      elementCount = svp->GetNumberOfElements();
      }
    else if(Type == UNCHECKED)
      {
      elementCount = svp->GetNumberOfUncheckedElements();
      }

    if (!svp->GetRepeatCommand() && i >= elementCount)
      {
      break;
      }

    if(Type == CHECKED)
      {
      svp->SetElement(i, file.toAscii().data());
      }
    else if(Type == UNCHECKED)
      {
      svp->SetUncheckedElement(i, file.toAscii().data());
      }

    i++;
    }
}

QStringList pqSMAdaptor::getFieldSelection(vtkSMProperty *Property,
                                           PropertyValueType Type)
{
  vtkSMStringVectorProperty *StringVectorProperty =
    vtkSMStringVectorProperty::SafeDownCast(Property);
  vtkSMEnumerationDomain* domain =
    vtkSMEnumerationDomain::SafeDownCast(StringVectorProperty->GetDomain("field_list"));

  QString mode;
  QString scalars;

  if(StringVectorProperty && domain)
    {
    int which = -1;

    if(Type == CHECKED)
      {
      which = QString(StringVectorProperty->GetElement(3)).toInt();
      }
    else if(Type == UNCHECKED)
      {
      which = QString(StringVectorProperty->GetUncheckedElement(3)).toInt();
      }

    for(unsigned int i = 0; i < domain->GetNumberOfEntries(); i++)
      {
      if(domain->GetEntryValue(i) == which)
        {
        mode = domain->GetEntryText(i);
        break;
        }
      }

    if(Type == CHECKED)
      {
      scalars = StringVectorProperty->GetElement(4);
      }
    else if(Type == UNCHECKED)
      {
      scalars = StringVectorProperty->GetUncheckedElement(4);
      }
    }

  QStringList selection;
  selection.append(mode);
  selection.append(scalars);
  return selection;
}

void pqSMAdaptor::setFieldSelection(vtkSMProperty *prop,
                                    const QStringList &Value,
                                    PropertyValueType Type)
{
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMEnumerationDomain* domain =
    vtkSMEnumerationDomain::SafeDownCast(prop->GetDomain("field_list"));

  if(Value.size() != 2)
    {
    qDebug() << "pqSMAdaptor::setFieldSelection(): "
                "Value should be string list with (AttributeType, ArrayName).";
    return;
    }

  if(Property && domain)
    {
    for(unsigned int i = 0; i < domain->GetNumberOfEntries(); i++)
      {
      if(Value[0] == domain->GetEntryText(i))
        {
        std::string text = QString("%1").arg(
          domain->GetEntryValue(i)).toStdString();

        if(Type == CHECKED)
          {
          Property->SetElement(3, text.c_str());
          Property->SetElement(4, Value[1].toAscii().data());
          }
        else if(Type == UNCHECKED)
          {
          Property->SetUncheckedElement(3, text.c_str());
          Property->SetUncheckedElement(4, Value[1].toAscii().data());
          }
        break;
        }
      }
    }
}

QString pqSMAdaptor::getFieldSelectionMode(vtkSMProperty* prop,
                                           PropertyValueType Type)
{
  QString ret;
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMEnumerationDomain* domain =
    vtkSMEnumerationDomain::SafeDownCast(prop->GetDomain("field_list"));
  
  if(Property && domain)
    {
    int which = -1;

    if(Type == CHECKED)
      {
      which = QString(Property->GetElement(3)).toInt();
      }
    else if(Type == UNCHECKED)
      {
      which = QString(Property->GetUncheckedElement(3)).toInt();
      }

    int numEntries = domain->GetNumberOfEntries();
    for(int i=0; i<numEntries; i++)
      {
      if(domain->GetEntryValue(i) == which)
        {
        ret = domain->GetEntryText(i);
        break;
        }
      }
    }
  return ret;
}

void pqSMAdaptor::setFieldSelectionMode(vtkSMProperty* prop,
                                        const QString& val,
                                        PropertyValueType Type)
{
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMEnumerationDomain* domain =
    vtkSMEnumerationDomain::SafeDownCast(prop->GetDomain("field_list"));
  
  if(Property && domain)
    {
    int numEntries = domain->GetNumberOfEntries();
    for(int i=0; i<numEntries; i++)
      {
      if(val == domain->GetEntryText(i))
        {
        std::string text = QString("%1").arg(
          domain->GetEntryValue(i)).toStdString();

        if(Type == CHECKED)
          {
          Property->SetElement(3, text.c_str());
          }
        else if(Type == UNCHECKED)
          {
          Property->SetUncheckedElement(3, text.c_str());
          }
        break;
        }
      }
    }
}

QList<QString> pqSMAdaptor::getFieldSelectionModeDomain(vtkSMProperty* prop)
{
  QList<QString> types;
  if(!prop)
    {
    return types;
    }

  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMEnumerationDomain* domain =
    vtkSMEnumerationDomain::SafeDownCast(prop->GetDomain("field_list"));
  
  if(Property && domain)
    {
    int numEntries = domain->GetNumberOfEntries();
    for(int i=0; i<numEntries; i++)
      {
      types.append(domain->GetEntryText(i));
      }
    }
  return types;
}


QString pqSMAdaptor::getFieldSelectionScalar(vtkSMProperty* prop,
                                             PropertyValueType Type)
{
  QString ret;
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  
  if(Property)
    {
    if(Type == CHECKED)
      {
      ret = Property->GetElement(4);
      }
    else if(Type == UNCHECKED)
      {
      ret = Property->GetUncheckedElement(4);
      }
    }
  return ret;
}

void pqSMAdaptor::setFieldSelectionScalar(vtkSMProperty* prop, 
                                          const QString& val,
                                          PropertyValueType Type)
{
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  
  if(Property)
    {
    if(Type == CHECKED)
      {
      Property->SetElement(4, val.toAscii().data());
      }
    else if(Type == UNCHECKED)
      {
      Property->SetUncheckedElement(4, val.toAscii().data());
      }
    }
}

//-----------------------------------------------------------------------------
QList<QString> pqSMAdaptor::getFieldSelectionScalarDomain(vtkSMProperty* prop)
{
  QList<QString> types;
  if(!prop)
    {
    return types;
    }

  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMArrayListDomain* domain = prop ?
    vtkSMArrayListDomain::SafeDownCast(prop->GetDomain("array_list")) : 0;
  
  if(Property && domain)
    {
    int numEntries = domain->GetNumberOfStrings();
    for(int i=0; i<numEntries; i++)
      {
      types.append(domain->GetString(i));
      }
    }
  return types;
}

//-----------------------------------------------------------------------------
QList<QPair<QString, bool> > pqSMAdaptor::getFieldSelectionScalarDomainWithPartialArrays(
  vtkSMProperty* prop)
{
  QList<QPair<QString, bool> > types;
  if (!prop)
    {
    return types;
    }

  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMArrayListDomain* domain = prop ?
    vtkSMArrayListDomain::SafeDownCast(prop->GetDomain("array_list")) : 0;
  
  if(Property && domain)
    {
    int numEntries = domain->GetNumberOfStrings();
    for(int i=0; i<numEntries; i++)
      {
      types.append(QPair<QString, bool>(domain->GetString(i), domain->IsArrayPartial(i)!=0));
      }
    }
  return types;
}

//-----------------------------------------------------------------------------
QList<QString> pqSMAdaptor::getDomainTypes(vtkSMProperty* property)
{
  QList<QString> types;
  if(!property)
    {
    return types;
    }
  
  vtkSMDomainIterator* iter = property->NewDomainIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMDomain* d = iter->GetDomain();
    QString classname = d->GetClassName();
    if (!types.contains(classname))
      {
      types.push_back(classname);
      }
    }
  iter->Delete();
  return types;
}

//-----------------------------------------------------------------------------
void pqSMAdaptor::clearUncheckedProperties(vtkSMProperty *property)
{
  if(vtkSMVectorProperty *VectorProperty = vtkSMVectorProperty::SafeDownCast(property))
    {
    VectorProperty->ClearUncheckedElements();
    }
  else if(vtkSMProxyProperty *ProxyProperty = vtkSMProxyProperty::SafeDownCast(property))
    {
    ProxyProperty->ClearUncheckedProxies();
    }
}

//-----------------------------------------------------------------------------
QVariant pqSMAdaptor::convertToQVariant(const vtkVariant &variant)
{
  switch(variant.GetType())
    {
    case VTK_CHAR:
      return variant.ToChar();
    case VTK_UNSIGNED_CHAR:
      return variant.ToUnsignedChar();
    case VTK_SIGNED_CHAR:
      return variant.ToSignedChar();
    case VTK_SHORT:
      return variant.ToShort();
    case VTK_UNSIGNED_SHORT:
      return variant.ToUnsignedShort();
    case VTK_INT:
      return variant.ToInt();
    case VTK_UNSIGNED_INT:
      return variant.ToUnsignedInt();
#ifdef VTK_TYPE_USE___INT64
    case VTK___INT64:
      return variant.ToTypeInt64();
    case VTK_UNSIGNED___INT64:
      return variant.ToTypeUInt64();
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
    case VTK_LONG_LONG:
      return variant.ToLongLong();
    case VTK_UNSIGNED_LONG_LONG:
      return variant.ToUnsignedLongLong();
#endif
    case VTK_FLOAT:
      return variant.ToFloat();
    case VTK_DOUBLE:
      return variant.ToDouble();
    case VTK_STRING:
      return variant.ToString().c_str();
    case VTK_OBJECT:
      return QVariant(QMetaType::VoidStar, variant.ToVTKObject());
    default:
      return QVariant();
    }
}
