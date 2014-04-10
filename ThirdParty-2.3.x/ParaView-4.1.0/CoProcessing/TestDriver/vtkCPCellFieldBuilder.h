/*=========================================================================

  Program:   ParaView
  Module:    vtkCPCellFieldBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPCellFieldBuilder - Class for specifying cell fields over grids.
// .SECTION Description
// Class for specifying cell data fields over grids for a test driver.  

#ifndef __vtkCPCellFieldBuilder_h
#define __vtkCPCellFieldBuilder_h

#include "vtkCPFieldBuilder.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPCellFieldBuilder : public vtkCPFieldBuilder
{
public:
  static vtkCPCellFieldBuilder* New();
  vtkTypeMacro(vtkCPCellFieldBuilder, vtkCPFieldBuilder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a field on Grid. 
  virtual void BuildField(unsigned long TimeStep, double Time,
                          vtkDataSet* Grid);

  // Description:
  // Return the highest order of discretization of the field.
  //virtual unsigned int GetHighestFieldOrder();

protected:
  vtkCPCellFieldBuilder();
  ~vtkCPCellFieldBuilder();

private:
  vtkCPCellFieldBuilder(const vtkCPCellFieldBuilder&); // Not implemented
  void operator=(const vtkCPCellFieldBuilder&); // Not implemented
};

#endif
