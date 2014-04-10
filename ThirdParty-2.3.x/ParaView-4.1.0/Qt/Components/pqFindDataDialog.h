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
#ifndef __pqFindDataDialog_h
#define __pqFindDataDialog_h

#include <QDialog>
#include "pqComponentsModule.h"

class pqOutputPort;

/// pqFindDataDialog encapsulates the logic for the "Find Data" dialog in
/// ParaView. This class puts together components provided by other
/// classes e.g. pqFindDataCreateSelectionFrame and
/// pqFindDataCurrentSelectionFrame. 
class PQCOMPONENTS_EXPORT pqFindDataDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;
public:
  pqFindDataDialog(QWidget* parent=0, Qt::WindowFlags flags=0);
  virtual ~pqFindDataDialog();

signals:
  /// triggered to request help about the pqFindDataDialog.
  void helpRequested();

private slots:
  /// called when pqFindDataCurrentSelectionFrame notifies the dialog that it's
  /// showing a new selection. We update the UI accordingly.
  void showing(pqOutputPort*);

  void freezeSelection();
  void extractSelection();
  void extractSelectionOverTime();

private:
  Q_DISABLE_COPY(pqFindDataDialog)

  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
