/*=========================================================================

   Program: ParaView
   Module:    pqSGExportStateWizard.cxx

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
#include "pqSGExportStateWizard.h"

#include <pqApplicationCore.h>
#include <pqContextView.h>
#include <pqFileDialog.h>
#include <pqPipelineFilter.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqPythonDialog.h>
#include <pqPythonManager.h>
#include <pqRenderViewBase.h>
#include <pqServerManagerModel.h>
#include <pqSettings.h>
#include "pqImageOutputInfo.h"

#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPNGWriter.h>
#include <vtkPVXMLElement.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>
#include <vtksys/SystemTools.hxx>

#include <QLabel>
#include <QPixmap>
#include <QSize>
#include <QMessageBox>
#include <QPointer>
#include <QRegExp>
#include <QRegExpValidator>

#include <QDebug>

// HACK.
namespace
{
  static QPointer<pqSGExportStateWizard> ActiveWizard;
}

pqSGExportStateWizardPage2::pqSGExportStateWizardPage2(QWidget* _parent)
  : QWizardPage(_parent)
{
  this->Internals = ::ActiveWizard->Internals;
}

pqSGExportStateWizardPage3::pqSGExportStateWizardPage3(QWidget* _parent)
  : QWizardPage(_parent)
{
  this->Internals = ::ActiveWizard->Internals;
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizardPage2::initializePage()
{
  this->Internals->simulationInputs->clear();
  this->Internals->allInputs->clear();
  QList<pqPipelineSource*> sources =
    pqApplicationCore::instance()->getServerManagerModel()->
    findItems<pqPipelineSource*>();
  foreach (pqPipelineSource* source, sources)
    {
    if (qobject_cast<pqPipelineFilter*>(source))
      {
      continue;
      }
    this->Internals->allInputs->addItem(source->getSMName());
    }
}

//-----------------------------------------------------------------------------
bool pqSGExportStateWizardPage2::isComplete() const
{
  return this->Internals->simulationInputs->count() > 0;
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizardPage3::initializePage()
{
  this->Internals->nameWidget->clearContents();
  this->Internals->nameWidget->setRowCount(
    this->Internals->simulationInputs->count());
  for (int cc=0; cc < this->Internals->simulationInputs->count(); cc++)
    {
    QListWidgetItem* item = this->Internals->simulationInputs->item(cc);
    QString text = item->text();
    this->Internals->nameWidget->setItem(cc, 0, new QTableWidgetItem(text));
    // if there is only 1 input then call it input, otherwise
    // use the same name as the filter
    if(this->Internals->simulationInputs->count() == 1)
      {
      this->Internals->nameWidget->setItem(cc, 1, new QTableWidgetItem("input"));
      }
    else
      {
      this->Internals->nameWidget->setItem(cc, 1, new QTableWidgetItem(text));
      }
    QTableWidgetItem* tableItem = this->Internals->nameWidget->item(cc, 1);
    tableItem->setFlags(tableItem->flags()|Qt::ItemIsEditable);

    tableItem = this->Internals->nameWidget->item(cc, 0);
    tableItem->setFlags(tableItem->flags() & ~Qt::ItemIsEditable);
    }
}

//-----------------------------------------------------------------------------
pqSGExportStateWizard::pqSGExportStateWizard(
  QWidget *parentObject, Qt::WindowFlags parentFlags)
: Superclass(parentObject, parentFlags)
{
  this->CurrentView = 0;
  ::ActiveWizard = this;
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
  ::ActiveWizard = NULL;
  //this->setWizardStyle(ModernStyle);
  this->setOption(QWizard::NoCancelButton, false);
  this->Internals->viewsContainer->hide();
  this->Internals->rescaleDataRange->hide();
  this->Internals->previousView->hide();
  this->Internals->nextView->hide();

  QObject::connect(this->Internals->allInputs, SIGNAL(itemSelectionChanged()),
    this, SLOT(updateAddRemoveButton()));
  QObject::connect(this->Internals->simulationInputs, SIGNAL(itemSelectionChanged()),
    this, SLOT(updateAddRemoveButton()));
  QObject::connect(this->Internals->addButton, SIGNAL(clicked()),
    this, SLOT(onAdd()));
  QObject::connect(this->Internals->removeButton, SIGNAL(clicked()),
    this, SLOT(onRemove()));

  QObject::connect(this->Internals->allInputs, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
    this, SLOT(onAdd()));
  QObject::connect(this->Internals->simulationInputs, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
    this, SLOT(onRemove()));

  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->viewsContainer, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->rescaleDataRange, SLOT(setVisible(bool)));

  QObject::connect(this->Internals->nextView, SIGNAL(pressed()),
                   this, SLOT(incrementView()));
  QObject::connect(this->Internals->previousView, SIGNAL(pressed()),
                   this, SLOT(decrementView()));

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqRenderViewBase*> renderViews = smModel->findItems<pqRenderViewBase*>();
  QList<pqContextView*> contextViews = smModel->findItems<pqContextView*>();
  int viewCounter = 0;
  int numberOfViews = renderViews.size() + contextViews.size();
  // first do 2D and 3D render views
  for(QList<pqRenderViewBase*>::Iterator it=renderViews.begin();
      it!=renderViews.end();it++)
    {
    QString viewName = (numberOfViews == 1 ? "image_%t.png" :
                        QString("image_%1_%t.png").arg(viewCounter) );
    pqImageOutputInfo* imageOutputInfo = new pqImageOutputInfo(
      this->Internals->viewsContainer, parentFlags, *it,  viewName);
    this->Internals->viewsContainer->addWidget(imageOutputInfo);
    viewCounter++;
    }
  for(QList<pqContextView*>::Iterator it=contextViews.begin();
      it!=contextViews.end();it++)
    {
    QString viewName = (numberOfViews == 1 ? "image_%t.png" :
                        QString("image_%1_%t.png").arg(viewCounter) );
    pqImageOutputInfo* imageOutputInfo = new pqImageOutputInfo(
      this->Internals->viewsContainer, parentFlags, *it, viewName);
    this->Internals->viewsContainer->addWidget(imageOutputInfo);
    viewCounter++;
    }
  if(numberOfViews > 1)
    {
    this->Internals->nextView->setEnabled(true);
    }
  this->Internals->viewsContainer->setCurrentIndex(0);

  // a bit of a hack but we name the finish button here since for testing
  // it's having a hard time finding that button otherwise.
  QAbstractButton* finishButton = this->button(FinishButton);
  QString name("finishButton");
  finishButton->setObjectName(name);
}

//-----------------------------------------------------------------------------
pqSGExportStateWizard::~pqSGExportStateWizard()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::updateAddRemoveButton()
{
  this->Internals->addButton->setEnabled(
    this->Internals->allInputs->selectedItems().size() > 0);
  this->Internals->removeButton->setEnabled(
    this->Internals->simulationInputs->selectedItems().size() > 0);
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::onAdd()
{
  foreach (QListWidgetItem* item, this->Internals->allInputs->selectedItems())
    {
    QString text = item->text();
    this->Internals->simulationInputs->addItem(text);
    delete this->Internals->allInputs->takeItem(
      this->Internals->allInputs->row(item));
    }
  dynamic_cast<pqSGExportStateWizardPage2*>(
    this->currentPage())->emitCompleteChanged();
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::onRemove()
{
  foreach (QListWidgetItem* item, this->Internals->simulationInputs->selectedItems())
    {
    QString text = item->text();
    this->Internals->allInputs->addItem(text);
    delete this->Internals->simulationInputs->takeItem(
      this->Internals->simulationInputs->row(item));
    }
  dynamic_cast<pqSGExportStateWizardPage2*>(this->currentPage())->emitCompleteChanged();
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::incrementView()
{
  if(this->CurrentView >= this->Internals->viewsContainer->count()-1)
    {
    cerr << "Already on the last view.  Next View button should be disabled.\n";
    this->Internals->nextView->setEnabled(false);
    return;
    }
  if(this->CurrentView == 0)
    {
    this->Internals->previousView->setEnabled(true);
    }
  this->CurrentView++;
  this->Internals->viewsContainer->setCurrentIndex(this->CurrentView);
  if(this->CurrentView >= this->Internals->viewsContainer->count()-1)
    {
    this->Internals->nextView->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::decrementView()
{
  if(this->CurrentView <= 0)
    {
    cerr << "Already on the first view.  Previous View button should be disabled.\n";
    this->Internals->previousView->setEnabled(false);
    return;
    }
  if(this->CurrentView == this->Internals->viewsContainer->count()-1)
    {
    this->Internals->nextView->setEnabled(true);
    }
  this->CurrentView--;
  this->Internals->viewsContainer->setCurrentIndex(this->CurrentView);
  if(this->CurrentView <= 0)
    {
    this->Internals->previousView->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
QList<pqImageOutputInfo*> pqSGExportStateWizard::getImageOutputInfos()
{
  QList<pqImageOutputInfo*> infos;
  for(int i=0;i<this->Internals->viewsContainer->count();i++)
    {
    if( pqImageOutputInfo* qinfo = qobject_cast<pqImageOutputInfo*>(
          this->Internals->viewsContainer->widget(i)) )
      {
      infos.append(qinfo);
      }
    }
  return infos;
}

//-----------------------------------------------------------------------------
bool pqSGExportStateWizard::validateCurrentPage()
{
  if (!this->Superclass::validateCurrentPage())
    {
    return false;
    }

  if (this->nextId() != -1)
    {
    // not yet done with the wizard.
    return true;
    }

  // Last Page, export the state.
  pqPythonManager* manager = qobject_cast<pqPythonManager*>(
    pqApplicationCore::instance()->manager("PYTHON_MANAGER"));

  pqPythonDialog* dialog = 0;
  if (manager)
    {
    dialog = manager->pythonShellDialog();
    }
  if (!dialog)
    {
    qCritical("Failed to locate Python dialog. Cannot save state.");
    return true;
    }

  // Get from the settings whether or not we should save the full state
  // or just non-default values.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if(settings)
    {
    manager->setSaveFullState(settings->value("saveFullState", false).toBool());
    }

  QString command;
  if(this->getCommandString(command))
    {
    dialog->runString(command);
    return true;
    }
  return false;
}
