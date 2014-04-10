/*=========================================================================

   Program: ParaView
   Module:  pqMultiSliceAxisWidget.cxx

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
#include "pqMultiSliceAxisWidget.h"

// PV includes
#include "QVTKWidget.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMProperty.h"

// VTK includes
#include "vtkAxis.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkMultiSliceContextItem.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"

// Qt includes
#include <QMouseEvent>
#include <QVBoxLayout>

//----------------------------------------------------------------------------
class pqMultiSliceAxisWidget::pqInternal
{

public:
  pqInternal(
    pqMultiSliceAxisWidget& object):Widget_ptr(&object)
    {
    this->View = new QVTKWidget(Widget_ptr);
    this->Range[0] = -10.;
    this->Range[1] = +10.;

    this->SliceItem->AddObserver(vtkCommand::EndInteractionEvent,
                                 this->Widget_ptr,
                                 &pqMultiSliceAxisWidget::onMarkClicked);
    }

  ~pqInternal()
    {
    }

  void init()
    {
    this->ContextView->SetInteractor(this->View->GetInteractor());
    this->View->SetRenderWindow(this->ContextView->GetRenderWindow());
#ifdef Q_WS_WIN
    this->ContextView->GetRenderWindow()->SetLineSmoothing(true);
#endif
    this->View->setAutomaticImageCacheEnabled(true);

    this->ContextView->GetScene()->AddItem(this->SliceItem.GetPointer());

    this->SliceItem->GetAxis()->SetPoint1(10, 10);
    this->SliceItem->GetAxis()->SetPoint2(390, 10);
    this->SliceItem->GetAxis()->SetRange(-10, 10);
    this->SliceItem->GetAxis()->SetPosition(vtkAxis::TOP);
    this->SliceItem->GetAxis()->SetTitle("Default title");
    this->SliceItem->GetAxis()->Update();
    }

  vtkNew<vtkContextView>           ContextView;
  vtkNew<vtkMultiSliceContextItem> SliceItem;
  QPointer<QVTKWidget>             View;
  double                           Range[2];
  pqMultiSliceAxisWidget*          Widget_ptr;
};

//-----------------------------------------------------------------------------
pqMultiSliceAxisWidget::pqMultiSliceAxisWidget(
                             QWidget* parentW/*=NULL*/):Superclass(parentW)
{
  this->Internal = new pqMultiSliceAxisWidget::pqInternal(*this);
  this->Internal->init();
  QVBoxLayout* vLayout = new QVBoxLayout(this);
  vLayout->setMargin(0);
  vLayout->addWidget(this->Internal->View);

  this->Internal->SliceItem->AddObserver(
        vtkCommand::ModifiedEvent, this,
        &pqMultiSliceAxisWidget::invalidateCallback);
}

//-----------------------------------------------------------------------------
pqMultiSliceAxisWidget::~pqMultiSliceAxisWidget()
{
  // Don't need to remove observer as we are looking at an internal field that
  // will be deleted in the same time as us.

  // remove internal data structure
  if(this->Internal)
    {
    delete this->Internal;
    }
}

// ----------------------------------------------------------------------------
QVTKWidget* pqMultiSliceAxisWidget::getVTKWidget()
{
  return this->Internal->View;
}

// ----------------------------------------------------------------------------
void pqMultiSliceAxisWidget::setTitle(const QString& newTitle)
{
  this->Internal->SliceItem->GetAxis()->SetTitle(newTitle.toLatin1().data());
}

// ----------------------------------------------------------------------------
QString pqMultiSliceAxisWidget::title()const
{
  return QString(this->Internal->SliceItem->GetAxis()->GetTitle());
}

// ----------------------------------------------------------------------------
vtkContextScene* pqMultiSliceAxisWidget::scene()const
{
  return this->Internal->ContextView->GetScene();
}

// ----------------------------------------------------------------------------
void pqMultiSliceAxisWidget::renderView()
{
  // bug 0013947
  // on Mac OSX don't render into invalid drawable, all subsequent
  // OpenGL calls fail with invalid framebuffer operation.
  vtkRenderWindow *renWin = this->Internal->View->GetRenderWindow();
  if (!renWin->IsDrawable())
    {
    return;
    }

  this->Internal->View->GetRenderWindow()->Render();
}

// ----------------------------------------------------------------------------
void pqMultiSliceAxisWidget::setAxisType(int type)
{
  this->Internal->SliceItem->GetAxis()->SetPosition(type);
  this->Internal->SliceItem->GetAxis()->Update();
}

// ----------------------------------------------------------------------------
void pqMultiSliceAxisWidget::setRange(double min, double max)
{
  this->Internal->SliceItem->GetAxis()->SetRange(min, max);
  this->Internal->SliceItem->GetAxis()->Update();
}

// ----------------------------------------------------------------------------
void pqMultiSliceAxisWidget::invalidateCallback(vtkObject*, unsigned long, void*)
{
  emit this->modelUpdated();
}

// ----------------------------------------------------------------------------
const double* pqMultiSliceAxisWidget::getVisibleSlices(int &nbSlices) const
{
  return this->Internal->SliceItem->GetVisibleSlices(nbSlices);
}

// ----------------------------------------------------------------------------
void pqMultiSliceAxisWidget::SetActiveSize(int value)
{
  this->Internal->SliceItem->SetActiveSize(value);
}

// ----------------------------------------------------------------------------
void pqMultiSliceAxisWidget::SetEdgeMargin(int margin)
{
  this->Internal->SliceItem->SetEdgeMargin(margin);
}

// ----------------------------------------------------------------------------
void pqMultiSliceAxisWidget::updateSlices( double* values, bool* visibility,
                                           int numberOfValues)
{
  this->Internal->SliceItem->SetSlices(values, visibility, numberOfValues);
}

// ----------------------------------------------------------------------------
void pqMultiSliceAxisWidget::onMarkClicked(vtkObject* src, unsigned long eventId, void* dataArray)
{
  vtkMultiSliceContextItem* item = vtkMultiSliceContextItem::SafeDownCast(src);
  if(item && eventId == vtkCommand::EndInteractionEvent)
    {
    int *array = reinterpret_cast<int*>(dataArray);
    int button   = array[0];
    int modifier = array[1];
    double value = item->GetSliceValue(array[2]);
    emit markClicked(button, modifier, value);
    }
}
