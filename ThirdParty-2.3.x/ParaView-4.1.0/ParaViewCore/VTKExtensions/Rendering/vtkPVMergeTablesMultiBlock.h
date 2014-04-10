/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMergeTablesMultiBlock.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMergeTablesMultiBlock - used to merge rows in tables.
// .SECTION Description
// Simplified version of vtkMergeTables which simply combines tables merging
// columns. This assumes that each of the inputs either has exactly identical
// columns or no columns at all.
// This filter can handle composite datasets as well. The output is produced by
// merging corresponding leaf nodes. This assumes that all inputs have the same
// composite structure.
// All inputs must either be vtkTable or vtkCompositeDataSet mixing is not
// allowed.
// The output is a multiblock dataset of vtkTable.
// .SECTION TODO
// We may want to consolidate with vtkPVMergeTable somehow

#ifndef __vtkPVMergeTablesMultiBlock_h
#define __vtkPVMergeTablesMultiBlock_h

#include "vtkAlgorithm.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVMergeTablesMultiBlock : public vtkAlgorithm
{
public:
  static vtkPVMergeTablesMultiBlock* New();
  vtkTypeMacro(vtkPVMergeTablesMultiBlock, vtkAlgorithm);
  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVMergeTablesMultiBlock();
  ~vtkPVMergeTablesMultiBlock();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual vtkExecutive* CreateDefaultExecutive();

private:
  vtkPVMergeTablesMultiBlock(const vtkPVMergeTablesMultiBlock&); // Not implemented
  void operator=(const vtkPVMergeTablesMultiBlock&); // Not implemented
//ETX
};

#endif
