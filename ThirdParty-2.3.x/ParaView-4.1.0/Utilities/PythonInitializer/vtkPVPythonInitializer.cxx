/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h"
#include "pvpythonmodules.h"
#include "vtkUtilitiesPythonInitializerModule.h"

#ifdef PARAVIEW_FREEZE_PYTHON
# include "vtkFrozenParaViewPython.h"
# include <vtksys/SystemTools.hxx>
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void VTKUTILITIESPYTHONINITIALIZER_EXPORT vtkPVInitializePythonModules()
    {
#ifdef PARAVIEW_FREEZE_PYTHON
     // If PYTHONHOME is unset, python attempts to build a reasonable sys.path
     // by inspecting paths relative to the executable among others locations.
     // We set PYTHONHOME to avoid that from happening.
    vtksys::SystemTools::PutEnv("PYTHONHOME=temp");

    // removes an access for locale
    vtksys::SystemTools::PutEnv("LC_CTYPE=C");

    vtkFrozenParaViewPython();
#endif
    CMakeLoadAllPythonModules();
    }
#ifdef __cplusplus
}
#endif
