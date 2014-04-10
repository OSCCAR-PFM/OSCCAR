/*=========================================================================

   Program: ParaView
   Module: pqTextureSelectorPropertyWidget.h

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

#include "pqTextureSelectorPropertyWidget.h"

#include <QVBoxLayout>

#include "pqRenderView.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqPipelineRepresentation.h"

pqTextureSelectorPropertyWidget::pqTextureSelectorPropertyWidget(vtkSMProxy *smProxy, QWidget *pWidget)
  : pqPropertyWidget(smProxy, pWidget)
{
  QVBoxLayout *l = new QVBoxLayout;
  l->setMargin(0);

  this->Selector = new pqTextureComboBox(this);

  pqServerManagerModel *smm = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineRepresentation *repr = smm->findItem<pqPipelineRepresentation *>(smProxy);
  if(repr)
    {
    this->Selector->setRepresentation(repr);
    }

  this->connect(this, SIGNAL(viewChanged(pqView*)),
                this, SLOT(handleViewChanged(pqView*)));

  l->addWidget(this->Selector);
  this->setLayout(l);
}

pqTextureSelectorPropertyWidget::~pqTextureSelectorPropertyWidget()
{
}

void pqTextureSelectorPropertyWidget::handleViewChanged(pqView *v)
{
  pqRenderView *renderView = qobject_cast<pqRenderView *>(v);
  if(renderView)
    {
    this->Selector->setRenderView(renderView);
    }
}
