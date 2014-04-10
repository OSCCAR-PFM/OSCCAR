/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataSetAlgorithmSelectorFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDataSetAlgorithmSelectorFilter -
// is a generic vtkAlgorithm that allow the user to register
// several vtkAlgorithm to it and be able to switch the active
// one on the fly.
// .SECTION Description
// The idea behind that filter is to merge the usage of any number of existing
// vtk filter and allow to easily switch from one implementation to another
// without changing anything in your pipeline.

#ifndef __vtkPVDataSetAlgorithmSelectorFilter_h
#define __vtkPVDataSetAlgorithmSelectorFilter_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVDataSetAlgorithmSelectorFilter : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkPVDataSetAlgorithmSelectorFilter,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPVDataSetAlgorithmSelectorFilter *New();

  // Description:
  // Register a new filter that can be used underneath in the requestData call.
  // The return value is the index of that registered filter that should be use
  // to activate it later on. (This number can became wrong in case you remove
  // some previous registered filter)
  int RegisterFilter(vtkAlgorithm* filter);

  // Description:
  // UnRegister an existing filter that was previously registered
  void UnRegisterFilter(int index);

  // Description:
  // Remove all the registered filters.
  void ClearFilters();

  // Description:
  // Return the current number of registered filters
  int GetNumberOfFilters();

  // Description:
  // Return the filter that lies at the given index of the filters registration queue.
  vtkAlgorithm* GetFilter(int index);

  // Description:
  // Return the current active filter if any otherwise return NULL
  vtkAlgorithm* GetActiveFilter();

  // Description:
  // Set the active filter based on the given index of the filters registration
  // queue. And return the corresponding active filter.
  virtual vtkAlgorithm* SetActiveFilter(int index);

  // Description:
  // Override GetMTime because we delegate to other filters to do the real work
  unsigned long GetMTime();

  // Description:
  // Forward those methods to the underneath filters
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);

  // Description:
  // Forward those methods to the underneath filters
  virtual int ProcessRequest(vtkInformation* request,
                     vtkCollection* inInfo,
                     vtkInformationVector* outInfo);

protected:
  vtkPVDataSetAlgorithmSelectorFilter();
  ~vtkPVDataSetAlgorithmSelectorFilter();

  virtual int RequestDataObject(vtkInformation *, vtkInformationVector**,
                                vtkInformationVector* outputVector);
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  vtkGetMacro(OutputType, int);
  vtkSetMacro(OutputType, int);
  int OutputType;

private:
  vtkPVDataSetAlgorithmSelectorFilter(const vtkPVDataSetAlgorithmSelectorFilter&);  // Not implemented.
  void operator=(const vtkPVDataSetAlgorithmSelectorFilter&);  // Not implemented.

  class vtkInternals;
  vtkInternals* Internal;
};

#endif


