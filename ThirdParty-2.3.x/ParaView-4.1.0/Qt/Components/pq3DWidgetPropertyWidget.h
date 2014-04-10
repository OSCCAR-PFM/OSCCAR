/*=========================================================================

   Program: ParaView
   Module:    pq3DWidgetPropertyWidget.h

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
#ifndef __pq3DWidgetPropertyWidget_h
#define __pq3DWidgetPropertyWidget_h

#include "pqPropertyWidget.h"

class pq3DWidget;

class pq3DWidgetPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  explicit pq3DWidgetPropertyWidget(pq3DWidget *widget, QWidget *parent = 0);
  ~pq3DWidgetPropertyWidget();

  void apply();
  void reset();

  /// Forwards the call to the internal pq3DWidget instance.
  virtual void select();

  /// Forwards the call to the internal pq3DWidget instance.
  virtual void deselect();

  pq3DWidget* get3DWidget() const { return this->Widget; }

private:
  pq3DWidget *Widget;
};

#endif // __pq3DWidgetPropertyWidget_h
