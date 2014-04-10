/*=========================================================================

   Program: ParaView
   Module:  pqMultiSliceAxisWidget.h

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

========================================================================*/
#ifndef __pqMultiSliceAxisWidget_h
#define __pqMultiSliceAxisWidget_h

#include "pqCoreModule.h"

#include <QWidget>
#include <QPointer>

class vtkContextScene;
class vtkObject;
class QVTKWidget;

class PQCORE_EXPORT pqMultiSliceAxisWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqMultiSliceAxisWidget(QWidget* parent=NULL);
  virtual ~pqMultiSliceAxisWidget();

  /// Set the range of the Axis (Bound)
  void setRange(double min, double max);

  /// Axis::LEFT=0, Axis::BOTTOM, Axis::RIGHT, Axis::TOP
  void setAxisType(int type);

  /// Title that appears inside the view
  QString title()const;
  void setTitle(const QString& title);

  /// Return the Widget that contain the ContextView
  QVTKWidget* getVTKWidget();

  /// Return the locations of the visible slices within the range as well as
  /// the number of values that can be read from the pointer
  const double* getVisibleSlices(int &nbSlices) const;

  /// Update our internal model to reflect the proxy state
  void updateSlices(double* values, bool* visibility, int numberOfValues);

  /// The active size define the number of pixel that are going to be used for
  /// the slider handle.
  void SetActiveSize(int size);

  /// The margin used on the side of the Axis.
  void SetEdgeMargin(int margin);

public slots:
  void renderView();

signals:
  /// Signal emitted when the model has changed internally
  void modelUpdated();

  /// Signal emitted when a mark is clicked. The value is inside the provided range
  void markClicked(int button, int modifier, double value);

protected:
  vtkContextScene* scene() const;

  /// Internal VTK callback used to emit the modelUpdated() signal
  void invalidateCallback(vtkObject*, unsigned long, void*);

  /// Internal VTK callback used to emit markClicked(...) signale
  void onMarkClicked(vtkObject*, unsigned long, void*);

private:
  pqMultiSliceAxisWidget(const pqMultiSliceAxisWidget&); // Not implemented.
  void operator=(const pqMultiSliceAxisWidget&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
