/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqColorMapEditor.h"
#include "ui_pqColorMapEditor.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqLookupTableManager.h"
#include "pqProxyWidgetDialog.h"
#include "pqProxyWidget.h"
#include "pqSettings.h"
#include "pqUndoStack.h"
#include "vtkCommand.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkWeakPointer.h"

#include <QDialog>
#include <QKeyEvent>
#include <QPointer>
#include <QVBoxLayout>

namespace
{
  class pqClearTextOnEsc : public QObject
  {
public:
  pqClearTextOnEsc(QLineEdit* parentObject) : QObject(parentObject)
    {
    }
protected:
  virtual bool eventFilter(QObject *obj, QEvent *evt)
    {
    if (evt->type() == QEvent::KeyPress)
      {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(evt);
      if (keyEvent->key() == Qt::Key_Escape)
        {
        qobject_cast<QLineEdit*>(this->parent())->clear();
        }
      }
    return this->QObject::eventFilter(obj, evt);
    }
  };
}

class pqColorMapEditor::pqInternals
{
public:
  Ui::ColorMapEditor Ui;
  QPointer<pqProxyWidget> ProxyWidget;
  QPointer<pqDataRepresentation> ActiveRepresentation;
  unsigned long ObserverId;
  vtkWeakPointer<vtkSMProxy> ScalarBarProxy;

  pqInternals(pqColorMapEditor* self) : ObserverId(0)
    {
    this->Ui.setupUi(self);

    QVBoxLayout* vbox = new QVBoxLayout(this->Ui.PropertiesFrame);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    this->Ui.SearchLineEdit->installEventFilter(
      new pqClearTextOnEsc(this->Ui.SearchLineEdit));
    }

  ~pqInternals()
    {
    }
};

//-----------------------------------------------------------------------------
pqColorMapEditor::pqColorMapEditor(QWidget* parentObject)
  : Superclass(parentObject),
  Internals(new pqColorMapEditor::pqInternals(this))
{
  QObject::connect(this->Internals->Ui.AdvancedButton, SIGNAL(toggled(bool)),
                   this, SLOT(updatePanel()));
  QObject::connect(this->Internals->Ui.SearchLineEdit, SIGNAL(textChanged(QString)),
                   this, SLOT(updatePanel()));
  QObject::connect(this->Internals->Ui.ShowScalarBar, SIGNAL(clicked(bool)),
                   this, SLOT(showScalarBar(bool)));
  QObject::connect(this->Internals->Ui.EditScalarBar, SIGNAL(clicked()),
                   this, SLOT(editScalarBar()));
  QObject::connect(this->Internals->Ui.SaveAsDefault, SIGNAL(clicked()),
                   this, SLOT(saveAsDefault()));
  QObject::connect(this->Internals->Ui.AutoUpdate, SIGNAL(clicked(bool)),
                   this, SLOT(setAutoUpdate(bool)));
  QObject::connect(this->Internals->Ui.Update, SIGNAL(clicked()),
                   this, SLOT(renderViews()));
 
  pqActiveObjects *activeObjects = &pqActiveObjects::instance();
  this->connect(activeObjects, SIGNAL(representationChanged(pqDataRepresentation*)),
    this, SLOT(updateActive()));

  pqSettings *settings = pqApplicationCore::instance()->settings();
  if (settings)
    {
    this->Internals->Ui.AdvancedButton->setChecked(
      settings->value("showAdvancedPropertiesColorMapEditor", false).toBool());
    this->Internals->Ui.AutoUpdate->setChecked( 
      settings->value("autoUpdateColorMapEditor", false).toBool());
    }

  this->updateActive();
}

//-----------------------------------------------------------------------------
pqColorMapEditor::~pqColorMapEditor()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  if (settings)
    {
    // save the state of advanced button in the user config.
    settings->setValue("showAdvancedPropertiesColorMapEditor",
      this->Internals->Ui.AdvancedButton->isChecked());
    settings->setValue("autoUpdateColorMapEditor",
      this->Internals->Ui.AutoUpdate->isChecked());
    }

  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updatePanel()
{
  if (this->Internals->ProxyWidget)
    {
    this->Internals->ProxyWidget->filterWidgets(
      this->Internals->Ui.AdvancedButton->isChecked(),
      this->Internals->Ui.SearchLineEdit->text());
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateActive()
{
  pqDataRepresentation* repr =
    pqActiveObjects::instance().activeRepresentation();
  pqView* view = pqActiveObjects::instance().activeView();

  this->setDataRepresentation(repr);

  // Set the current LUT proxy to edit.
  if (repr && vtkSMPVRepresentationProxy::GetUsingScalarColoring(repr->getProxy()))
    {
    this->setColorTransferFunction(
      vtkSMPropertyHelper(repr->getProxy(), "LookupTable", true).GetAsProxy());
    }
  else
    {
    this->setColorTransferFunction(NULL);
    }

  // check if there's a scalar-bar to show/edit for the current state.
  if (this->Internals->ProxyWidget && view)
    {
    vtkSMProxy* lutProxy = this->Internals->ProxyWidget->proxy();
    vtkSMProxy* scalarBarProxy =
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
        lutProxy, view->getProxy());
    this->setScalarBar(scalarBarProxy, view->getProxy());
    }
  else
    {
    this->setScalarBar(NULL, NULL);
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setDataRepresentation(pqDataRepresentation* repr)
{
  // this method sets up hooks to ensure that when the repr's properties are
  // modified, the editor shows the correct LUT.
  if (this->Internals->ActiveRepresentation == repr)
    {
    return;
    }

  if (this->Internals->ActiveRepresentation)
    {
    // disconnect signals.
    if (this->Internals->ObserverId)
      {
      this->Internals->ActiveRepresentation->getProxy()->RemoveObserver(
        this->Internals->ObserverId);
      }
    }

  this->Internals->ObserverId = 0;
  this->Internals->ActiveRepresentation = repr;
  if (repr && repr->getProxy())
    {
    this->Internals->ObserverId = repr->getProxy()->AddObserver(
      vtkCommand::PropertyModifiedEvent, this, &pqColorMapEditor::updateActive);
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setColorTransferFunction(vtkSMProxy* ctf)
{
  Ui::ColorMapEditor& ui = this->Internals->Ui;

  if (this->Internals->ProxyWidget == NULL && ctf == NULL)
    {
    return;
    }
  if (this->Internals->ProxyWidget && ctf &&
    this->Internals->ProxyWidget->proxy() == ctf)
    {
    return;
    }

  if ( (ctf==NULL && this->Internals->ProxyWidget) ||
       (this->Internals->ProxyWidget && ctf && this->Internals->ProxyWidget->proxy() != ctf))
    {
    ui.PropertiesFrame->layout()->removeWidget(this->Internals->ProxyWidget);
    delete this->Internals->ProxyWidget;
    }

  ui.SaveAsDefault->setEnabled(ctf != NULL);
  if (!ctf)
    {
    return;
    }

  pqProxyWidget* widget = new pqProxyWidget(ctf, this);
  widget->setObjectName("Properties");
  widget->setApplyChangesImmediately(true);
  widget->filterWidgets();

  ui.PropertiesFrame->layout()->addWidget(widget);

  this->Internals->ProxyWidget = widget;
  this->updatePanel();

  QObject::connect(widget, SIGNAL(changeFinished()), this, SLOT(updateIfNeeded()));
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setScalarBar(vtkSMProxy* sb, vtkSMProxy* viewProxy)
{
  this->Internals->ScalarBarProxy = sb;

  Ui::ColorMapEditor& ui = this->Internals->Ui;
  ui.ShowScalarBar->setEnabled(viewProxy != NULL);
  ui.ShowScalarBar->setChecked(sb != NULL &&
    vtkSMPropertyHelper(sb, "Visibility").GetAsInt() != 0);
  ui.EditScalarBar->setEnabled(sb != NULL);
  // ^--- this won't trigger clicked(bool)
  //      and hence this->showScalarBar() won't be called.
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::showScalarBar(bool show_sb)
{
  if (show_sb)
    {
    BEGIN_UNDO_SET("Show scalar bar");
    }
  else
    {
    BEGIN_UNDO_SET("Hide scalar bar");
    }

  if (this->Internals->ScalarBarProxy)
    {
    vtkSMPropertyHelper(this->Internals->ScalarBarProxy,
      "Visibility").Set(0, show_sb? 1 : 0);
    this->Internals->ScalarBarProxy->UpdateVTKObjects();
    }
  else
    {
    // need to create a new scalar bar repr.
    // For now, we'll use pqLookupTableManager. This should be cleaned up to use
    // some ServerManager logic instead.
    pqApplicationCore* core = pqApplicationCore::instance();
    pqLookupTableManager* lut_mgr = core->getLookupTableManager();
    if (lut_mgr)
      {
      lut_mgr->setScalarBarVisibility(
        this->Internals->ActiveRepresentation, show_sb);
      // this will ensure we set the current scalar bar proxy correctly.
      this->updateActive();
      }
    }
  this->renderViews();
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::editScalarBar()
{
  pqProxyWidgetDialog dialog(this->Internals->ScalarBarProxy);
  QObject::connect(&dialog, SIGNAL(accepted()),
    this, SLOT(renderViews()));
  dialog.setWindowTitle("Edit Color Legend Parameters");
  dialog.setObjectName("ColorLegendEditor");
  dialog.exec();
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::renderViews()
{
  if (this->Internals->ActiveRepresentation)
    {
    this->Internals->ActiveRepresentation->renderViewEventually();
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::saveAsDefault()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  if (lut_mgr && this->Internals->ProxyWidget &&
    this->Internals->ProxyWidget->proxy())
    {
    lut_mgr->saveAsDefault(
      this->Internals->ProxyWidget->proxy(),
      pqActiveObjects::instance().activeView()?
      pqActiveObjects::instance().activeView()->getProxy() : NULL);
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setAutoUpdate(bool val)
{
  this->Internals->Ui.AutoUpdate->setChecked(val);
  this->updateIfNeeded();
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateIfNeeded()
{
  if (this->Internals->Ui.AutoUpdate->isChecked())
    {
    this->renderViews();
    }
}
