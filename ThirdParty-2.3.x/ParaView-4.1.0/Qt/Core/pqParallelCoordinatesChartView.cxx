/*=========================================================================

   Program: ParaView
   Module:    pqParallelCoordinatesChartView.cxx

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
#include "pqParallelCoordinatesChartView.h"

#include "vtkSMProperty.h"
#include "vtkSMContextViewProxy.h"
#include "pqSMAdaptor.h"
#include "pqRepresentation.h"
#include "pqDataRepresentation.h"

//-----------------------------------------------------------------------------
pqParallelCoordinatesChartView::pqParallelCoordinatesChartView(const QString& group,
                             const QString& name,
                             vtkSMContextViewProxy* viewModule,
                             pqServer* server,
                             QObject* p/*=NULL*/):
  Superclass(chartViewType(), group, name, viewModule, server, p)
{
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    this, SLOT(onRemoveRepresentation(pqRepresentation*)));
  QObject::connect(
    this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));
}

//-----------------------------------------------------------------------------
pqParallelCoordinatesChartView::~pqParallelCoordinatesChartView()
{
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartView::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartView::onAddRepresentation(pqRepresentation* repr)
{
  this->updateRepresentationVisibility(repr, repr->isVisible());
}


//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartView::onRemoveRepresentation(pqRepresentation*)
{
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartView::updateRepresentationVisibility(
    pqRepresentation* repr, bool visible)
{
  if (!visible && repr)
    {
    emit this->showing(0);
    }

  if (!visible || !repr)
    {
    return;
    }

  // If visible, turn-off visibility of all other representations.
  QList<pqRepresentation*> reprs = this->getRepresentations();
  foreach (pqRepresentation* cur_repr, reprs)
    {
    if (cur_repr != repr)
      {
      cur_repr->setVisible(false);
      }
    }

  pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(repr);
  emit this->showing(dataRepr);
}
