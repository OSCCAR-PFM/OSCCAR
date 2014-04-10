/*=========================================================================

   Program: ParaView
   Module:    pqProxyInformationWidget.h

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
#ifndef _pqProxyInformationWidget_h
#define _pqProxyInformationWidget_h

#include <QWidget>
#include <QPointer>
#include "pqComponentsModule.h"

class pqOutputPort;
class QTreeWidgetItem;
class vtkEventQtSlotConnect;
class vtkPVDataInformation;

/// Widget which provides information about an output port of a source proxy
class PQCOMPONENTS_EXPORT pqProxyInformationWidget : public QWidget
{
  Q_OBJECT
public:
  /// constructor
  pqProxyInformationWidget(QWidget* p=0);
  /// destructor
  ~pqProxyInformationWidget();

  /// get the proxy for which properties are displayed
  pqOutputPort* getOutputPort();

public slots:
  /// TODO: have this become automatic instead of relying on 
  /// the accept button in case another client modifies the pipeline.
  void updateInformation();

  /// Set the display whose properties we want to edit.
  void setOutputPort(pqOutputPort* outputport);

private slots:
  void onItemClicked(QTreeWidgetItem* item);

private:
  /// builds the composite tree structure.
  QTreeWidgetItem* fillCompositeInformation(vtkPVDataInformation* info,
    QTreeWidgetItem* parent=0);

  void fillDataInformation(vtkPVDataInformation* info);

private:
  QPointer<pqOutputPort> OutputPort;
  vtkEventQtSlotConnect* VTKConnect;
  class pqUi;
  pqUi* Ui;
};

#endif

