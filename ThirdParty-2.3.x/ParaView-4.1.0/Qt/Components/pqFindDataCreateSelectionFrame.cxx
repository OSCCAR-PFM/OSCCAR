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
#include "pqFindDataCreateSelectionFrame.h"
#include "ui_pqFindDataCreateSelectionFrame.h"

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPropertiesPanel.h"
#include "pqQueryClauseWidget.h"
#include "pqSelectionManager.h"
#include "vtkDataObject.h"
#include "vtkPVDataInformation.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

#include <QPointer>

namespace
{
  static QMap<int, QIcon> Icons;
  void InitIcons()
    {
    if (Icons.size() == 0)
      {
      Icons[vtkDataObject::POINT] =
        QIcon(":/pqWidgets/Icons/pqPointData16.png");
      Icons[vtkDataObject::CELL] =
        QIcon(":/pqWidgets/Icons/pqCellData16.png");
      Icons[vtkDataObject::VERTEX] =
        QIcon(":/pqWidgets/Icons/pqPointData16.png");
      Icons[vtkDataObject::EDGE] =
        QIcon(":/pqWidgets/Icons/pqEdgeCenterData16.png");
      Icons[vtkDataObject::ROW] =
        QIcon(":/pqWidgets/Icons/pqSpreadsheet16.png");
      }
    }

}

class pqFindDataCreateSelectionFrame::pqInternals
{
public:
  Ui::FindDataCreateSelectionFrame Ui;
  QPointer<pqSelectionManager> SelectionManager;
  bool CreatingSelection;

  //---------------------------------------------------------------------------
  pqInternals(pqFindDataCreateSelectionFrame* self) : CreatingSelection(false)
    {
    this->SelectionManager = qobject_cast<pqSelectionManager*>(
      pqApplicationCore::instance()->manager("SELECTION_MANAGER"));


    this->Ui.setupUi(self);

    this->Ui.verticalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.verticalLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.horizontalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.horizontalLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

    // we don't want the combo-box to follow the actively selected source
    // automatically.
    this->Ui.source->setAutoUpdateIndex(false);

    self->connect(this->Ui.source, SIGNAL(currentIndexChanged(pqOutputPort*)),
      SLOT(setPort(pqOutputPort*)));
    self->connect(this->Ui.selectionType, SIGNAL(currentIndexChanged(int)),
      SLOT(refreshQuery()));
    self->connect(this->Ui.queryClauseWidget, SIGNAL(helpRequested()),
      SIGNAL(helpRequested()));
    self->connect(this->Ui.runQuery, SIGNAL(clicked()), SLOT(runQuery()));

    // when the selection manager reports a new selection, we reset the query
    // created to avoid confusing the user.
    if (this->SelectionManager)
      {
      self->connect(this->SelectionManager,
        SIGNAL(selectionChanged(pqOutputPort*)),
        SLOT(onSelectionChanged(pqOutputPort*)));
      }
    }

  //---------------------------------------------------------------------------
  // based on the current "source", fill up selection type with available
  // options.
  void populateSelectionType()
    {
    bool prev = this->Ui.selectionType->blockSignals(true);
    pqFindDataCreateSelectionFrame::populateSelectionTypeCombo(
      this->Ui.selectionType, this->Ui.source->currentPort());
    this->Ui.selectionType->blockSignals(prev);
    }

  //---------------------------------------------------------------------------
  int selectionType() const
    {
    return this->Ui.selectionType->itemData(
      this->Ui.selectionType->currentIndex()).toInt();
    }

  //---------------------------------------------------------------------------
  void updateClause(bool force_reset = false)
    {
    Ui::FindDataCreateSelectionFrame& ui = this->Ui;

    pqOutputPort* port = ui.source->currentPort();
    ui.queryClauseWidget->setEnabled(port != NULL);
    ui.runQuery->setEnabled(port != NULL);

    bool reset_clause = force_reset;
    if (ui.queryClauseWidget->producer() != port)
      {
      ui.queryClauseWidget->setProducer(port);
      reset_clause = true;
      }

    int attrType = this->selectionType();
    if (ui.queryClauseWidget->attributeType() != attrType)
      {
      ui.queryClauseWidget->setAttributeType(attrType);
      reset_clause = true;
      }
    if (reset_clause && port)
      {
      ui.queryClauseWidget->initialize();
      }
    }
};

//-----------------------------------------------------------------------------
pqFindDataCreateSelectionFrame::pqFindDataCreateSelectionFrame(
  QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags),
  Internals(new pqInternals(this))
{
  // fill the source combo with existing sources, if any.
  this->Internals->Ui.source->fillExistingPorts();
}

//-----------------------------------------------------------------------------
pqFindDataCreateSelectionFrame::~pqFindDataCreateSelectionFrame()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqFindDataCreateSelectionFrame::setPort(pqOutputPort* port)
{
  Ui::FindDataCreateSelectionFrame &ui = this->Internals->Ui;
  if (ui.source->currentPort() != port)
    {
    ui.source->setCurrentPort(port);
    }

  // Update available selection types.
  this->Internals->populateSelectionType();
  this->refreshQuery();
}

//-----------------------------------------------------------------------------
void pqFindDataCreateSelectionFrame::refreshQuery()
{
  this->Internals->updateClause();
}

//-----------------------------------------------------------------------------
void pqFindDataCreateSelectionFrame::runQuery()
{
  Ui::FindDataCreateSelectionFrame &ui = this->Internals->Ui;
  Q_ASSERT(ui.source->currentPort() == ui.queryClauseWidget->producer() &&
    ui.source->currentPort() != NULL);

  pqOutputPort* port = ui.source->currentPort();

  vtkSmartPointer<vtkSMProxy> selectionSource;
  selectionSource.TakeReference(ui.queryClauseWidget->newSelectionSource());
  if (!selectionSource)
    {
    qWarning("Failed to create a selection based on the given criteria.");
    return;
    }

  switch (ui.queryClauseWidget->attributeType())
    {
  case vtkDataObject::CELL:
    vtkSMPropertyHelper(selectionSource, "FieldType").Set(vtkSelectionNode::CELL);
    break;

  case vtkDataObject::POINT:
    vtkSMPropertyHelper(selectionSource, "FieldType").Set(vtkSelectionNode::POINT);
    break;

  case vtkDataObject::EDGE:
    vtkSMPropertyHelper(selectionSource, "FieldType").Set(vtkSelectionNode::EDGE);
    break;

  case vtkDataObject::VERTEX:
    vtkSMPropertyHelper(selectionSource, "FieldType").Set(vtkSelectionNode::VERTEX);
    break;

  case vtkDataObject::ROW:
    vtkSMPropertyHelper(selectionSource, "FieldType").Set(vtkSelectionNode::ROW);
    break;
    }
  selectionSource->UpdateVTKObjects();
  port->setSelectionInput(vtkSMSourceProxy::SafeDownCast(selectionSource), 0);

  // tell the selection manager about this selection , of one exists. The
  // selection manager has logic to clear other selections and such as per the
  // application's requirements.
  if (this->Internals->SelectionManager)
    {
    this->Internals->CreatingSelection = true;
    this->Internals->SelectionManager->select(port);
    this->Internals->CreatingSelection = false;
    }
  port->renderAllViews();
}

//-----------------------------------------------------------------------------
void pqFindDataCreateSelectionFrame::onSelectionChanged(pqOutputPort* port)
{
  if (!this->Internals->CreatingSelection)
    {
    this->setPort(port);
    this->Internals->updateClause(true);
    }
}

//-----------------------------------------------------------------------------
void pqFindDataCreateSelectionFrame::populateSelectionTypeCombo(
  QComboBox* cbox, pqOutputPort* port)
{
  if (port == NULL)
    {
    cbox->clear();
    return;
    }

  InitIcons();

  // preserve the selection type, if possible.
  const QString currentText = cbox->currentText();

  cbox->clear();
  vtkPVDataInformation* dataInfo = port->getDataInformation();

  if (dataInfo->DataSetTypeIsA("vtkGraph"))
    {
    cbox->addItem(Icons[vtkDataObject::VERTEX], "Vertex", vtkDataObject::VERTEX);
    cbox->addItem(Icons[vtkDataObject::EDGE], "Edge(s)",   vtkDataObject::EDGE);
    }
  else if (dataInfo->DataSetTypeIsA("vtkTable"))
    {
    cbox->addItem(Icons[vtkDataObject::ROW], "Row(s)", vtkDataObject::ROW);
    }
  else
    {
    cbox->addItem(Icons[vtkDataObject::CELL], "Cell(s)",  vtkDataObject::CELL);
    cbox->addItem(Icons[vtkDataObject::POINT], "Point(s)", vtkDataObject::POINT);
    }

  int index = cbox->findText(currentText);
  if (index != -1)
    {
    cbox->setCurrentIndex(index);
    }
}
