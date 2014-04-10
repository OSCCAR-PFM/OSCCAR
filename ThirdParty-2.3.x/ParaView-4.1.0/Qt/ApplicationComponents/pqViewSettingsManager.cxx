/*=========================================================================

   Program: ParaView
   Module:    pqViewSettingsManager.cxx

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
#include "pqViewSettingsManager.h"

#include "pqActiveMultiSliceViewOptions.h"
#include "pqActiveObjects.h"
#include "pqActivePlotMatrixViewOptions.h"
#include "pqActivePythonViewOptions.h"
#include "pqActiveRenderViewOptions.h"
#include "pqActiveXYChartOptions.h"
#include "pqApplicationCore.h"
#include "pqComparativeRenderView.h"
#include "pqComparativeXYBarChartView.h"
#include "pqComparativeXYChartView.h"
#include "pqInterfaceTracker.h"
#include "pqMultiSliceView.h"
#include "pqPlotMatrixView.h"
#include "pqViewOptionsInterface.h"
#include "pqXYBarChartView.h"
#include "pqXYChartView.h"

#include "vtkPVConfig.h"
#if defined(PARAVIEW_ENABLE_PYTHON) && defined(PARAVIEW_ENABLE_MATPLOTLIB)
#include "pqPythonView.h"
#endif

//-----------------------------------------------------------------------------
pqViewSettingsManager::pqViewSettingsManager(QObject* parentObject)
  : Superclass(parentObject)
{
  pqActiveRenderViewOptions* renderViewOptions = new
    pqActiveRenderViewOptions(this);
  this->registerOptions(pqRenderView::renderViewType(), renderViewOptions);
  this->registerOptions(pqComparativeRenderView::comparativeRenderViewType(),
    renderViewOptions);

  pqActiveXYChartOptions *xyChartOptions = new pqActiveXYChartOptions(this);
  pqActiveXYChartOptions *xyBarChartOptions = new pqActiveXYChartOptions(this);
  this->registerOptions(pqXYChartView::XYChartViewType(), xyChartOptions);
  this->registerOptions(pqXYBarChartView::XYBarChartViewType(), xyBarChartOptions);

  // register for comparative views.
  this->registerOptions(pqComparativeXYChartView::chartViewType(), xyChartOptions);
  this->registerOptions(pqComparativeXYBarChartView::chartViewType(), xyBarChartOptions);

  // register for plot matrix views.
  pqActivePlotMatrixViewOptions *plotMatrixOptions = new pqActivePlotMatrixViewOptions(this);
  this->registerOptions(pqPlotMatrixView::viewType(), plotMatrixOptions);

  // register for multislice view
  pqActiveMultiSliceViewOptions *multiSliceOptions = new pqActiveMultiSliceViewOptions(this);
  this->registerOptions(pqMultiSliceView::multiSliceViewType(), multiSliceOptions);

#if defined(PARAVIEW_ENABLE_PYTHON) && defined(PARAVIEW_ENABLE_MATPLOTLIB)
  // register for python view
  pqActivePythonViewOptions *pythonViewOptions = new pqActivePythonViewOptions(this);
  this->registerOptions(pqPythonView::pythonViewType(), pythonViewOptions);
#endif

  /// Add panes as plugins are loaded.
  QObject::connect(pqApplicationCore::instance()->interfaceTracker(),
    SIGNAL(interfaceRegistered(QObject*)),
    this, SLOT(pluginLoaded(QObject*)));

  // Load panes from already loaded plugins.
  foreach (QObject* plugin_interface,
    pqApplicationCore::instance()->interfaceTracker()->interfaces())
    {
    this->pluginLoaded(plugin_interface);
    }

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(viewChanged(pqView*)),
    this, SLOT(setActiveView(pqView*)));
  this->setActiveView(pqActiveObjects::instance().activeView());
}

//-----------------------------------------------------------------------------
void pqViewSettingsManager::pluginLoaded(QObject* iface)
{
  pqViewOptionsInterface* viewOptions =
    qobject_cast<pqViewOptionsInterface*>(iface);
  if(viewOptions)
    {
    foreach(QString viewtype, viewOptions->viewTypes())
      {

      // Try to create active view options
      pqActiveViewOptions* options =
        viewOptions->createActiveViewOptions(viewtype, this);
      if (options)
        {
        this->registerOptions(viewtype, options);
        }
      }
    }
}


