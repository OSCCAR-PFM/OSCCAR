/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmyCommonWin32Header.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkmyCommonWin32Header - manage Windows system differences
// .SECTION Description
// The vtkmyCommonWin32Header captures some system differences between Unix
// and Windows operating systems.

#ifndef __vtkmyCommonWin32Header_h
#define __vtkmyCommonWin32Header_h

#include <vtkmyConfigure.h>

#if defined(WIN32) && !defined(VTKMY_STATIC)
#if defined(vtkmyCommon_EXPORTS)
#define VTK_MY_COMMON_EXPORT __declspec( dllexport )
#else
#define VTK_MY_COMMON_EXPORT __declspec( dllimport )
#endif
#else
#define VTK_MY_COMMON_EXPORT
#endif

#endif
