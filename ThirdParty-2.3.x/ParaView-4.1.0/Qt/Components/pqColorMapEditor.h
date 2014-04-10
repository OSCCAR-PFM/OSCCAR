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
#ifndef __pqColorMapEditor_h
#define __pqColorMapEditor_h

#include "pqComponentsModule.h"
#include <QWidget>

class vtkSMProxy;
class pqDataRepresentation;

/// pqColorMapEditor is a widget that can be used to edit the active color-map,
/// if any. The panel is implemented as an auto-generated panel (similar to the
/// Properties panel) that shows the properties on the lookup-table proxy.
/// Custom widgets such as pqColorOpacityEditorWidget,
/// pqColorAnnotationsPropertyWidget, and others are used to
/// control certain properties on the proxy.
class PQCOMPONENTS_EXPORT pqColorMapEditor : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  pqColorMapEditor(QWidget* parent=0);
  virtual ~pqColorMapEditor();

protected slots:
  /// slot called to update the currently showing proxies.
  void updateActive();

  /// slot called to update the visible widgets.
  void updatePanel();

  /// render's view when transfer function is modified.
  void renderViews();

  /// called when the "ShowScalarBar"  button is clicked.
  void showScalarBar(bool);

  /// Pops up the scalar bar edit widget.
  void editScalarBar();

  /// Save the current transfer function(s) as default.
  void saveAsDefault();

  /// called when AutoUpdate button is toggled.
  void setAutoUpdate(bool);

  void updateIfNeeded();
protected:
  void setDataRepresentation(pqDataRepresentation* repr);
  void setColorTransferFunction(vtkSMProxy* ctf);
  void setScalarBar(vtkSMProxy* sb, vtkSMProxy* viewProxy);

private:
  Q_DISABLE_COPY(pqColorMapEditor)
  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
