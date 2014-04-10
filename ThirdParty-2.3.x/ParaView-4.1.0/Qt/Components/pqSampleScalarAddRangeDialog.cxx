/*=========================================================================

   Program: ParaView
   Module:    pqSampleScalarAddRangeDialog.cxx

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

#include "pqSampleScalarAddRangeDialog.h"
#include "ui_pqSampleScalarAddRangeDialog.h"

#include <QDoubleValidator>

#include <algorithm>

///////////////////////////////////////////////////////////////////////////
// pqSampleScalarAddRangeDialog::pqImplementation

class pqSampleScalarAddRangeDialog::pqImplementation
{
public:
  Ui::pqSampleScalarAddRangeDialog Ui;
  bool StrictLog;
};

///////////////////////////////////////////////////////////////////////////
// pqSampleScalarAddRangeDialog

pqSampleScalarAddRangeDialog::pqSampleScalarAddRangeDialog(
    double default_from,
    double default_to,
    unsigned long default_steps,
    bool default_logarithmic,
    QWidget* Parent) :
  Superclass(Parent),
  Implementation(new pqImplementation())
{
  this->Implementation->StrictLog = false;
  this->Implementation->Ui.setupUi(this);
  
  this->Implementation->Ui.from->setValidator(
    new QDoubleValidator(this->Implementation->Ui.from));
  this->setFrom(default_from);
    
  this->Implementation->Ui.to->setValidator(
    new QDoubleValidator(this->Implementation->Ui.to));
  this->setTo(default_to);
  
  this->Implementation->Ui.steps->setValidator(
    new QIntValidator(2, 9999, this->Implementation->Ui.steps));
  this->setSteps(default_steps);
  
  this->setLogarithmic(default_logarithmic);
  
  QObject::connect(
    this->Implementation->Ui.from,
    SIGNAL(textChanged(const QString&)),
    this,
    SLOT(onRangeChanged()));
  
  QObject::connect(
    this->Implementation->Ui.to,
    SIGNAL(textChanged(const QString&)),
    this,
    SLOT(onRangeChanged()));
  
  this->onRangeChanged();
}

pqSampleScalarAddRangeDialog::~pqSampleScalarAddRangeDialog()
{
  delete this->Implementation;
}

double pqSampleScalarAddRangeDialog::from() const
{
  return this->Implementation->Ui.from->text().toDouble();
}

void pqSampleScalarAddRangeDialog::setFrom(double value)
{
  this->Implementation->Ui.from->setText(QString::number(value));
}

double pqSampleScalarAddRangeDialog::to() const
{
  return this->Implementation->Ui.to->text().toDouble();
}

void pqSampleScalarAddRangeDialog::setTo(double value)
{
  this->Implementation->Ui.to->setText(QString::number(value));
}

unsigned long pqSampleScalarAddRangeDialog::steps() const
{
  return this->Implementation->Ui.steps->text().toInt();
}

void pqSampleScalarAddRangeDialog::setSteps(unsigned long number)
{
  this->Implementation->Ui.steps->setText(QString::number(number));
}

bool pqSampleScalarAddRangeDialog::logarithmic() const
{
  return this->Implementation->Ui.log->isChecked();
}

void pqSampleScalarAddRangeDialog::setLogarithmic(bool useLog)
{
  this->Implementation->Ui.log->setChecked(useLog);
}

void pqSampleScalarAddRangeDialog::setLogRangeStrict(bool on)
{
  if(on != this->Implementation->StrictLog)
    {
    this->Implementation->StrictLog = on;
    if(this->Implementation->StrictLog)
      {
      this->Implementation->Ui.logWarning->setText(
          "The range must be greater than zero to use logarithmic scale.");
      }
    else
      {
      this->Implementation->Ui.logWarning->setText(
          "Can't use logarithmic scale when zero is in the range.");
      }
    }
}

bool pqSampleScalarAddRangeDialog::isLogRangeStrict() const
{
  return this->Implementation->StrictLog;
}

void pqSampleScalarAddRangeDialog::onRangeChanged()
{
  double from_value = this->from();
  double to_value = this->to();
  bool logOk = false;
  if(this->Implementation->StrictLog)
    {
    logOk = from_value > 0.0 && to_value > 0.0;
    }
  else
    {
    if(to_value < from_value)
      {
      std::swap(from_value, to_value);
      }

    logOk = !(from_value < 0.0 && to_value > 0.0);
    }
  
  if(!logOk)
    {
    this->Implementation->Ui.log->setChecked(false);
    }
    
  this->Implementation->Ui.log->setEnabled(logOk);
  this->Implementation->Ui.logWarning->setVisible(!logOk);
}
