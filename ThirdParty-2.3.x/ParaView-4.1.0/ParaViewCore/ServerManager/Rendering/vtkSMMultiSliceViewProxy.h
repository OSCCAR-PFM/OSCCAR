/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiSliceViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMultiSliceViewProxy
// .SECTION Description
// Custom RenderViewProxy to override CreateDefaultRepresentation method
// so only the Multi-Slice representation will be available to the user

#ifndef __vtkSMMultiSliceViewProxy_h
#define __vtkSMMultiSliceViewProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMRenderViewProxy.h"

class vtkSMSourceProxy;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMMultiSliceViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMMultiSliceViewProxy* New();
  vtkTypeMacro(vtkSMMultiSliceViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*, int);

  // Description:
  // Similar to IsSelectionAvailable(), however, on failure returns the
  // error message otherwise 0.
  virtual const char* IsSelectVisiblePointsAvailable();

//BTX
protected:
  vtkSMMultiSliceViewProxy();
  ~vtkSMMultiSliceViewProxy();

  // Description:
  // Use the center of the source to initialize the view with three orthogonal
  // slices in x, y, z.
  void InitDefaultSlices(vtkSMSourceProxy* source, int opport);

private:
  vtkSMMultiSliceViewProxy(const vtkSMMultiSliceViewProxy&); // Not implemented
  void operator=(const vtkSMMultiSliceViewProxy&); // Not implemented
//ETX
};

#endif
