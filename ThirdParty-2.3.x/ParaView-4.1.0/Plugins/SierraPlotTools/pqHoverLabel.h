// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqHoverLabel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef __pqHoverLabel_h
#define __pqHoverLabel_h

#include <QLabel>

class QMouseEvent;
class pqPlotter;
class QEvent;

class pqHoverLabel : public QLabel
{
  Q_OBJECT;

public:
  pqHoverLabel(QWidget *parent = 0);

  virtual void mouseMoveEvent ( QMouseEvent * theEvent );

  void setPlotter(pqPlotter * thePlotter);

  bool event(QEvent * event);

  pqPlotter * plotter;
};

#endif //__pqHoverLabel_h
