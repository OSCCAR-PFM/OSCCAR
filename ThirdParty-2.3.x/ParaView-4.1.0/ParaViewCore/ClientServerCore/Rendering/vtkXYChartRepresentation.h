/*=========================================================================

  Program:   ParaView
  Module:    vtkXYChartRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXYChartRepresentation
// .SECTION Description
//

#ifndef __vtkXYChartRepresentation_h
#define __vtkXYChartRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkChartRepresentation.h"

class vtkChartXY;
class vtkXYChartNamedOptions;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkXYChartRepresentation : public vtkChartRepresentation
{
public:
  static vtkXYChartRepresentation* New();
  vtkTypeMacro(vtkXYChartRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the options object. This must be done before any other state is
  // updated.
  void SetOptions(vtkChartNamedOptions *);

  // Description:
  // Provides access to the underlying VTK representation.
  vtkChartXY* GetChart();

  // Description:
  // Set the series to use as the X-axis.
  void SetXAxisSeriesName(const char* name);
  const char* GetXAxisSeriesName();

  // Description:
  // Set whether the index should be used for the x axis.
  void SetUseIndexForXAxis(bool useIndex);

  // Description:
  // Set the chart type, defaults to line chart
  void SetChartType(const char *type);

  // Description:
  // Get the chart type, defaults to line chart, the return value is from the
  // vtkChart enum (anonymous for wrapping).
  int GetChartType();

//BTX
protected:
  vtkXYChartRepresentation();
  ~vtkXYChartRepresentation();

  vtkXYChartNamedOptions *XYOptions;
  // vtkDataRepresentation* SelectionRepresentation;

private:
  vtkXYChartRepresentation(const vtkXYChartRepresentation&); // Not implemented
  void operator=(const vtkXYChartRepresentation&); // Not implemented
//ETX
};

#endif
