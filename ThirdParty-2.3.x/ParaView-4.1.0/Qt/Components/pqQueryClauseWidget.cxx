/*=========================================================================

   Program: ParaView
   Module:    pqQueryClauseWidget.cxx

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
#include "pqQueryClauseWidget.h"
#include "ui_pqQueryClauseWidget.h"
#include "ui_pqQueryCompositeTreeDialog.h"

#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqHelpWindow.h"
#include "vtkDataObject.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkQuerySelectionSource.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelectionNode.h"

#include <QMap>

// BUG #13806, remove collective operations temporarily since they don't work
// for composite datasets (esp. in parallel) as expected.
#define REMOVE_COLLECTIVE_CLAUSES

class pqQueryClauseWidget::pqInternals : public Ui::pqQueryClauseWidget
{
public:
  struct ArrayInfo
    {
    QString ArrayName;
    int ComponentNo;
    ArrayInfo(const QString& name, int comp)
      : ArrayName(name), ComponentNo(comp)
      {
      }
    ArrayInfo() : ComponentNo(0) {}
    };

  // key == index in the combo-box
  // value == array params.
  QMap<int, ArrayInfo> Arrays;
};



//-----------------------------------------------------------------------------
pqQueryClauseWidget::pqQueryClauseWidget(
  QWidget* parentObject, Qt::WindowFlags _flags)
  : Superclass(parentObject, _flags)
{
  this->AsQualifier = false;
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  QObject::connect(this->Internals->showCompositeTree, SIGNAL(clicked()),
    this, SLOT(showCompositeTree()));

  if (qobject_cast<pqQueryClauseWidget*>(parentObject))
    {
    // don't show the separator line for sub-clauses.
    this->Internals->line->hide();
    }
}

//-----------------------------------------------------------------------------
pqQueryClauseWidget::~pqQueryClauseWidget()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation*
pqQueryClauseWidget::getChosenAttributeInfo() const
{
  vtkPVDataInformation* dataInfo = this->producer()->getDataInformation();
  return dataInfo->GetAttributeInformation(this->attributeType());
}

//-----------------------------------------------------------------------------
void pqQueryClauseWidget::initialize(
  pqQueryClauseWidget::CriteriaTypes type_flags, bool qualifier_mode)
{
  this->AsQualifier = qualifier_mode;
  this->populateSelectionCriteria(type_flags);
  this->populateSelectionCondition();
  this->updateValueWidget();

  this->updateDependentClauseWidgets();

  QObject::connect(this->Internals->criteria,
    SIGNAL(currentIndexChanged(int)),
    this, SLOT(populateSelectionCondition()));
  QObject::connect(this->Internals->criteria,
    SIGNAL(currentIndexChanged(int)),
    this, SLOT(updateDependentClauseWidgets()));
  QObject::connect(this->Internals->condition,
    SIGNAL(currentIndexChanged(int)),
    this, SLOT(updateValueWidget()));
  QObject::connect(this->Internals->helpButton,
                   SIGNAL(clicked()),
                   this,
                   SIGNAL(helpRequested()));
}

//-----------------------------------------------------------------------------
pqQueryClauseWidget::CriteriaType
pqQueryClauseWidget::currentCriteriaType() const
{
  int criteria = this->Internals->criteria->currentIndex();
  if (criteria == -1)
    {
    return INVALID;
    }

  return static_cast<CriteriaType>(
    this->Internals->criteria->itemData(criteria).toInt());
}

//-----------------------------------------------------------------------------
pqQueryClauseWidget::ConditionMode
pqQueryClauseWidget::currentConditionType() const
{
  int condition = this->Internals->condition->currentIndex();
  if (condition == -1)
    {
    return SINGLE_VALUE;
    }

  return static_cast<ConditionMode>(
    this->Internals->condition->itemData(condition).toInt());
}

//-----------------------------------------------------------------------------
void pqQueryClauseWidget::populateSelectionCriteria(
  pqQueryClauseWidget::CriteriaTypes type_flags)
{
  this->Internals->criteria->clear();
  this->Internals->Arrays.clear();

  if (type_flags & INDEX)
    {
    this->Internals->criteria->addItem("ID", INDEX);
    }

  vtkPVDataSetAttributesInformation* attrInfo = this->getChosenAttributeInfo();
  if (type_flags & GLOBALID)
    {
    // Do we have global ids?
    if (attrInfo->GetAttributeInformation(vtkDataSetAttributes::GLOBALIDS))
      {
      this->Internals->criteria->addItem("Global ID", GLOBALID);
      this->Internals->criteria->setCurrentIndex(
            this->Internals->criteria->count()-1);
      }
    }

  if (type_flags & THRESHOLD)
    {
    // Now add the attribute arrays.
    for (int cc=0; cc < attrInfo->GetNumberOfArrays(); cc++)
      {
      vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(cc);
      if (arrayInfo->GetNumberOfComponents() > 1)
        {
        this->Internals->criteria->addItem(
              QString("%1 (Magnitude)").arg(arrayInfo->GetName()),
              THRESHOLD);

        int item_index = (this->Internals->criteria->count()-1);
        this->Internals->Arrays.insert(item_index,
                                       pqInternals::ArrayInfo(arrayInfo->GetName(), -1));

        for (int kk=0; kk < arrayInfo->GetNumberOfComponents(); kk++)
          {
          this->Internals->criteria->addItem(
                QString("%1 (%2)").arg(arrayInfo->GetName()).arg(kk),
                THRESHOLD);
          item_index = (this->Internals->criteria->count()-1);
          this->Internals->Arrays.insert(item_index,
                                         pqInternals::ArrayInfo(arrayInfo->GetName(), kk));
          }
        }
      else
        {
        this->Internals->criteria->addItem(arrayInfo->GetName(),
                                           THRESHOLD);
        int item_index = (this->Internals->criteria->count()-1);
        this->Internals->Arrays.insert(item_index,
                                       pqInternals::ArrayInfo(arrayInfo->GetName(), 0));
        }
      }
    }

  vtkPVDataInformation* dataInfo = this->producer()->getDataInformation();

  if(type_flags & QUERY)
    {
    this->Internals->criteria->addItem("Query", QUERY);
    }

  if (dataInfo->GetCompositeDataSetType() == VTK_MULTIBLOCK_DATA_SET)
    {
    if (type_flags & BLOCK)
      {
      this->Internals->criteria->addItem("Block ID", BLOCK);
      }
    }
  else if (dataInfo->GetCompositeDataSetType() == VTK_HIERARCHICAL_BOX_DATA_SET)
    {
    if (type_flags & AMR_LEVEL)
      {
      this->Internals->criteria->addItem("AMR Level", AMR_LEVEL);
      }
    if (type_flags & AMR_BLOCK)
      {
      this->Internals->criteria->addItem("AMR Block", AMR_BLOCK);
      }
    }

  if (type_flags & PROCESSID)
    {
    pqServer* server = this->producer()->getServer();
    if (server->getNumberOfPartitions() > 1)
      {
      this->Internals->criteria->addItem("Process ID", -1);
      }
    }
}

//-----------------------------------------------------------------------------
void pqQueryClauseWidget::populateSelectionCondition()
{
  this->Internals->condition->clear();

  CriteriaType criteria_type = this->currentCriteriaType(); 
  if (criteria_type == INVALID)
    {
    return;
    }

  switch (criteria_type)
    {
  case QUERY:
    this->Internals->condition->addItem("is", pqQueryClauseWidget::SINGLE_VALUE);
    break;
  case INDEX:
  case GLOBALID:
  case THRESHOLD:
  case PROCESSID:
    this->Internals->condition->addItem("is", pqQueryClauseWidget::SINGLE_VALUE);
    this->Internals->condition->addItem("is between",
      pqQueryClauseWidget::PAIR_OF_VALUES);
    this->Internals->condition->addItem("is one of",
      pqQueryClauseWidget::LIST_OF_VALUES);
    this->Internals->condition->addItem("is >=",
      pqQueryClauseWidget::SINGLE_VALUE_GE);
    this->Internals->condition->addItem("is <=",
      pqQueryClauseWidget::SINGLE_VALUE_LE);
#ifndef REMOVE_COLLECTIVE_CLAUSES
    this->Internals->condition->addItem("is min",
      pqQueryClauseWidget::SINGLE_VALUE_MIN);
    this->Internals->condition->addItem("is max",
      pqQueryClauseWidget::SINGLE_VALUE_MAX);
    this->Internals->condition->addItem("is less than mean",
      pqQueryClauseWidget::SINGLE_VALUE_LE_MEAN);
    this->Internals->condition->addItem("is greater than mean",
      pqQueryClauseWidget::SINGLE_VALUE_GE_MEAN);
    this->Internals->condition->addItem("is equal to mean with tolerance",
      pqQueryClauseWidget::SINGLE_VALUE_MEAN_WITH_TOLERANCE);
#endif
    break;

  case BLOCK:
    this->Internals->condition->addItem("is",
      pqQueryClauseWidget::BLOCK_ID_VALUE);
    if (!this->AsQualifier)
      {
      this->Internals->condition->addItem("is one of",
        pqQueryClauseWidget::LIST_OF_BLOCK_ID_VALUES);
      }
    break;

  case AMR_LEVEL:
    this->Internals->condition->addItem("is",
      pqQueryClauseWidget::AMR_LEVEL_VALUE);
    break;

  case AMR_BLOCK:
    this->Internals->condition->addItem("is",
      pqQueryClauseWidget::AMR_BLOCK_VALUE);
    break;

  case ANY:
  case INVALID:
    break;
    }
}

//-----------------------------------------------------------------------------
void pqQueryClauseWidget::updateValueWidget()
{
  switch (this->currentConditionType())
    {
  case SINGLE_VALUE_MIN:
  case SINGLE_VALUE_MAX:
  case SINGLE_VALUE_LE_MEAN:
  case SINGLE_VALUE_GE_MEAN:
    this->Internals->valueStackedWidget->setCurrentIndex(4);
    break;
  case SINGLE_VALUE_MEAN_WITH_TOLERANCE:
  case SINGLE_VALUE:
  case SINGLE_VALUE_LE:
  case SINGLE_VALUE_GE:
  case LIST_OF_VALUES:
    this->Internals->valueStackedWidget->setCurrentIndex(0);
    break;

  case PAIR_OF_VALUES:
    this->Internals->valueStackedWidget->setCurrentIndex(1);
    break;

  case TRIPLET_OF_VALUES:
    this->Internals->valueStackedWidget->setCurrentIndex(2);
    break;

  case BLOCK_ID_VALUE:
  case AMR_LEVEL_VALUE:
  case AMR_BLOCK_VALUE:
  case LIST_OF_BLOCK_ID_VALUES:
    this->Internals->valueStackedWidget->setCurrentIndex(3);
    break;

  case BLOCK_NAME_VALUE:
    this->Internals->valueStackedWidget->setCurrentIndex(3);
    break;
    }
}

//-----------------------------------------------------------------------------
void pqQueryClauseWidget::updateDependentClauseWidgets()
{
  if (qobject_cast<pqQueryClauseWidget*>(this->parentWidget()))
    {
    return;
    }

  CriteriaType criteria_type = this->currentCriteriaType(); 
  if (criteria_type == INVALID)
    {
    return;
    }

  foreach (pqQueryClauseWidget* child,
    this->findChildren<pqQueryClauseWidget*>())
    {
    delete child;
    }

  pqServer* server = this->producer()->getServer();
  bool multi_process = (server->getNumberOfPartitions() > 1);
  bool multi_block = false;
  bool amr = false;

  vtkPVDataInformation* dataInfo = this->producer()->getDataInformation();
  if (dataInfo->GetCompositeDataSetType() == VTK_MULTIBLOCK_DATA_SET)
    {
    multi_block = true;
    }
  else if (dataInfo->GetCompositeDataSetType() == VTK_HIERARCHICAL_BOX_DATA_SET)
    {
    amr = true;
    }

  QVBoxLayout* vbox = qobject_cast<QVBoxLayout*>(this->layout());

  QList<CriteriaTypes> sub_widgets;

  if (multi_block)
    {
    switch (criteria_type)
      {
    case INDEX:
    case QUERY:
    case THRESHOLD:
      sub_widgets.push_back(BLOCK);
      break;
    case GLOBALID:
    default:
      break;
      }
    }

  if (amr)
    {
    switch (criteria_type)
      {
    case INDEX:
    case THRESHOLD:
      sub_widgets.push_back(AMR_LEVEL);
      sub_widgets.push_back(AMR_BLOCK);
      break;
    case GLOBALID:
      // for now, when selecting Global ids, we don't allow the users to pick the
      // block to extract the array from. We can support this if needed in
      // future.
    case AMR_LEVEL:
      sub_widgets.push_back(AMR_BLOCK);
      break;
      
    case AMR_BLOCK:
      sub_widgets.push_back(AMR_LEVEL);
      break;

    default:
      break;
      }
    }

  if (multi_process)
    {
    sub_widgets.push_back(PROCESSID);
    }

  foreach (CriteriaTypes t_flag, sub_widgets)
    {
    pqQueryClauseWidget* sub_widget = new pqQueryClauseWidget(this);
    sub_widget->Internals->helpButton->hide();
    sub_widget->setProducer(this->producer());
    sub_widget->setAttributeType(this->attributeType());
    sub_widget->initialize(t_flag, true);
    vbox->addWidget(sub_widget);
    }

  if(criteria_type == QUERY)
    {
    this->Internals->value->setText(this->LastQuery);
    }
  else
    {
    this->Internals->value->setText("");
    }
}

//-----------------------------------------------------------------------------
void pqQueryClauseWidget::showCompositeTree()
{
  CriteriaType criteria_type = this->currentCriteriaType(); 
  if (criteria_type == INVALID)
    {
    return;
    }

  QDialog dialog(this);
  Ui::pqQueryCompositeTreeDialog ui;
  ui.setupUi(&dialog);

  if (this->currentConditionType() == LIST_OF_BLOCK_ID_VALUES)
    {
    // allow multiple selections.
    ui.Blocks->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }

  pqSignalAdaptorCompositeTreeWidget adaptor(ui.Blocks,
    this->producer()->getOutputPortProxy(),
    vtkSMCompositeTreeDomain::NONE);
  if (dialog.exec() != QDialog::Accepted)
    {
    return;
    }

  QStringList values;
  QList<QTreeWidgetItem*> selItems = ui.Blocks->selectedItems();
  foreach (QTreeWidgetItem* item, selItems)
    {
    int current_flat_index = adaptor.flatIndex(item);
    switch (criteria_type)
      {
    case BLOCK:
      if (this->Internals->criteria->currentText() == "Block ID")
        {
        values << QString("%1").arg(current_flat_index);
        }
      else
        {
        // name.
        QString blockName = adaptor.blockName(item);
        if (blockName.isEmpty())
          {
          qWarning("Data block doesn't have a name assigned to it. Query may"
            " not work. Use 'Block ID' based criteria instead.");
          }
        else
          {
          values << blockName;
          }
        }
      break;

    case AMR_LEVEL:
      values << QString("%1").arg(adaptor.hierarchicalLevel(item));
      break;

    case AMR_BLOCK:
      values << QString("%1").arg(adaptor.hierarchicalBlockIndex(item));
      break;

    default:
      qCritical("Invalid criteria_type.");
      }
    }
  this->Internals->value_block->setText(values.join(","));
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqQueryClauseWidget::newSelectionSource()
{
  // Find the proper Proxy manager
  vtkSMSessionProxyManager* pxm =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  // Create a new selection source proxy based on the criteria_type.
  vtkSMProxy* selSource = pxm->NewProxy("sources",
      "SelectionQuerySource");

  // * Determine FieldType.
  int field_type = 0;
  switch (this->attributeType())
    {
  case vtkDataObject::POINT:
    field_type = vtkSelectionNode::POINT;
    break;

  case vtkDataObject::CELL:
    field_type = vtkSelectionNode::CELL;
    break;

  case vtkDataObject::ROW:
    field_type = vtkSelectionNode::ROW;
    break;

  case vtkDataObject::VERTEX:
    field_type = vtkSelectionNode::VERTEX;
    break;

  case vtkDataObject::EDGE:
    field_type = vtkSelectionNode::EDGE;
    break;
    }
  vtkSMPropertyHelper(selSource, "FieldType").Set(field_type);

  // Pass on qualifiers and values from this and sub widgets.
  this->addSelectionQualifiers(selSource);
  foreach (pqQueryClauseWidget* child, 
    this->findChildren<pqQueryClauseWidget*>())
    {
    child->addSelectionQualifiers(selSource);
    }

  selSource->UpdateVTKObjects();

  return selSource;
}

//-----------------------------------------------------------------------------
void pqQueryClauseWidget::addSelectionQualifiers(vtkSMProxy* selSource)
{
  CriteriaType criteria_type = this->currentCriteriaType(); 
  if (criteria_type == INVALID)
    {
    return;
    }

  // Variables used to build Query
  QString query;
  QString fieldName;

  // Determine the name of the field
  bool ok_no_value = false;
  switch(criteria_type)
    {
  case INVALID:
  case BLOCK:
  case AMR_LEVEL:
  case AMR_BLOCK:
  case PROCESSID:
  case QUERY:
  case ANY:
    // Options not supported
    break;
  case INDEX:
    fieldName = "id";
    break;
  case GLOBALID:
    fieldName =
        this->getChosenAttributeInfo()->GetAttributeInformation(
          vtkDataSetAttributes::GLOBALIDS)->GetName();
    break;
  case THRESHOLD:
    pqInternals::ArrayInfo info = this->Internals->Arrays[
        this->Internals->criteria->currentIndex()];
    if(info.ComponentNo == -1)
      {
      // Magnitude
      fieldName.append("mag(").append(info.ArrayName).append(")");
      }
    else
      {
      fieldName.append(info.ArrayName)
          .append("[:,").append(QString::number(info.ComponentNo)).append("]");
      }
    break;
    }

  ConditionMode condition_type = this->currentConditionType();
  QList<QVariant> values;
  switch (condition_type)
    {
  case SINGLE_VALUE:
    if(query.isEmpty()) query = "%1 == %2";
  case SINGLE_VALUE_LE:
    if(query.isEmpty()) query = "%1 <= %2";
  case SINGLE_VALUE_GE:
    if(query.isEmpty()) query = "%1 >= %2";
    if (!this->Internals->value->text().isEmpty())
      {
      values << this->Internals->value->text();
      query = query.arg(fieldName, this->Internals->value->text());
      }
    break;
  case LIST_OF_VALUES:
    if(query.isEmpty()) query = "contains(%1,[%2])";
    if (!this->Internals->value->text().isEmpty())
      {
      query = query.arg(fieldName, this->Internals->value->text());
      QStringList parts = this->Internals->value->text().split(',',
        QString::SkipEmptyParts);
      foreach (QString part, parts)
        {
        values << part;
        }
      }
    break;

  case PAIR_OF_VALUES:
    if(query.isEmpty()) query = "(%1 > %2) & (%1 < %3)";
    if (!this->Internals->value_min->text().isEmpty() &&
      !this->Internals->value_max->text().isEmpty())
      {
      values << this->Internals->value_min->text();
      values << this->Internals->value_max->text();
      query = query.arg(fieldName, this->Internals->value_min->text(), this->Internals->value_max->text());
      }
    break;

  case TRIPLET_OF_VALUES:
    // FIXME don't really work but we don't care as we removed that case in the possibility
    if(query.isEmpty()) query = "[(tuple[0,0] & tuple[0,1] & tuple[0,2] ) for tuple in (abs(Points - [%1,%2,%3]) < 1e-6)]";
    if (!this->Internals->value_x->text().isEmpty() &&
      !this->Internals->value_y->text().isEmpty() &&
      !this->Internals->value_z->text().isEmpty())
      {
      values << this->Internals->value_x->text();
      values << this->Internals->value_y->text();
      values << this->Internals->value_z->text();
      query = query.arg( this->Internals->value_x->text(),
                         this->Internals->value_y->text(),
                         this->Internals->value_z->text());
      }
    break;

  case BLOCK_ID_VALUE:
  case LIST_OF_BLOCK_ID_VALUES:
  case BLOCK_NAME_VALUE:
  case AMR_LEVEL_VALUE:
  case AMR_BLOCK_VALUE:
    if(query.isEmpty()) query = "contains(%1,[%2])";
    if (!this->Internals->value_block->text().isEmpty())
      {
      query = query.arg(fieldName, this->Internals->value_block->text());
      if (this->AsQualifier)
        {
        values << this->Internals->value_block->text();
        }
      else
        {
        QStringList parts = this->Internals->value_block->text().split(',',
          QString::SkipEmptyParts);
        foreach (QString part, parts)
          {
          values << part;
          }
        }
      }
    break;
  case SINGLE_VALUE_MIN:
    if(query.isEmpty()) query = "%1  == global_min(%1)";
    query = query.arg(fieldName);
    ok_no_value = true;
    break;
  case SINGLE_VALUE_MAX:
    if(query.isEmpty()) query = "%1  == global_max(%1)";
    query = query.arg(fieldName);
    ok_no_value = true;
    break;
  case SINGLE_VALUE_LE_MEAN:
    if(query.isEmpty()) query = "%1  <= global_mean(%1)";
    query = query.arg(fieldName);
    ok_no_value = true;
    break;
  case SINGLE_VALUE_GE_MEAN:
    if(query.isEmpty()) query = "%1  >= global_mean(%1)";
    query = query.arg(fieldName);
    ok_no_value = true;
    break;
  case SINGLE_VALUE_MEAN_WITH_TOLERANCE:
    if(query.isEmpty()) query = "abs(%1 - global_mean(%1)) < %2";
    if (!this->Internals->value->text().isEmpty())
      {
      values << this->Internals->value->text();
      query = query.arg(fieldName, this->Internals->value->text());
      }
    break;
  default: break;
    }

  if (values.size() == 0 && !ok_no_value)
    {
    return;
    }

  switch (criteria_type)
    {
  case QUERY:
      vtkSMPropertyHelper(selSource, "QueryString").Set(values[0].toString().toAscii().constData());
      break;

  case BLOCK:
    if (this->AsQualifier)
      {
      vtkSMPropertyHelper(selSource,
        "CompositeIndex").Set(values[0].toInt());
      break;
      }
    // break; -- don't break

  case INDEX:
  case GLOBALID:
  case THRESHOLD:
    this->LastQuery = query;
    vtkSMPropertyHelper(selSource, "QueryString").Set(query.toAscii().constData());
    break;
  case AMR_LEVEL:
      {
      vtkSMPropertyHelper(selSource,
        "HierarchicalLevel").Set(values[0].toInt());
      }
    break;

  case AMR_BLOCK:
      {
      vtkSMPropertyHelper(selSource,
        "HierarchicalIndex").Set(values[0].toInt());
      }
    break;


  case PROCESSID:
      {
      vtkSMPropertyHelper(selSource, "ProcessID").Set(values[0].toInt());
      }
    break;

  case INVALID:
  case ANY:
    break;
    }
}
