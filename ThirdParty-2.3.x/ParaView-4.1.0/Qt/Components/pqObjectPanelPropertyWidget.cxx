/*=========================================================================

   Program: ParaView
   Module: pqObjectPanelPropertyWidget.cxx

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

#include "pqObjectPanelPropertyWidget.h"

#include <QVBoxLayout>

pqObjectPanelPropertyWidget::pqObjectPanelPropertyWidget(pqObjectPanel *objectPanel,
                                                         QWidget *parentObject)
  : pqPropertyWidget(objectPanel->proxy(), parentObject)
{
  this->ObjectPanel = objectPanel;
  this->connect(this->ObjectPanel, SIGNAL(modified()),
    this, SIGNAL(changeAvailable()));
  this->connect(this->ObjectPanel, SIGNAL(modified()),
    this, SIGNAL(changeFinished()));

  this->connect(this, SIGNAL(viewChanged(pqView*)),
                this->ObjectPanel, SLOT(setView(pqView*)));

  QVBoxLayout *layoutLocal = new QVBoxLayout;
  layoutLocal->setMargin(0);
  layoutLocal->addWidget(objectPanel);
  setLayout(layoutLocal);
}

pqObjectPanelPropertyWidget::~pqObjectPanelPropertyWidget()
{
}

void pqObjectPanelPropertyWidget::apply()
{
  this->ObjectPanel->accept();
}

void pqObjectPanelPropertyWidget::reset()
{
  this->ObjectPanel->reset();
}


void pqObjectPanelPropertyWidget::select()
{
  this->ObjectPanel->select();
}

void pqObjectPanelPropertyWidget::deselect()
{
  this->ObjectPanel->deselect();
}

pqObjectPanel* pqObjectPanelPropertyWidget::getObjectPanel() const
{
  return this->ObjectPanel;
}
