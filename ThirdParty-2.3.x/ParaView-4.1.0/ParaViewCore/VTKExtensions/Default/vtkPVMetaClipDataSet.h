/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMetaClipDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMetaClipDataSet -
// Meta class for clip filter that will allow the user to switch between
// a regular clip filter or an extract cell by region filter.

#ifndef __vtkPVMetaClipDataSet_h
#define __vtkPVMetaClipDataSet_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkPVDataSetAlgorithmSelectorFilter.h"

class vtkImplicitFunction;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVMetaClipDataSet : public vtkPVDataSetAlgorithmSelectorFilter
{
public:
  vtkTypeMacro(vtkPVMetaClipDataSet,vtkPVDataSetAlgorithmSelectorFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPVMetaClipDataSet *New();

  // Description:
  // Enable or disable the Extract Cells By Regions.
  void PreserveInputCells(int keepCellAsIs);

  void SetImplicitFunction(vtkImplicitFunction* func);

  void SetInsideOut(int insideOut);

  // Only available for cut -------------

  // Description:
  // Expose method from vtkCutter
  void SetClipFunction(vtkImplicitFunction* func)
  { this->SetImplicitFunction(func); };

  // Description:
  // Expose method from vtkClip
  void SetValue(double value);

  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              int fieldAssociation,
                              const char *name);
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              int fieldAssociation,
                              int fieldAttributeType);
  virtual void SetInputArrayToProcess(int idx, vtkInformation *info);

  virtual void SetInputArrayToProcess(int idx, int port, int connection, const char* fieldName, const char* fieldType);

  // Description:
  // Expose method from vtkClip
  void SetUseValueAsOffset(int);

  // Description:
  // Add validation for active filter so that the vtkExtractGeometry
  // won't be used without ImplicifFuntion being set.
  virtual int ProcessRequest(vtkInformation* request,
    vtkInformationVector** inInfo,
    vtkInformationVector* outInfo);

  // Add validation for active filter so that the vtkExtractGeometry
  // won't be used without ImplicifFuntion being set.
  virtual int ProcessRequest(vtkInformation* request,
    vtkCollection* inInfo,
    vtkInformationVector* outInfo);

protected:
  vtkPVMetaClipDataSet();
  ~vtkPVMetaClipDataSet();

  // Check to see if this filter can do crinkle, return true if
  // we need to switch active filter, so that we can switch back after.
  bool SwitchFilterForCrinkle();

private:
  vtkPVMetaClipDataSet(const vtkPVMetaClipDataSet&);  // Not implemented.
  void operator=(const vtkPVMetaClipDataSet&);  // Not implemented.

  class vtkInternals;
  vtkInternals *Internal;
};

#endif
