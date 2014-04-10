/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowserWidget.h

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
#ifndef __pqPipelineBrowserWidget_h 
#define __pqPipelineBrowserWidget_h

#include "pqFlatTreeView.h"
#include "pqComponentsModule.h"

class pqPipelineModel;
class pqPipelineAnnotationFilterModel;
class pqPipelineSource;
class pqView;
class vtkSession;

/// pqPipelineBrowserWidget is the widget for the pipeline  browser. This is a
/// replacement for pqPipelineBrowser.
class PQCOMPONENTS_EXPORT pqPipelineBrowserWidget : public pqFlatTreeView
{
  Q_OBJECT
  typedef pqFlatTreeView Superclass;
public:
  pqPipelineBrowserWidget(QWidget* parent=0);
  virtual ~pqPipelineBrowserWidget();

  /// Used to monitor the key press events in the tree view.
  /// Returns True if the event should not be sent to the object.
  virtual bool eventFilter(QObject *object, QEvent *e);

  /// Set the visibility of selected items.
  void setSelectionVisibility(bool visible);

  /// Set Annotation filter to use
  void enableAnnotationFilter(const QString& annotationKey);

  /// Disable any Annotation filter
  void disableAnnotationFilter();

  /// Set Session filter to use
  void enableSessionFilter(vtkSession* session);

  /// Disable any Session filter
  void disableSessionFilter();

signals:
  /// Fired when the delete key is pressed.
  /// Typically implies that the selected items need to be deleted.
  void deleteKey();

public slots:
  /// Set the active view. By default connected to
  /// pqActiveObjects::viewChanged() so it keeps track of the active view.
  void setActiveView(pqView*);

protected slots:
  void handleIndexClicked(const QModelIndex& index);
  void expandWithModelIndexTranslation(const QModelIndex &);

protected:
  /// sets the visibility for items in the indices list.
  void setVisibility(bool visible, const QModelIndexList& indices);
  pqPipelineModel* PipelineModel;
  pqPipelineAnnotationFilterModel* FilteredPipelineModel;
  const QModelIndex pipelineModelIndex(const QModelIndex& index) const;
  const pqPipelineModel* getPipelineModel(const QModelIndex& index) const;

private:
  Q_DISABLE_COPY(pqPipelineBrowserWidget)
};

#endif


