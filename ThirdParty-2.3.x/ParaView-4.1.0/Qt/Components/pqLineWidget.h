/*=========================================================================

   Program:   ParaQ
   Module:    pqLineWidget.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.2. 

   See License_v1.2.txt for the full ParaQ license.
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

#ifndef _pqLineWidget_h
#define _pqLineWidget_h

#include "pq3DWidget.h"
#include "pqComponentsModule.h"
#include <QColor>

class pqServer;

/// Provides a complete Qt UI for working with a 3D line widget
class PQCOMPONENTS_EXPORT pqLineWidget : public pq3DWidget
{
  typedef pq3DWidget Superclass;
  
  Q_OBJECT
  
public:
  pqLineWidget(vtkSMProxy* o, vtkSMProxy* pxy, QWidget* p = 0, 
    const char* xmlname="LineWidgetRepresentation");
  ~pqLineWidget();

  /// Resets the bounds of the 3D widget to the reference proxy bounds.
  /// This typically calls PlaceWidget on the underlying 3D Widget 
  /// with reference proxy bounds.
  /// This should be explicitly called after the panel is created
  /// and the widget is initialized i.e. the reference proxy, controlled proxy
  /// and hints have been set.
  virtual void resetBounds()
    { this->Superclass::resetBounds(); }
  virtual void resetBounds(double bounds[6]);

  void setControlledProperties(vtkSMProperty* point1, vtkSMProperty* point2);
  void setLineColor(const QColor& color);

public slots:
  void onXAxis();
  void onYAxis();
  void onZAxis();

protected:
  virtual void setControlledProperty(const char* function,
    vtkSMProperty * controlled_property);

  /// Called on pick.
  virtual void pick(double, double, double);

private slots:
  void onWidgetVisibilityChanged(bool visible);

private:
  void createWidget(pqServer* server, const QString& xmlname);
  void getReferenceBoundingBox(double center[3], double size[3]);

  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
