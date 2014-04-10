/*=========================================================================

   Program: ParaView
   Module: pqCommandButtonPropertyWidget.cxx

   Copyright (c) 2005-2012 Kitware Inc.
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

#include "pqCommandButtonPropertyWidget.h"

#include "vtkSMProxy.h"
#include "vtkSMProperty.h"

#include "pqPVApplicationCore.h"

#include <QVBoxLayout>
#include <QPushButton>

pqCommandButtonPropertyWidget::pqCommandButtonPropertyWidget(vtkSMProxy *smProxy,
                                                             vtkSMProperty *proxyProperty,
                                                             QWidget *pWidget)
  : pqPropertyWidget(smProxy, pWidget),
    Property(proxyProperty)
{
  QVBoxLayout *l = new QVBoxLayout;
  l->setSpacing(0);
  l->setMargin(0);

  QPushButton *button = new QPushButton(proxyProperty->GetXMLLabel());
  connect(button, SIGNAL(clicked()), this, SLOT(buttonClicked()));
  l->addWidget(button);

  this->setShowLabel(false);
  this->setLayout(l);

  PV_DEBUG_PANELS() << "pqCommandButtonPropertyWidget for a property with "
                << "the panel_widget=\"command_button\" attribute";
}

pqCommandButtonPropertyWidget::~pqCommandButtonPropertyWidget()
{
}

void pqCommandButtonPropertyWidget::buttonClicked()
{
  this->Property->Modified();
  this->proxy()->UpdateProperty(this->proxy()->GetPropertyName(this->Property));

  // Trigger a rendering
  pqPVApplicationCore::instance()->render();
}
