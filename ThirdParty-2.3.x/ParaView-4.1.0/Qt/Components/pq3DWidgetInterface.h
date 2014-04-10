/*=========================================================================

   Program: ParaView
   Module:    pq3DWidgetInterface.h

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
#ifndef __pq3DWidgetInterface_h 
#define __pq3DWidgetInterface_h

#include <QtPlugin>
#include "pqComponentsModule.h"

class pq3DWidget;
class vtkSMProxy;

/// Interface for plugins that provide pq3DWidget subclasses.
class PQCOMPONENTS_EXPORT pq3DWidgetInterface
{
public:
  virtual ~pq3DWidgetInterface();

  /// Creates the 3D widget of the requested type is possible otherwise simply
  /// returns NULL.
  /// \c referenceProxy -- source proxy providing initialization data bounds
  ///                      etc.
  /// \c controlledProxy -- proxy whose properties are controlled by the 3D
  ///                       widget.
  virtual pq3DWidget* newWidget(const QString& name,
    vtkSMProxy* referenceProxy,
    vtkSMProxy* controlledProxy)=0;
};

Q_DECLARE_INTERFACE(pq3DWidgetInterface, "com.kitware/paraview/3dwidget")

#endif


