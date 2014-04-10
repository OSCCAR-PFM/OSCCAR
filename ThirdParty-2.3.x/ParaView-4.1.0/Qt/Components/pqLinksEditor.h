/*=========================================================================

   Program: ParaView
   Module:    pqLinksEditor.h

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

#ifndef _pqLinksEditor_h
#define _pqLinksEditor_h

#include <QDialog>
#include "pqComponentsModule.h"
#include "ui_pqLinksEditor.h"
#include "pqLinksModel.h"

class pqLinksEditorProxyModel;


/// a Qt dialog for editing a property/proxy/camera link
/// two proxies can be selected, and if property type is
/// selected, then two properties can be selected as well.
class PQCOMPONENTS_EXPORT pqLinksEditor :
  public QDialog, private Ui::pqLinksEditor
{
  Q_OBJECT
  typedef QDialog base;
public:

  /// create a link editor to create/edit a link
  /// initial values are retrieved from the provided vtkSMLink
  pqLinksEditor(vtkSMLink* link, QWidget* p=0);
  /// destroy this dialog
  ~pqLinksEditor();

  /// get the name of the link
  QString linkName();

  /// get the type of link
  pqLinksModel::ItemType linkType();

  /// get the first selected proxy
  vtkSMProxy* selectedProxy1();
  /// get the second selected proxy;
  vtkSMProxy* selectedProxy2();
  
  /// get the first selected property
  QString selectedProperty1();
  /// get the second selected property
  QString selectedProperty2();

private slots:
  void currentProxy1Changed(const QModelIndex& cur, const QModelIndex&);
  void currentProxy2Changed(const QModelIndex& cur, const QModelIndex&);
  
  void currentProperty1Changed(QListWidgetItem* item);
  void currentProperty2Changed(QListWidgetItem* item);
  
  void updateEnabledState();

private:

  void updatePropertyList(QListWidget* tw, vtkSMProxy* proxy);

  pqLinksEditorProxyModel* Proxy1Model;
  pqLinksEditorProxyModel* Proxy2Model;
  
  vtkSMProxy* SelectedProxy1;
  vtkSMProxy* SelectedProxy2;
  QString SelectedProperty1;
  QString SelectedProperty2;

};

#endif

