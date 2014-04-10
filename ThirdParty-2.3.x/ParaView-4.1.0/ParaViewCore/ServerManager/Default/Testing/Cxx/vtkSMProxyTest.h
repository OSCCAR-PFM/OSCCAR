/*=========================================================================

Program:   ParaView
Module:    vtkSMProxyTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSMProxyTest_h
#define __vtkSMProxyTest_h

#include <QtTest>

class vtkSMProxyTest : public QObject
{
  Q_OBJECT

private slots:
  void SetAnnotation();
  void GetProperty();
  void GetVTKClassName();
};

#endif
