/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreeSliceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkThreeSliceFilter - Cut vtkDataSet along 3 planes
// .SECTION Description
// vtkThreeSliceFilter is a filter that slice the input data using 3 plane cut.
// Each axis cut could embed several slices by providing several values.
// As output you will find 4 output ports.
// The output ports are defined as follow:
// - 0: Merge of all the cutter output
// - 1: Output of the first internal vtkCutter filter
// - 2: Output of the second internal vtkCutter filter
// - 3: Output of the third internal vtkCutter filter

#ifndef __vtkThreeSliceFilter_h
#define __vtkThreeSliceFilter_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPolyDataAlgorithm.h"

class vtkAppendPolyData;
class vtkCellData;
class vtkCompositeDataSet;
class vtkCutter;
class vtkDataSet;
class vtkPProbeFilter;
class vtkPlane;
class vtkPointData;
class vtkPointSource;
class vtkPolyData;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkThreeSliceFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkThreeSliceFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function; initial value of 0.0; and
  // generating cut scalars turned off.
  static vtkThreeSliceFilter *New();

  // Description:
  // Override GetMTime because we rely on internal filters that have their own MTime
  unsigned long GetMTime();

  // Description:
  // Set a Slice Normal for a given cutter
  void SetCutNormal(int cutIndex, double normal[3]);

  // Description:
  // Set a slice Origin for a given cutter
  void SetCutOrigin(int cutIndex, double origin[3]);

  // Description:
  // Set a slice value for a given cutter
  void SetCutValue(int cutIndex, int index, double value);

  // Description:
  // Set number of slices for a given cutter
  void SetNumberOfSlice(int cutIndex, int size);

  // Description:
  // Default settings:
  // - reset the plan origin to be (0,0,0)
  // - number of slice for X, Y and Z to be 0
  // - Normal for SliceX=[1,0,0], SliceY=[0,1,0], SliceZ=[0,0,1]
  void SetToDefaultSettings();

  // Description:
  // Set slice Origin for all cutter
  void SetCutOrigins(double origin[3]);

  // Description:
  // Enable to probe the dataset at the given cut origin.
  void EnableProbe(int enable);

  // Description:
  // Return true if any data is available and provide the value as argument
  bool GetProbedPointData(const char* arrayName, double &value);

protected:
  vtkThreeSliceFilter();
  ~vtkThreeSliceFilter();

  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkCutter* Slices[3];
  vtkPlane* Planes[3];
  vtkAppendPolyData* CombinedFilteredInput;
  vtkPProbeFilter* Probe;
  vtkPointSource* PointToProbe;

  void Process(vtkDataSet* input, vtkPolyData* outputs[4], unsigned int compositeIndex);
  void Process(vtkCompositeDataSet* input, vtkCompositeDataSet* outputs[4]);
  void Merge(vtkCompositeDataSet* compositeOutput[4], vtkPolyData* polydataOutput[4]);
  void Append(vtkCompositeDataSet* composite, vtkAppendPolyData* appender);
  void Append(vtkPolyData* polydata, vtkAppendPolyData* appender);

private:
  vtkThreeSliceFilter(const vtkThreeSliceFilter&);  // Not implemented.
  void operator=(const vtkThreeSliceFilter&);  // Not implemented.
};

#endif
