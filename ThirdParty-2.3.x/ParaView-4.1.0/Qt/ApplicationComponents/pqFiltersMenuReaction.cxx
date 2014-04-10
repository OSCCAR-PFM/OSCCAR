/*=========================================================================

   Program: ParaView
   Module:    pqFiltersMenuReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqFiltersMenuReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqChangeInputDialog.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPluginManager.h"
#include "pqProxyGroupMenuManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqCollaborationManager.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSmartPointer.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMDocumentation.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QMap>
#include <QDebug>

static vtkSMInputProperty* getInputProperty(vtkSMProxy* proxy)
{
  // if "Input" is present, we return that, otherwise the "first"
  // vtkSMInputProperty encountered is returned.

  vtkSMInputProperty *prop = vtkSMInputProperty::SafeDownCast(
    proxy->GetProperty("Input"));
  vtkSMPropertyIterator* propIter = proxy->NewPropertyIterator();
  for (propIter->Begin(); !prop && !propIter->IsAtEnd(); propIter->Next())
    {
    prop = vtkSMInputProperty::SafeDownCast(propIter->GetProperty());
    }

  propIter->Delete();
  return prop;
}

namespace
{
  QString getDomainDisplayText(vtkSMDomain* domain, vtkSMInputProperty*)
    {
    if (domain->IsA("vtkSMDataTypeDomain"))
      {
      QStringList types;
      vtkSMDataTypeDomain* dtd = static_cast<vtkSMDataTypeDomain*>(domain);
      for (unsigned int cc=0; cc < dtd->GetNumberOfDataTypes(); cc++)
        {
        types << dtd->GetDataType(cc);
        }

      return QString("Input data must be %1").arg(
        types.join(" or "));
      }
    else if (domain->IsA("vtkSMInputArrayDomain"))
      {
      vtkSMInputArrayDomain* iad = static_cast<vtkSMInputArrayDomain*>(domain);
      QString txt = (iad->GetAttributeType() == vtkSMInputArrayDomain::ANY?
        QString("Requires an attribute array") :
        QString("Requires a %1 attribute array").arg(iad->GetAttributeTypeAsString()));
      if (iad->GetNumberOfComponents() > 0)
        {
        txt += QString(" with %1 component(s)").arg(iad->GetNumberOfComponents());
        }
      return txt;
      }
    return QString("Requirements not met");
    }
}

//-----------------------------------------------------------------------------
pqFiltersMenuReaction::pqFiltersMenuReaction(
  pqProxyGroupMenuManager* menuManager)
: Superclass(menuManager)
{
  QObject::connect(&this->Timer, SIGNAL(timeout()),
    this, SLOT(updateEnableState()));
  this->Timer.setInterval(10);
  this->Timer.setSingleShot(true);

  QObject::connect(
    menuManager, SIGNAL(triggered(const QString&, const QString&)),
    this, SLOT(onTriggered(const QString&, const QString&)));

  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(serverChanged(pqServer*)),
    &this->Timer, SLOT(start()));
  QObject::connect(activeObjects, SIGNAL(selectionChanged(pqProxySelection)),
    &this->Timer, SLOT(start()));
  QObject::connect(activeObjects, SIGNAL(dataUpdated()),
    &this->Timer, SLOT(start()));
  QObject::connect(pqApplicationCore::instance()->getPluginManager(),
                   SIGNAL(pluginsUpdated()),
                   &this->Timer, SLOT(start()));

  QObject::connect(pqApplicationCore::instance(),
    SIGNAL(forceFilterMenuRefresh()),
    &this->Timer, SLOT(start()));

  QObject::connect(pqApplicationCore::instance(),
                   SIGNAL(updateMasterEnableState(bool)),
                   this, SLOT(updateEnableState()));

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqFiltersMenuReaction::updateEnableState()
{
  // Impossible to validate anything without any SessionProxyManager
  if(!vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager())
    {
    this->Timer.start(100); // Try later
    return;
    }

  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  pqServer* server = activeObjects->activeServer();
  bool enabled = (server != NULL);
  enabled = enabled ? server->isMaster() : enabled;

  // Make sure we already have a selection model
  vtkSMProxySelectionModel* selModel =
    pqActiveObjects::instance().activeSourcesSelectionModel();
  enabled = enabled && (selModel != NULL);

  // selected ports.
  QList<pqOutputPort*> outputPorts;

  // If active proxy is non-existent, then also the filters are disabled.
  if (enabled)
    {
    pqApplicationCore* core = pqApplicationCore::instance();
    pqServerManagerModel* smmodel = core->getServerManagerModel();

    for (unsigned int cc=0; cc < selModel->GetNumberOfSelectedProxies(); cc++)
      {
      pqServerManagerModelItem* item =
        smmodel->findItem<pqServerManagerModelItem*>(
          selModel->GetSelectedProxy(cc));
      pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
      pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
      if (opPort)
        {
        source = opPort->getSource();
        }
      else if (source)
        {
        opPort = source->getOutputPort(0);
        }
      if (source && source->modifiedState() == pqProxy::UNINITIALIZED)
        {
        enabled = false;
        // we will update when the active representation updates the data.
        break;
        }

      // Make sure we still have a valid port, this issue came up with multi-server
      if(opPort)
        {
        outputPorts.append(opPort);
        }
      }
    if (selModel->GetNumberOfSelectedProxies() == 0 || outputPorts.size() == 0)
      {
      enabled = false;
      }
    }

  pqProxyGroupMenuManager* mgr =
    static_cast<pqProxyGroupMenuManager*>(this->parent());
  mgr->setEnabled(enabled);
  bool some_enabled = false;
  foreach (QAction* action, mgr->actions())
    {
    vtkSMProxy* prototype = mgr->getPrototype(action);
    if (!prototype || !enabled)
      {
      action->setEnabled(false);
      action->setStatusTip("Requires an input");
      continue;
      }

    int numProcs = outputPorts[0]->getServer()->getNumberOfPartitions();
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(prototype);
    if (sp && (
        (sp->GetProcessSupport() == vtkSMSourceProxy::SINGLE_PROCESS && numProcs > 1) ||
        (sp->GetProcessSupport() == vtkSMSourceProxy::MULTIPLE_PROCESSES && numProcs == 1)))
      {
      // Skip single process filters when running in multiprocesses and vice
      // versa.
      action->setEnabled(false);
      if (numProcs > 1)
        {
        action->setStatusTip("Not supported in parallel");
        }
      else
        {
        action->setStatusTip("Supported only in parallel");
        }
      continue;
      }

    // TODO: Handle case where a proxy has multiple input properties.
    vtkSMInputProperty *input = ::getInputProperty(prototype);
    if (input)
      {
      if(!input->GetMultipleInput() && outputPorts.size() > 1)
        {
        action->setEnabled(false);
        action->setStatusTip("Multiple inputs not support");
        continue;
        }

      input->RemoveAllUncheckedProxies();
      for (int cc=0; cc < outputPorts.size(); cc++)
        {
        pqOutputPort* port = outputPorts[cc];
        input->AddUncheckedInputConnection(
          port->getSource()->getProxy(), port->getPortNumber());
        }

      vtkSMDomain* domain = NULL;
      if (input->IsInDomains(&domain))
        {
        action->setEnabled(true);
        some_enabled = true;
        const char* help = prototype->GetDocumentation()->GetShortHelp();
        action->setStatusTip(help? help : "");
        }
      else
        {
        action->setEnabled(false);
        // Here we need to go to the domain that returned false and find out why
        // it said the domain criteria wasn't met.
        action->setStatusTip(::getDomainDisplayText(domain, input));
        }
      input->RemoveAllUncheckedProxies();
      }
    }

  if (!some_enabled)
    {
    mgr->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqFiltersMenuReaction::createFilter(
  const QString& xmlgroup, const QString& xmlname)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSMProxy* prototype = pxm->GetPrototypeProxy(
    xmlgroup.toAscii().data(), xmlname.toAscii().data());
  if (!prototype)
    {
    qCritical() << "Unknown proxy type: " << xmlname;
    return 0;
    }

  // Get the list of selected sources.
  QMap<QString, QList<pqOutputPort*> > namedInputs;
  QList<pqOutputPort*> selectedOutputPorts;

  vtkSMProxySelectionModel* selModel =
    pqActiveObjects::instance().activeSourcesSelectionModel();
  // Determine the list of selected output ports.
  for (unsigned int cc=0; cc < selModel->GetNumberOfSelectedProxies(); cc++)
    {
    pqServerManagerModelItem* item =
      smmodel->findItem<pqServerManagerModelItem*>(
        selModel->GetSelectedProxy(cc));

    pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
    if (opPort)
      {
      selectedOutputPorts.push_back(opPort);
      }
    else if (source)
      {
      selectedOutputPorts.push_back(source->getOutputPort(0));
      }
    }

  QList<const char*> inputPortNames = pqPipelineFilter::getInputPorts(prototype);
  namedInputs[inputPortNames[0]] = selectedOutputPorts;

  // If the filter has more than 1 input ports, we are simply going to ask the
  // user to make selection for the inputs for each port. We may change that in
  // future to be smarter.
  if (pqPipelineFilter::getRequiredInputPorts(prototype).size() > 1)
    {
    vtkSMProxy* filterProxy = pxm->GetPrototypeProxy("filters",
      xmlname.toAscii().data());
    vtkSMPropertyHelper helper(filterProxy, inputPortNames[0]);
    helper.RemoveAllValues();

    foreach (pqOutputPort *outputPort, selectedOutputPorts)
      {
      helper.Add(outputPort->getSource()->getProxy(),
        outputPort->getPortNumber());
      }

    pqChangeInputDialog dialog(filterProxy, pqCoreUtilities::mainWidget());
    dialog.setObjectName("SelectInputDialog");
    if (QDialog::Accepted != dialog.exec())
      {
      helper.RemoveAllValues();
      // User aborted creation.
      return 0;
      }
    helper.RemoveAllValues();
    namedInputs = dialog.selectedInputs();
    }

  BEGIN_UNDO_SET(QString("Create '%1'").arg(xmlname));
  pqPipelineSource* filter = builder->createFilter("filters", xmlname,
    namedInputs, server);
  END_UNDO_SET();
  return filter;
}
