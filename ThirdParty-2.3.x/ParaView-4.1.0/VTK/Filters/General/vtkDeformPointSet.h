/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDeformPointSet - use a control polyhedron to deform an input vtkPointSet
// .SECTION Description
// vtkDeformPointSet is a filter that uses a control polyhdron to deform an
// input dataset of type vtkPointSet. The control polyhedron (or mesh) must
// be a closed, manifold surface.
//
// The filter executes as follows. In initial pipeline execution, the control
// mesh and input vtkPointSet are assumed in undeformed position, and an
// initial set of interpolation weights are computed for each point in the
// vtkPointSet (one interpolation weight value for each point in the control
// mesh). The filter then stores these interpolation weights after filter
// execution. The next time the filter executes, assuming that the number of
// points/cells in the control mesh and vtkPointSet have not changed, the
// points in the vtkPointSet are recomputed based on the original
// weights. Hence if the control mesh has been deformed, it will in turn
// cause deformation in the vtkPointSet. This can be used to animate or edit
// the geometry of the vtkPointSet.

// .SECTION Caveats
// Note that a set of interpolation weights per point in the vtkPointSet is
// maintained. The number of interpolation weights is the number of points
// in the control mesh. Hence keep the control mesh small in size or a n^2
// data explostion will occur.
//
// The filter maintains interpolation weights between executions (after the
// initial execution pass computes the interpolation weights). You can
// explicitly cause the filter to reinitialize by setting the
// InitializeWeights boolean to true. By default, the filter will execute and
// then set InitializeWeights to false.
//
// This work was motivated by the work of Tao Ju et al in "Mesh Value Coordinates
// for Closed Triangular Meshes." The MVC algorithm is currently used to generate
// interpolation weights. However, in the future this filter may be extended to
// provide other interpolation functions.
//
// A final note: point data and cell data are passed from the input to the output.
// Only the point coordinates of the input vtkPointSet are modified.

// .SECTION See Also
// vtkMeanValueCoordinatesInterpolator vtkProbePolyhedron vtkPolyhedron


#ifndef __vtkDeformPointSet_h
#define __vtkDeformPointSet_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

#include "vtkSmartPointer.h" // For protected ivars

class vtkDoubleArray;
class vtkPolyData;


class VTKFILTERSGENERAL_EXPORT vtkDeformPointSet : public vtkPointSetAlgorithm
{
public:
  // Description:
  // Standard methods for instantiable (i.e., concrete) class.
  static vtkDeformPointSet *New();
  vtkTypeMacro(vtkDeformPointSet,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the control mesh to deform the input vtkPointSet. The control
  // mesh must be a closed, non-self-intersecting, manifold mesh.
  void SetControlMeshData(vtkPolyData *controlMesh);
  vtkPolyData *GetControlMeshData();

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used. New style. Equivalent to SetInputConnection(1, algOutput).
  void SetControlMeshConnection(vtkAlgorithmOutput* algOutput);

  // Description:
  // Specify whether to regenerate interpolation weights or not. Initially
  // the filter will reexecute no matter what this flag is set to (initial
  // weights must be computed). Also, this flag is ignored if the number of
  // input points/cells or the number of control mesh points/cells changes
  // between executions. Thus flag is used to force reexecution and
  // recomputation of weights.
  vtkSetMacro(InitializeWeights, int);
  vtkGetMacro(InitializeWeights, int);
  vtkBooleanMacro(InitializeWeights, int);

protected:
  vtkDeformPointSet();
  ~vtkDeformPointSet();

  int InitializeWeights;

  // Keep track of information between execution passes
  vtkIdType InitialNumberOfControlMeshPoints;
  vtkIdType InitialNumberOfControlMeshCells;
  vtkIdType InitialNumberOfPointSetPoints;
  vtkIdType InitialNumberOfPointSetCells;
  vtkSmartPointer<vtkDoubleArray> Weights;

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);

private:
  vtkDeformPointSet(const vtkDeformPointSet&);  // Not implemented.
  void operator=(const vtkDeformPointSet&);  // Not implemented.

};

#endif
