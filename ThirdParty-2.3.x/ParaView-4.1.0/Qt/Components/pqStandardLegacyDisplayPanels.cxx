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
#include "pqStandardLegacyDisplayPanels.h"

#include "pqParallelCoordinatesChartDisplayPanel.h"
#include "pqPlotMatrixDisplayPanel.h"
#include "pqRepresentation.h"
#include "pqSpreadSheetDisplayEditor.h"
#include "pqTextDisplayPropertiesWidget.h"
#include "pqTextRepresentation.h"
#include "pqXYChartDisplayPanel.h"
#include "vtkSMProxy.h"
#include <QDebug>
#include <QWidget>

//-----------------------------------------------------------------------------
pqStandardLegacyDisplayPanels::pqStandardLegacyDisplayPanels(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqStandardLegacyDisplayPanels::~pqStandardLegacyDisplayPanels()
{
}

//-----------------------------------------------------------------------------
/// Returns true if this panel can be created for the given the proxy.
bool pqStandardLegacyDisplayPanels::canCreatePanel(pqRepresentation* proxy) const
{
  if(!proxy || !proxy->getProxy())
    {
    return false;
    }

  QString type = proxy->getProxy()->GetXMLName();

  if (type == "XYPlotRepresentation" ||
    type == "XYChartRepresentation" ||
    type == "XYBarChartRepresentation" ||
    type == "BarChartRepresentation" ||
    type == "SpreadSheetRepresentation" ||
    qobject_cast<pqTextRepresentation*>(proxy)||
    type == "ScatterPlotRepresentation" ||
    type == "ParallelCoordinatesRepresentation" ||
    type == "PlotMatrixRepresentation")
    {
    return true;
    }

  return false;
}
//-----------------------------------------------------------------------------
/// Creates a panel for the given proxy
pqDisplayPanel* pqStandardLegacyDisplayPanels::createPanel(pqRepresentation* proxy, QWidget* p)
{
  if(!proxy || !proxy->getProxy())
    {
    qDebug() << "Proxy is null" << proxy;
    return NULL;
    }

  QString type = proxy->getProxy()->GetXMLName();
  if (type == QString("XYChartRepresentation"))
    {
    return new pqXYChartDisplayPanel(proxy, p);
    }
  if (type == QString("XYBarChartRepresentation"))
    {
    return new pqXYChartDisplayPanel(proxy, p);
    }
  if (type == "SpreadSheetRepresentation")
    {
    return new pqSpreadSheetDisplayEditor(proxy, p);
    }

  if (qobject_cast<pqTextRepresentation*>(proxy))
    {
    return new pqTextDisplayPropertiesWidget(proxy, p);
    }
#ifdef FIXME
  if (type == "ScatterPlotRepresentation")
    {
    return new pqScatterPlotDisplayPanel(proxy, p);
    }
#endif
  if (type == QString("ParallelCoordinatesRepresentation"))
    {
    return new pqParallelCoordinatesChartDisplayPanel(proxy, p);
    }
  else if (type == "PlotMatrixRepresentation")
    {
    return new pqPlotMatrixDisplayPanel(proxy, p);
    }

  return NULL;
}
