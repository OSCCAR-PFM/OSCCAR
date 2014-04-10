/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPYoungsMaterialInterface.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPYoungsMaterialInterface - parallel reconstruction of material interfaces
//
// .SECTION Description
// This is a subclass of vtkYoungsMaterialInterface, implementing the reconstruction
// of material interfaces, for parallel data sets
//
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by <br>
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Implementation by Thierry Carrard and Philippe Pebay
//
// .SECTION See also
// vtkYoungsMaterialInterface

#ifndef __vtkPYoungsMaterialInterface_h
#define __vtkPYoungsMaterialInterface_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkYoungsMaterialInterface.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPYoungsMaterialInterface : public vtkYoungsMaterialInterface
{
public:
  static vtkPYoungsMaterialInterface* New();
  vtkTypeMacro(vtkPYoungsMaterialInterface,vtkYoungsMaterialInterface);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Parallel implementation of the material aggregation.
  virtual void Aggregate ( int, int* );

  // Description:
  // Get/Set the multiprocess controller. If no controller is set,
  // single process is assumed.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkPYoungsMaterialInterface ();
  virtual ~vtkPYoungsMaterialInterface ();

  vtkMultiProcessController* Controller;

private:
  vtkPYoungsMaterialInterface(const vtkPYoungsMaterialInterface&); // Not implemented
  void operator=(const vtkPYoungsMaterialInterface&); // Not implemented
};

#endif /* VTK_PYOUNGS_MATERIAL_INTERFACE_H */
