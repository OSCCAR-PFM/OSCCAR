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
#include "pqFontPropertyWidget.h"
#include "ui_pqFontPropertyWidget.h"

#include "pqComboBoxDomain.h"
#include "pqPropertiesPanel.h"
#include "pqSignalAdaptors.h"
#include "pqStandardColorLinkAdaptor.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"

class pqFontPropertyWidget::pqInternals
{
public:
  Ui::FontPropertyWidget Ui;

  pqInternals(pqFontPropertyWidget* self)
    {
    this->Ui.setupUi(self);
    this->Ui.mainLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.mainLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    }
};

//-----------------------------------------------------------------------------
pqFontPropertyWidget::pqFontPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject),
  Internals(new pqInternals(this))
{
  Ui::FontPropertyWidget &ui = this->Internals->Ui;

  vtkSMProperty* smproperty = smgroup->GetProperty("Family");
  if (smproperty)
    {
    new pqComboBoxDomain(ui.FontFamily, smproperty);
    pqSignalAdaptorComboBox *adaptor = new pqSignalAdaptorComboBox(ui.FontFamily);
    this->addPropertyLink(
      adaptor, "currentText", SIGNAL(currentTextChanged(QString)),
      smproperty);
    }
  else
    {
    ui.FontFamily->hide();
    }

  smproperty = smgroup->GetProperty("Size");
  if (smproperty)
    {
    this->addPropertyLink(ui.FontSize, "value", SIGNAL(valueChanged(int)),
      smproperty);
    }
  else
    {
    ui.FontSize->hide();
    }

  smproperty = smgroup->GetProperty("Color");
  if (smproperty)
    {
    pqSignalAdaptorColor *adaptor = new pqSignalAdaptorColor(ui.FontColor, "chosenColor",
      SIGNAL(chosenColorChanged(const QColor&)), false);
    this->addPropertyLink(adaptor, "color",
      SIGNAL(colorChanged(const QVariant&)), smproperty);

    // pqStandardColorLinkAdaptor makes it possible to set this color to one of
    // the standard colors.
    new pqStandardColorLinkAdaptor(ui.FontColor, smproxy, smproxy->GetPropertyName(smproperty));
    }
  else
    {
    ui.FontColor->hide();
    }
  
  smproperty = smgroup->GetProperty("Bold");
  if (smproperty)
    {
    this->addPropertyLink(ui.Bold, "checked", SIGNAL(toggled(bool)), smproperty);
    }
  else
    {
    ui.Bold->hide();
    }

  smproperty = smgroup->GetProperty("Italics");
  if (smproperty)
    {
    this->addPropertyLink(ui.Italics, "checked", SIGNAL(toggled(bool)), smproperty);
    }
  else
    {
    ui.Italics->hide();
    }

  smproperty = smgroup->GetProperty("Shadow");
  if (smproperty)
    {
    this->addPropertyLink(ui.Shadow, "checked", SIGNAL(toggled(bool)), smproperty);
    }
  else
    {
    ui.Shadow->hide();
    }
}

//-----------------------------------------------------------------------------
pqFontPropertyWidget::~pqFontPropertyWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}
