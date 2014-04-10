/*=========================================================================

  Program:   ParaView
  Module:    vtkStructuredGridVolumeRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredGridVolumeRepresentation - representation for showing
// vtkStructuredGrid datasets as volumes.
// .SECTION Description
// vtkStructuredGridVolumeRepresentation is a representation for volume
// rendering vtkStructuredGrid datasets with one caveat: it assumes that the
// structured grid is not "curved" i.e. bounding boxes of non-intersecting
// extents don't intersect (or intersect negligibly). This is the default (and
// faster) method. Alternatively, one can set UseDataParititions to
// off and the representation will simply reply on the view to build the sorting
// order using the unstructured grid. In which case, however data will be
// transferred among processing.

#ifndef __vtkStructuredGridVolumeRepresentation_h
#define __vtkStructuredGridVolumeRepresentation_h

#include "vtkUnstructuredGridVolumeRepresentation.h"

class vtkTableExtentTranslator;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkStructuredGridVolumeRepresentation :
  public vtkUnstructuredGridVolumeRepresentation
{
public:
  static vtkStructuredGridVolumeRepresentation* New();
  vtkTypeMacro(vtkStructuredGridVolumeRepresentation, vtkUnstructuredGridVolumeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When on (default), the representation tells the view to use the
  // partitioning information from the input structured grid for ordered
  // compositing. When off we let the view build its own ordering and
  // redistribute data as needed.
  void SetUseDataParititions(bool);
  vtkGetMacro(UseDataParititions, bool);

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

//BTX
protected:
  vtkStructuredGridVolumeRepresentation();
  ~vtkStructuredGridVolumeRepresentation();

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);

  bool UseDataParititions;
  vtkTableExtentTranslator* TableExtentTranslator;
private:
  vtkStructuredGridVolumeRepresentation(const vtkStructuredGridVolumeRepresentation&); // Not implemented
  void operator=(const vtkStructuredGridVolumeRepresentation&); // Not implemented
//ETX
};

#endif
