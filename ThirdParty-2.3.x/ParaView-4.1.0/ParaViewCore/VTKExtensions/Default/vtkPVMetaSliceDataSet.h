/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMetaSliceDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMetaSliceDataSet -
// Meta class for slice filter that will allow the user to switch between
// a regular cutter filter or an extract cell by region filter.

#ifndef __vtkPVMetaSliceDataSet_h
#define __vtkPVMetaSliceDataSet_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkPVDataSetAlgorithmSelectorFilter.h"

class vtkImplicitFunction;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVMetaSliceDataSet : public vtkPVDataSetAlgorithmSelectorFilter
{
public:
  vtkTypeMacro(vtkPVMetaSliceDataSet,vtkPVDataSetAlgorithmSelectorFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPVMetaSliceDataSet *New();

  // Description:
  // Enable or disable the Extract Cells By Regions.
  void PreserveInputCells(int keepCellAsIs);

  // Description:
  // Override it so we can change the output type of the filter
  virtual vtkAlgorithm* SetActiveFilter(int index);

  void SetImplicitFunction(vtkImplicitFunction* func);

  // Only available for cut -------------

  // Description:
  // Expose method from vtkCutter
  void SetCutFunction(vtkImplicitFunction* func)
  { this->SetImplicitFunction(func); };

  // Description:
  // Expose method from vtkCutter
  void SetNumberOfContours(int nbContours);

  // Description:
  // Expose method from vtkCutter
  void SetValue(int index, double value);

  // Description:
  // Expose method from vtkCutter
  void SetGenerateTriangles(int status);


protected:
  vtkPVMetaSliceDataSet();
  ~vtkPVMetaSliceDataSet();

private:
  vtkPVMetaSliceDataSet(const vtkPVMetaSliceDataSet&);  // Not implemented.
  void operator=(const vtkPVMetaSliceDataSet&);  // Not implemented.

  class vtkInternals;
  vtkInternals *Internal;
};

#endif
