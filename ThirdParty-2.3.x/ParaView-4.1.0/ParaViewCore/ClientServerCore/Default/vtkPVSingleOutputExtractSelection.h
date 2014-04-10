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
// .NAME vtkPVSingleOutputExtractSelection
// .SECTION Description
// vtkPVSingleOutputExtractSelection extends to vtkPVExtractSelection to simply
// hide the second output-port. This is the filter used in ParaView GUI.

#ifndef __vtkPVSingleOutputExtractSelection_h
#define __vtkPVSingleOutputExtractSelection_h

#include "vtkPVClientServerCoreDefaultModule.h" //needed for exports
#include "vtkPVExtractSelection.h"

class VTKPVCLIENTSERVERCOREDEFAULT_EXPORT vtkPVSingleOutputExtractSelection : public vtkPVExtractSelection
{
public:
  static vtkPVSingleOutputExtractSelection* New();
  vtkTypeMacro(vtkPVSingleOutputExtractSelection, vtkPVExtractSelection);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVSingleOutputExtractSelection();
  ~vtkPVSingleOutputExtractSelection();

private:
  vtkPVSingleOutputExtractSelection(const vtkPVSingleOutputExtractSelection&); // Not implemented
  void operator=(const vtkPVSingleOutputExtractSelection&); // Not implemented
//ETX
};

#endif
