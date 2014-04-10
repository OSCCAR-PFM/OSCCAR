/*=========================================================================

   Program: ParaView
   Module:    pqRescaleRange.cxx

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

/// \file pqRescaleRange.cxx
/// \date 3/28/2007

#include "pqRescaleRange.h"
#include "ui_pqRescaleRangeDialog.h"

#include <QDoubleValidator>

class pqRescaleRangeForm : public Ui::pqRescaleRangeDialog {};


pqRescaleRange::pqRescaleRange(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Form = new pqRescaleRangeForm();

  // Set up the ui.
  this->Form->setupUi(this);

  // Make sure the line edits only allow number inputs.
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Form->MinimumScalar->setValidator(validator);
  this->Form->MaximumScalar->setValidator(validator);

  // Connect the gui elements.
  this->connect(this->Form->MinimumScalar, SIGNAL(textChanged(const QString &)),
      this, SLOT(validate()));
  this->connect(this->Form->MaximumScalar, SIGNAL(textChanged(const QString &)),
      this, SLOT(validate()));
  
  this->connect(this->Form->RescaleButton, SIGNAL(clicked()),
      this, SLOT(accept()));
  this->connect(this->Form->CancelButton, SIGNAL(clicked()),
      this, SLOT(reject()));
}

pqRescaleRange::~pqRescaleRange()
{
  delete this->Form;
}

void pqRescaleRange::setRange(double min, double max)
{
  if(min > max)
    {
    double tmp = min;
    min = max;
    max = tmp;
    }

  // Update the displayed range.
  this->Form->MinimumScalar->setText(QString::number(min, 'g', 6));
  this->Form->MaximumScalar->setText(QString::number(max, 'g', 6));
}

double pqRescaleRange::getMinimum() const
{
  return this->Form->MinimumScalar->text().toDouble();
}

double pqRescaleRange::getMaximum() const
{
  return this->Form->MaximumScalar->text().toDouble();
}

void pqRescaleRange::validate()
{
  int dummy;
  QString tmp1 = this->Form->MinimumScalar->text();
  QString tmp2 = this->Form->MaximumScalar->text();

  if(this->Form->MinimumScalar->validator()->validate(tmp1, dummy) ==
    QValidator::Acceptable &&
     this->Form->MaximumScalar->validator()->validate(tmp2, dummy) ==
    QValidator::Acceptable &&
    tmp1.toDouble() <= tmp2.toDouble())
    {
    this->Form->RescaleButton->setEnabled(true);
    }
  else
    {
    this->Form->RescaleButton->setEnabled(false);
    }
}

