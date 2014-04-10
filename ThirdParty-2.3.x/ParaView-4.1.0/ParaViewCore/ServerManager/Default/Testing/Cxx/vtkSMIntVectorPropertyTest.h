/*=========================================================================

Program:   ParaView
Module:    vtkSMIntVectorPropertyTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSMIntVectorPropertyTest_h
#define __vtkSMIntVectorPropertyTest_h

#include <QtTest>

class vtkSMIntVectorPropertyTest : public QObject
{
  Q_OBJECT

private slots:
  void SetNumberOfElements();
  void SetElement();
  void SetElements();
  void Copy();
};

#endif
