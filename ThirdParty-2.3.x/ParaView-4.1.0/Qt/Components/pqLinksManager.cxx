/*=========================================================================

   Program: ParaView
   Module:    pqLinksManager.cxx

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

// self
#include "pqLinksManager.h"

// Qt
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

// SM
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

// pqCore
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"

// pqComponents
#include "pqLinksModel.h"
#include "pqLinksEditor.h"

pqLinksManager::pqLinksManager(QWidget* p)
  : QDialog(p)
{
  this->setupUi(this);
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  this->treeView->setModel(model);
  QObject::connect(this->treeView, SIGNAL(clicked(const QModelIndex&)),
                   this, SLOT(selectionChanged(const QModelIndex&)));
  QObject::connect(this->treeView, SIGNAL(activated(const QModelIndex&)),
                   this, SLOT(editLink()));
  QObject::connect(this->addButton, SIGNAL(clicked(bool)),
                   this, SLOT(addLink()));
  QObject::connect(this->editButton, SIGNAL(clicked(bool)),
                   this, SLOT(editLink()));
  QObject::connect(this->removeButton, SIGNAL(clicked(bool)),
                   this, SLOT(removeLink()));
  this->editButton->setEnabled(false);
  this->removeButton->setEnabled(false);
}

pqLinksManager::~pqLinksManager()
{
}

void pqLinksManager::addLink()
{
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  pqLinksEditor editor(NULL, this);
  editor.setWindowTitle("Add Link");
  if(editor.exec() == QDialog::Accepted)
    {
    if(editor.linkType() == pqLinksModel::Proxy)
      {
      vtkSMProxy* inP = editor.selectedProxy1();
      vtkSMProxy* outP = editor.selectedProxy2();

      if(inP->IsA("vtkSMRenderViewProxy") && outP->IsA("vtkSMRenderViewProxy"))
        {
        model->addCameraLink(editor.linkName(), inP, outP);
        }
      else
        {
        model->addProxyLink(editor.linkName(), inP, outP);
        }
      }
    else if(editor.linkType() == pqLinksModel::Property)
      {
      model->addPropertyLink(editor.linkName(),
                                editor.selectedProxy1(),
                                editor.selectedProperty1(),
                                editor.selectedProxy2(),
                                editor.selectedProperty2());
      }
    }
}

void pqLinksManager::editLink()
{
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  QModelIndex idx = this->treeView->selectionModel()->currentIndex();
  vtkSMLink* link = model->getLink(idx);
  pqLinksEditor editor(link, this);
  editor.setWindowTitle("Edit Link");
  if(editor.exec() == QDialog::Accepted)
    {
    model->removeLink(idx);
    
    if(editor.linkType() == pqLinksModel::Proxy)
      {
      vtkSMProxy* inP = editor.selectedProxy1();
      vtkSMProxy* outP = editor.selectedProxy2();

      if(inP->IsA("vtkSMRenderViewProxy") && outP->IsA("vtkSMRenderViewProxy"))
        {
        model->addCameraLink(editor.linkName(), inP, outP);
        }
      else
        {      
        model->addProxyLink(editor.linkName(), inP, outP);
        }
      }
    else if(editor.linkType() == pqLinksModel::Property)
      {
      model->addPropertyLink(editor.linkName(),
                                editor.selectedProxy1(),
                                editor.selectedProperty1(),
                                editor.selectedProxy2(),
                                editor.selectedProperty2());
      }
    }
}

void pqLinksManager::removeLink()
{
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  QModelIndexList idxs = this->treeView->selectionModel()->selectedIndexes();
  QStringList names;
  // convert indexes to names so our indexes don't become invalid during removal
  foreach(QModelIndex idx, idxs)
    {
    QString name = model->getLinkName(idx);
    if(!names.contains(name))
      {
      names.append(name);
      }
    }

  foreach(QString name, names)
    {
    model->removeLink(name);
    }
}

void pqLinksManager::selectionChanged(const QModelIndex& idx)
{
  if(!idx.isValid())
    {
    this->editButton->setEnabled(false);
    this->removeButton->setEnabled(false);
    }
  else
    {
    this->editButton->setEnabled(true);
    this->removeButton->setEnabled(true);
    }
}


