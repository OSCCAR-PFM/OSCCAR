/*=========================================================================

   Program: ParaView
   Module: pqPropertyWidget.cxx

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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
#include "pqPropertyWidget.h"

#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqProxy.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"

//-----------------------------------------------------------------------------
pqPropertyWidget::pqPropertyWidget(vtkSMProxy *smProxy, QWidget *parentObject)
  : QWidget(parentObject),
    Proxy(smProxy),
    Property(0),
    ChangeAvailableAsChangeFinished(true),
    AutoUpdateVTKObjects(false)
{
  this->ShowLabel = true;
  this->Links.setAutoUpdateVTKObjects(false);
  this->Links.setUseUncheckedProperties(true);

  this->connect(&this->Links, SIGNAL(qtWidgetChanged()),
                this, SIGNAL(changeAvailable()));

  // This has to be a QueuedConnection otherwise changeFinished() gets fired
  // before changeAvailable() is handled by pqProxyWidget and see BUG #13029.
  this->connect(this, SIGNAL(changeAvailable()),
                this, SLOT(onChangeAvailable()), Qt::QueuedConnection);

  this->connect(this, SIGNAL(changeFinished()),
    this, SLOT(onChangeFinished()));
}

//-----------------------------------------------------------------------------
pqPropertyWidget::~pqPropertyWidget()
{
  foreach (pqPropertyWidgetDecorator* decorator, this->Decorators)
    {
    delete decorator;
    }

  this->Decorators.clear();
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::onChangeAvailable()
{
  if (this->ChangeAvailableAsChangeFinished)
    {
    emit this->changeFinished();
    }
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::onChangeFinished()
{
  if (this->AutoUpdateVTKObjects)
    {
    BEGIN_UNDO_SET("Property Changed");
    this->apply();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
pqView* pqPropertyWidget::view() const
{
  return this->View;
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setView(pqView* pqview)
{
  this->View = pqview;
  emit this->viewChanged(pqview);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPropertyWidget::proxy() const
{
  return this->Proxy;
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setProperty(vtkSMProperty *smproperty)
{
  this->Property = smproperty;
  if (smproperty && smproperty->GetDocumentation())
    {
    QString doc = smproperty->GetDocumentation()->GetDescription();
    doc = doc.trimmed();
    doc = doc.replace(QRegExp("\\s+")," ");
    this->setToolTip(
      QString("<html><head/><body><p align=\"justify\">%1</p></body></html>").arg(doc));
    }
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqPropertyWidget::property() const
{
  return this->Property;
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::apply()
{
  this->Links.accept();
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::reset()
{
  this->Links.reset();
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setShowLabel(bool isLabelVisible)
{
  this->ShowLabel = isLabelVisible;
}

//-----------------------------------------------------------------------------
bool pqPropertyWidget::showLabel() const
{
  return this->ShowLabel;
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::addPropertyLink(QObject *qobject,
                                       const char *qproperty,
                                       const char *qsignal,
                                       vtkSMProperty *smproperty,
                                       int smindex)
{
  this->Links.addPropertyLink(qobject,
                              qproperty,
                              qsignal,
                              this->Proxy,
                              smproperty,
                              smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::addPropertyLink(QObject *qobject,
                                       const char *qproperty,
                                       const char *qsignal,
                                       vtkSMProxy* smproxy,
                                       vtkSMProperty *smproperty,
                                       int smindex)
{
  this->Links.addPropertyLink(qobject, qproperty, qsignal,
    smproxy, smproperty, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setAutoUpdateVTKObjects(bool autoUpdate)
{
  this->AutoUpdateVTKObjects = autoUpdate;
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setUseUncheckedProperties(bool useUnchecked)
{
  this->Links.setUseUncheckedProperties(useUnchecked);
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::addDecorator(pqPropertyWidgetDecorator* decorator)
{
  if (!decorator || decorator->parent() != this)
    {
    qCritical("Either the decorator is NULL or has an invalid parent."
      "Please check the code.");
    }
  else
    {
    this->Decorators.push_back(decorator);
    }
}
