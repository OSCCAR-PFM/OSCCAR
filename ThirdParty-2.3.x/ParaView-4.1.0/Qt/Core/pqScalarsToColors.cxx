/*=========================================================================

   Program: ParaView
   Module:    pqScalarsToColors.cxx

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
#include "pqScalarsToColors.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxy.h"

#include <QPointer>
#include <QList>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqPipelineRepresentation.h"
#include "pqRenderViewBase.h"
#include "pqScalarBarRepresentation.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "vtkSMProperty.h"

//-----------------------------------------------------------------------------
class pqScalarsToColorsInternal
{
public:
  QList<QPointer<pqScalarBarRepresentation> > ScalarBars;
  vtkEventQtSlotConnect* VTKConnect;

  pqScalarsToColorsInternal()
    {
    this->VTKConnect = vtkEventQtSlotConnect::New();
    }
  ~pqScalarsToColorsInternal()
    {
    this->VTKConnect->Delete();
    }
};

//-----------------------------------------------------------------------------
pqScalarsToColors::pqScalarsToColors(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* _parent/*=NULL*/)
: pqProxy(group, name, proxy, server, _parent)
{
  this->Internal = new pqScalarsToColorsInternal;

  this->Internal->VTKConnect->Connect(proxy->GetProperty("RGBPoints"),
                                      vtkCommand::ModifiedEvent,
                                      this, SLOT(checkRange()));
  this->Internal->VTKConnect->Connect(proxy->GetProperty("UseLogScale"),
                                      vtkCommand::ModifiedEvent,
                                      this, SLOT(checkRange()));
}

//-----------------------------------------------------------------------------
pqScalarsToColors::~pqScalarsToColors()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::addScalarBar(pqScalarBarRepresentation* sb)
{
  if (this->Internal->ScalarBars.indexOf(sb) == -1)
    {
    this->Internal->ScalarBars.push_back(sb);
    emit this->scalarBarsChanged();
    }
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::removeScalarBar(pqScalarBarRepresentation* sb)
{
  if (this->Internal->ScalarBars.removeAll(sb) > 0)
    {
    emit this->scalarBarsChanged();
    }
}

//-----------------------------------------------------------------------------
pqScalarBarRepresentation* pqScalarsToColors::getScalarBar(pqRenderViewBase* ren) const
{
  foreach(pqScalarBarRepresentation* sb, this->Internal->ScalarBars)
    {
    if (sb && (sb->getView() == ren))
      {
      return sb;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setScalarRangeLock(bool lock)
{
  vtkSMProperty* prop = this->getProxy()->GetProperty("LockScalarRange");
  if (prop)
    {
    pqSMAdaptor::setElementProperty(prop, (lock? 1: 0));
    }
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqScalarsToColors::getScalarRangeLock() const
{
  vtkSMProperty* prop = this->getProxy()->GetProperty("LockScalarRange");
  if (prop && pqSMAdaptor::getElementProperty(prop).toInt() != 0)
    {
    return true;
    }
  // we may keep some GUI only state for vtkLookupTable proxies.
  return false;
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::hideUnusedScalarBars()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  QList<pqPipelineRepresentation*> displays =
    smmodel->findItems<pqPipelineRepresentation*>(this->getServer());

  bool used_at_all = false;
  foreach(pqPipelineRepresentation* display, displays)
    {
    if (display->isVisible() &&
      display->getColorField(true) != pqPipelineRepresentation::solidColor() &&
      display->getLookupTableProxy() == this->getProxy())
      {
      used_at_all = true;
      break;
      }
    }
  if (!used_at_all)
    {
    foreach(pqScalarBarRepresentation* sb, this->Internal->ScalarBars)
      {
      sb->setVisible(false);
      sb->renderViewEventually();
      }
    }
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setScalarRange(double min, double max)
{
  if (min > max)
    {
    double t = min;
    min = max;
    max = t;
    }

  pqSMAdaptor::setElementProperty(
    this->getProxy()->GetProperty("ScalarRangeInitialized"), 1);

  QPair <double, double> current_range = this->getScalarRange();
  if (current_range.first == min && current_range.second == max)
    {
    // Nothing to do.
    return;
    }

  // Adjust vtkColorTransferFunction points to the new range.
  double dold = (current_range.second - current_range.first);
  dold = (dold > 0) ? dold : 1;

  double dnew = (max -min);
  dnew = (dnew >= 0) ? dnew : 1;

  double scale = dnew/dold;
  pqSMAdaptor::setElementProperty(
    this->getProxy()->GetProperty("AllowDuplicateScalars"), 1);
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->getProxy()->GetProperty("RGBPoints"));
  QList<QVariant> controlPoints = pqSMAdaptor::getMultipleElementProperty(dvp);

  int num_elems_per_command = dvp->GetNumberOfElementsPerCommand();
  for (int cc=0; cc < controlPoints.size();  cc += num_elems_per_command)
    {
    // These checks ensure that the first and last control points match the
    // min and max values exactly.
    if (cc==0)
      {
      controlPoints[cc] = min;
      }
    else if ((cc+num_elems_per_command) >= controlPoints.size())
      {
      controlPoints[cc] = max;
      }
    else
      {
      controlPoints[cc] =
        scale * (controlPoints[cc].toDouble()-current_range.first) + min;
      }
    }
  pqSMAdaptor::setMultipleElementProperty(dvp, controlPoints);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QPair<double, double> pqScalarsToColors::getScalarRange() const
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->getProxy()->GetProperty("RGBPoints"));
  QList<QVariant> controlPoints = pqSMAdaptor::getMultipleElementProperty(dvp);

  if (controlPoints.size() == 0)
    {
    return QPair<double, double>(0, 0);
    }

  int max_index = dvp->GetNumberOfElementsPerCommand() * (
    (controlPoints.size()-1)/ dvp->GetNumberOfElementsPerCommand());
  return QPair<double, double>(controlPoints[0].toDouble(),
    controlPoints[max_index].toDouble());
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setWholeScalarRange(double min, double max)
{
  if (this->getScalarRangeLock())
    {
    return;
    }

  if (pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("ScalarRangeInitialized")).toBool())
    {
    QPair<double, double> curRange = this->getScalarRange();
    min = (min < curRange.first)?  min :  curRange.first;
    max = (max > curRange.second)?  max :  curRange.second;
    }

  this->setScalarRange(min, max);
}

//-----------------------------------------------------------------------------
bool pqScalarsToColors::getUseLogScale() const
{
  vtkSMProxy *proxy = this->getProxy();
  vtkSMProperty *prop = proxy->GetProperty("UseLogScale");
  return (pqSMAdaptor::getElementProperty(prop).toInt() != 0);
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::checkRange()
{
  // Only need to adjust range if using log scale.
  if (!this->getUseLogScale()) return;

  QPair<double, double> range = this->getScalarRange();
  if (range.first > 0.0) return;

  // If we are here, we need to adjust the range to be all positive.
  QPair<double, double> newRange;
  if (range.second > 1.0)
    {
    newRange.first = 1.0;
    newRange.second = range.second;
    }
  else if (range.second > 0.0)
    {
    newRange.first = range.second/10.0;
    newRange.second = range.second;
    }
  else
    {
    range.first = 1.0;
    range.second = 10.0;
    }

  qWarning("Warning: Range [%g,%g] invalid for log scaling.  Changing to [%g,%g].",
           range.first, range.second, newRange.first, newRange.second);
  this->setScalarRange(newRange.first, newRange.second);
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setVectorMode(Mode mode, int comp)
{
  vtkSMProxy* proxy = this->getProxy();
  pqSMAdaptor::setEnumerationProperty(proxy->GetProperty("VectorMode"),
    (mode == MAGNITUDE)? "Magnitude" : "Component");
  pqSMAdaptor::setElementProperty(proxy->GetProperty("VectorComponent"),
    (mode == COMPONENT)? comp: 0);
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqScalarsToColors::getIndexedLookup()
{
  vtkSMProxy* proxy = this->getProxy();
  return pqSMAdaptor::getElementProperty( proxy->GetProperty( "IndexedLookup" ) ).toBool();
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setIndexedLookup( bool indexedLookup )
{
  vtkSMProxy* proxy = this->getProxy();
  pqSMAdaptor::setElementProperty( proxy->GetProperty( "IndexedLookup" ), indexedLookup );
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqScalarsToColors::getAnnotations()
{
  vtkSMProxy* proxy = this->getProxy();
  return pqSMAdaptor::getMultipleElementProperty( proxy->GetProperty( "Annotations" ) );
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setAnnotations( const QList<QVariant>& annotations )
{
  vtkSMProxy* proxy = this->getProxy();
  pqSMAdaptor::setMultipleElementProperty( proxy->GetProperty( "Annotations" ), annotations );
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::updateScalarBarTitles(const QString& component)
{
  foreach(pqScalarBarRepresentation* sb, this->Internal->ScalarBars)
    {
    sb->setTitle(sb->getTitle().first, component);
    }
}

//-----------------------------------------------------------------------------
pqScalarsToColors::Mode pqScalarsToColors::getVectorMode() const
{
  if (pqSMAdaptor::getEnumerationProperty(
      this->getProxy()->GetProperty("VectorMode")) == "Magnitude")
    {
    return MAGNITUDE;
    }
  return COMPONENT;
}

//-----------------------------------------------------------------------------
int pqScalarsToColors::getVectorComponent() const
{
  return pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("VectorComponent")).toInt();
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::build()
{
  this->getProxy()->InvokeCommand("Build");
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setColorRangeScalingMode(int mode)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("pqScalarsToColors/COLOR_RANGE_SCALING_MODE", mode);
}

//-----------------------------------------------------------------------------
int pqScalarsToColors::colorRangeScalingMode(
  int default_value/*=GROW_ON_MODIFIED*/)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  return settings->value(
    "pqScalarsToColors/COLOR_RANGE_SCALING_MODE", default_value).toInt();
}
