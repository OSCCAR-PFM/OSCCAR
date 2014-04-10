/*=========================================================================

Program:   ParaView
Module:    pvbatch.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pvpython.h" // include this 1st.
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkProcessModule.h"
#ifndef BUILD_SHARED_LIBS
#include "pvStaticPluginsInit.h"
#endif

int main(int argc, char* argv[])
{
#ifndef BUILD_SHARED_LIBS
  paraview_static_plugins_init();
#endif
  return ParaViewPython::Run(vtkProcessModule::PROCESS_BATCH, argc, argv);
}
