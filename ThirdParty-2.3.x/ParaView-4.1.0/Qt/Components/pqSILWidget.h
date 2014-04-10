/*=========================================================================

   Program: ParaView
   Module:    pqSILWidget.h

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
#ifndef __pqSILWidget_h 
#define __pqSILWidget_h

#include <QTabWidget>
#include <QPointer>
#include <QList>
#include "pqComponentsModule.h"

class pqSILModel;
class pqTreeView;
class pqProxySILModel;

/// pqSILWidget is a QTabWidget that creates tabs with pqTreeView instances
/// showing the top-level categories from the SIL.
class PQCOMPONENTS_EXPORT pqSILWidget : public QWidget
{
  Q_OBJECT

public:
  /// activeCategory is used to mark one of the top-level categories as the
  /// first one to show. This is typically the sub-tree that you want to set on
  /// the property.
  pqSILWidget(const QString& activeCategory, QWidget* parent=0);
  virtual ~pqSILWidget();

  /// Get/Set the SIL model. This is typically a pqSILModel instance.
  void setModel(pqSILModel* silModel);
  pqSILModel* model() const;

  /// Returns the proxy model for the active category.
  pqProxySILModel* activeModel()
    { return this->ActiveModel; }

protected slots:
  void onModelReset();
  void checkSelectedBlocks() { this->toggleSelectedBlocks(true); }
  void uncheckSelectedBlocks() { this->toggleSelectedBlocks(false); }
  void toggleSelectedBlocks(bool checked = false);

protected:
  QTabWidget *TabWidget;
  QPointer<pqSILModel> Model;
  QList<pqTreeView*> Trees;
  pqProxySILModel* ActiveModel;
  QString ActiveCategory;

private:
  Q_DISABLE_COPY(pqSILWidget)
};

#endif


