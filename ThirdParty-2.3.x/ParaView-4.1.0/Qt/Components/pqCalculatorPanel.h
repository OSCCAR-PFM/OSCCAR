/*=========================================================================

   Program: ParaView
   Module:    pqCalculatorPanel.h

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

#ifndef _pqCalculatorPanel_h
#define _pqCalculatorPanel_h

#include "pqObjectPanel.h"

/// Panel for vtkArrayCalculator proxy
class PQCOMPONENTS_EXPORT pqCalculatorPanel : public pqObjectPanel
{
  Q_OBJECT
public:
  /// constructor
  pqCalculatorPanel(pqProxy* proxy, QWidget* p = 0);
  /// destructor
  ~pqCalculatorPanel();

  /// accept the changes made to the properties
  /// changes will be propogated down to the server manager
  virtual void accept();

  /// reset the changes made
  /// editor will query properties from the server manager
  virtual void reset();

protected slots:
  /// slot called when any calculator button is pushed
  void buttonPressed(const QString& t);
  
  void updateVariables(const QString& mode);
  void variableChosen(QAction* a);
  void disableResults(bool);
  void updateVariableNames();
protected:
  class pqInternal;
  pqInternal* Internal;
};

#endif

