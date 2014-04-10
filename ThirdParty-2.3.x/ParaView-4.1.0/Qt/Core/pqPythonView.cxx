/*=========================================================================

  Program:   ParaView
  Module:    pqPythonView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqPythonView.h"

// Server Manager Includes.
#include "vtkErrorCode.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMInteractionUndoStackBuilder.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPythonViewProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUndoStack.h"

// Qt Includes.
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QMapIterator>
#include <QMenu>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QStatusBar>
#include <QtDebug>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqImageUtil.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqQVTKWidget.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqTimer.h"
#include "pqRenderView.h"

#include <string>

class pqPythonView::pqInternal
{
public:
  QPointer<QWidget> Viewport;
  QPoint MouseOrigin;
  bool InitializedWidgets;
  bool InitializedAfterObjectsCreated;

  pqInternal()
    {
    this->InitializedAfterObjectsCreated=false;
    this->InitializedWidgets = false;
    }
  ~pqInternal()
    {
    delete this->Viewport;
    }
};

//-----------------------------------------------------------------------------
pqPythonView::pqPythonView(
  const QString& type,
  const QString& group,
  const QString& name, 
  vtkSMViewProxy* renViewProxy, 
  pqServer* server, 
  QObject* _parent/*=NULL*/):
  Superclass(type, group, name, renViewProxy, server, _parent)
{
  this->Internal = new pqPythonView::pqInternal();
  this->AllowCaching = true;
}

//-----------------------------------------------------------------------------
pqPythonView::~pqPythonView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqPythonView::getWidget()
{
  if (!this->Internal->Viewport)
    {
    this->Internal->Viewport = this->createWidget();
    // we manage the context menu ourself, so it doesn't interfere with
    // render window interactions
    this->Internal->Viewport->setContextMenuPolicy(Qt::NoContextMenu);
    this->Internal->Viewport->installEventFilter(this);
    this->Internal->Viewport->setObjectName("Viewport");
    }

  return this->Internal->Viewport;
}

//-----------------------------------------------------------------------------
vtkSMPythonViewProxy* pqPythonView::getPythonViewProxy()
{
  return vtkSMPythonViewProxy::SafeDownCast(this->getViewProxy());
}

//-----------------------------------------------------------------------------
void pqPythonView::setPythonScript(QString & source)
{
  vtkSMPropertyHelper(this->getProxy(), "Script").Set(
    source.toStdString().c_str());
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QString pqPythonView::getPythonScript()
{
  const char* scriptString = vtkSMPropertyHelper(this->getProxy(),
                                                 "Script", true).GetAsString();
  if (scriptString)
    {
    return QString(scriptString);
    }
 
  return QString();
}

//-----------------------------------------------------------------------------
QWidget* pqPythonView::createWidget()
{
  pqQVTKWidget* vtkwidget = new pqQVTKWidget();
  vtkwidget->setViewProxy(this->getProxy());

  // do image caching for performance
  // For now, we are doing this only on Apple because it can render
  // and capture a frame buffer even when it is obstructred by a
  // window. This does not work as well on other platforms.

#if defined(__APPLE__)
  if (this->AllowCaching)
    {
    // Don't override the caching flag here. It is set correctly by
    // pqQVTKWidget.  I don't know why this explicit marking cached dirty was
    // done. But in case it's needed for streaming view, I am letting it be.
    // vtkwidget->setAutomaticImageCacheEnabled(true);

    // help the QVTKWidget know when to clear the cache
    this->getConnector()->Connect(
      this->getProxy(), vtkCommand::ModifiedEvent,
      vtkwidget, SLOT(markCachedImageAsDirty()));
    }
#endif

  return vtkwidget;
}

//-----------------------------------------------------------------------------
void pqPythonView::initialize()
{
  this->Superclass::initialize();

  // The render module needs to obtain client side objects
  // for the RenderWindow etc. to initialize the QVTKWidget
  // correctly. It cannot do this unless the underlying proxy
  // has been created. Since any pqProxy should never call
  // UpdateVTKObjects() on itself in the constructor, we 
  // do the following.
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy->GetObjectsCreated())
    {
    // Wait till first UpdateVTKObjects() call on the render module.
    // Under usual circumstances, after UpdateVTKObjects() the
    // render module objects will be created.
    this->getConnector()->Connect(proxy, vtkCommand::UpdateEvent,
      this, SLOT(initializeAfterObjectsCreated()));
    }
  else
    {
    this->initializeAfterObjectsCreated();
    }
}

//-----------------------------------------------------------------------------
void pqPythonView::initializeAfterObjectsCreated()
{
  if (!this->Internal->InitializedAfterObjectsCreated)
    {
    this->Internal->InitializedAfterObjectsCreated = true;
    this->initializeWidgets();
    }
}

//-----------------------------------------------------------------------------
void pqPythonView::initializeWidgets()
{
    if (this->Internal->InitializedWidgets)
    {
    return;
    }

  this->Internal->InitializedWidgets = true;

  vtkSMPythonViewProxy* renModule = this->getPythonViewProxy();

  QVTKWidget* vtkwidget = qobject_cast<QVTKWidget*>(this->getWidget());
  if (vtkwidget)
    {
    vtkwidget->SetRenderWindow(renModule->GetRenderWindow());
    this->render();
    }
}
  
//-----------------------------------------------------------------------------
bool pqPythonView::eventFilter(QObject* caller, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress)
    {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton)
      {
      this->Internal->MouseOrigin = me->pos();
      }
    this->render();
    }
  else if (e->type() == QEvent::MouseMove &&
    !this->Internal->MouseOrigin.isNull())
    {
    QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
    QPoint delta = newPos - this->Internal->MouseOrigin;
    if(delta.manhattanLength() < 3)
      {
      this->Internal->MouseOrigin = QPoint();
      }
    this->render();
    }
  else if (e->type() == QEvent::MouseButtonRelease)
    {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton && !this->Internal->MouseOrigin.isNull())
      {
      QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
      QPoint delta = newPos - this->Internal->MouseOrigin;
      if (delta.manhattanLength() < 3 && qobject_cast<QWidget*>(caller))
        {
        QList<QAction*> actions = this->Internal->Viewport->actions();
        if (!actions.isEmpty())
          {
          QMenu* menu = new QMenu(this->Internal->Viewport);
          menu->setAttribute(Qt::WA_DeleteOnClose);
          menu->addActions(actions);
          menu->popup(qobject_cast<QWidget*>(caller)->mapToGlobal(newPos));
          }
        }
      this->Internal->MouseOrigin = QPoint();
      }
    this->render();
    }
  else if (e->type() == QEvent::Resize)
    {
    this->render();
    }
  
  return Superclass::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
bool pqPythonView::canDisplay(pqOutputPort* opPort) const
{
  if(this->Superclass::canDisplay(opPort))
    {
    return true;
    }

  pqPipelineSource* source = opPort? opPort->getSource() :0;
  vtkSMSourceProxy* sourceProxy = source? 
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort|| !source ||
    opPort->getServer()->GetConnectionID() !=
    this->getServer()->GetConnectionID() || !sourceProxy ||
    sourceProxy->GetOutputPortsCreated()==0)
    {
    return false;
    }

  vtkPVXMLElement* hints = sourceProxy->GetHints();
  if (hints)
    {
    // If the source has an hint as follows, then it's a text producer and must
    // be is display-able.
    //  <Hints>
    //    <OutputPort name="..." index="..." type="text" />
    //  </Hints>

    unsigned int numElems = hints->GetNumberOfNestedElements(); 
    for (unsigned int cc=0; cc < numElems; cc++)
      {
      int index;
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      if (child->GetName() &&
        strcmp(child->GetName(), "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) &&
        index == opPort->getPortNumber() &&
        child->GetAttribute("type") &&
        strcmp(child->GetAttribute("type"), "text") == 0)
        {
        return true;
        }
      }
    }
  
  vtkPVDataInformation* dinfo = opPort->getDataInformation();
  if (dinfo->GetDataSetType() == -1)
    {
    return false;
    }


  return true;
}

//-----------------------------------------------------------------------------
vtkImageData* pqPythonView::captureImage(int magnification)
{
  if (this->getWidget()->isVisible())
    {
    return this->getPythonViewProxy()->CaptureWindow(magnification);
    }

  // Don't return any image when the view is not visible.
  return NULL;
}
