// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSierraPlotToolsActionGroup.h

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

#ifndef __pqSierraPlotToolsActionGroup_h
#define __pqSierraPlotToolsActionGroup_h

#include <QActionGroup>

/// Adds actions that are helpful for setting up visualization of SLAC
/// simulation result files.
class pqSierraPlotToolsActionGroup : public QActionGroup
{
  Q_OBJECT;
public:
  pqSierraPlotToolsActionGroup(QObject *p);

private:
  pqSierraPlotToolsActionGroup(const pqSierraPlotToolsActionGroup &);     // Not implemented
  void operator=(const pqSierraPlotToolsActionGroup &);        // Not implemented
};

#endif //__pqSierraPlotToolsActionGroup_h
