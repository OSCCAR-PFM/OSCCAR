/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtractArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtractArraysOverTime - extract point or cell data over time (parallel)
// .SECTION Description
// vtkPVExtractArraysOverTime is a subclass of vtkPExtractArraysOverTime
// that overrides the default SelectionExtractor with a vtkPVExtractSelection
// instance.
// This enables query selections to be extracted at each time step.
// .SECTION See Also
// vtkExtractArraysOverTime
// vtkPExtractArraysOverTime

#ifndef __vtkPVExtractArraysOverTime_h
#define __vtkPVExtractArraysOverTime_h

#include "vtkPVClientServerCoreCoreModule.h" // For export macro
#include "vtkPExtractArraysOverTime.h"

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVExtractArraysOverTime : public vtkPExtractArraysOverTime
{
public:
  static vtkPVExtractArraysOverTime* New();
  vtkTypeMacro(vtkPVExtractArraysOverTime,vtkPExtractArraysOverTime);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVExtractArraysOverTime();
  ~vtkPVExtractArraysOverTime();

private:
  vtkPVExtractArraysOverTime(const vtkPVExtractArraysOverTime&);  // Not implemented.
  void operator=(const vtkPVExtractArraysOverTime&);  // Not implemented.
};

#endif // __vtkPVExtractArraysOverTime_h
