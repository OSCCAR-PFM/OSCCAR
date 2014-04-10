
// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqVariableVariablePlotter.h

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

#ifndef __pqVariableVariablePlotter_h
#define __pqVariableVariablePlotter_h

#include "pqPlotter.h"

class pqVariableVariablePlotter : public pqPlotter
{
  Q_OBJECT;

public:

  pqVariableVariablePlotter()
  {
  }

  virtual ~pqVariableVariablePlotter()
  {
  }
};

#endif // __pqVariableVariablePlotter_h
