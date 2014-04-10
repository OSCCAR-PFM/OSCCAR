/*=========================================================================

Program:   ParaView
Module:    vtkSMProxyLinkTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSMProxyLinkTest_h
#define __vtkSMProxyLinkTest_h

#include <QtTest>

class vtkSMProxyLinkTest : public QObject
{
  Q_OBJECT

private slots:
  void AddLinkedProxy();
  void AddException();
};

#endif
