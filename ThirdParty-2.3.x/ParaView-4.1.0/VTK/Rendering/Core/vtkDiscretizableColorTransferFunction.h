/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiscretizableColorTransferFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDiscretizableColorTransferFunction - a combination of vtkColorTransferFunction and
// vtkLookupTable.
// .SECTION Description
// This is a cross between a vtkColorTransferFunction and a vtkLookupTable
// selectively combining the functionality of both. This class is a
// vtkColorTransferFunction allowing users to specify the RGB control points
// that control the color transfer function. At the same time, by setting
// \a Discretize to 1 (true), one can force the transfer function to only have
// \a NumberOfValues discrete colors.
//
// When \a IndexedLookup is true, this class behaves differently. The annotated
// valyes are considered to the be only valid values for which entries in the
// color table should be returned. The colors for annotated values are those
// specified using \a AddIndexedColors. Typically, there must be atleast as many
// indexed colors specified as the annotations. For backwards compatibility, if
// no indexed-colors are specified, the colors in the lookup \a Table are assigned
// to annotated values by taking the modulus of their index in the list
// of annotations. If a scalar value is not present in \a AnnotatedValues,
// then \a NanColor will be used.
//
// NOTE: One must call Build() after making any changes to the points
// in the ColorTransferFunction to ensure that the discrete and non-discrete
// versions match up.


#ifndef __vtkDiscretizableColorTransferFunction_h
#define __vtkDiscretizableColorTransferFunction_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkColorTransferFunction.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

class vtkLookupTable;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class VTKRENDERINGCORE_EXPORT vtkDiscretizableColorTransferFunction : public vtkColorTransferFunction
{
public:
  static vtkDiscretizableColorTransferFunction* New();
  vtkTypeMacro(vtkDiscretizableColorTransferFunction, vtkColorTransferFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  int IsOpaque();

  // Description:
  // Add colors to use when \a IndexedLookup is true.
  // \a SetIndexedColor() will automatically call
  // SetNumberOfIndexedColors(index+1) if the current number of indexed colors
  // is not sufficient for the specified index and all will be initialized to
  // with the rgb values passed to this call.
  void SetIndexedColor(unsigned int index, const double rgb[3])
    { this->SetIndexedColor(index, rgb[0], rgb[1], rgb[2]); }
  void SetIndexedColor(unsigned int index, double r, double g, double b);

  /** Get the "indexed color" assigned to an index.
   *
   * The index is used in \a IndexedLookup mode to assign colors to annotations (in the order
   * the annotations were set).
   * Subclasses must implement this and interpret how to treat the index.
   * vtkLookupTable simply returns GetTableValue(\a index % \a this->GetNumberOfTableValues()).
   * vtkColorTransferFunction returns the color assocated with node \a index % \a this->GetSize().
   *
   * Note that implementations *must* set the opacity (alpha) component of the color, even if they
   * do not provide opacity values in their colormaps. In that case, alpha = 1 should be used.
   */
  virtual void GetIndexedColor(vtkIdType i, double rgba[4]);

  // Description:
  // Set the number of indexed colors. These are used when IndexedLookup is
  // true. If no indexed colors are specified, for backwards compatibility,
  // this class reverts to using the RGBPoints for colors.
  void SetNumberOfIndexedColors(unsigned int count);
  unsigned int GetNumberOfIndexedColors();

  // Description:
  // Generate discretized lookup table, if applicable.
  // This method must be called after changes to the ColorTransferFunction
  // otherwise the discretized version will be inconsistent with the
  // non-discretized one.
  virtual void Build();

  // Description:
  // Set if the values are to mapped after discretization. The
  // number of discrete values is set by using SetNumberOfValues().
  // Not set by default, i.e. color value is determined by
  // interpolating at the scalar value.
  vtkSetMacro(Discretize, int);
  vtkGetMacro(Discretize, int);
  vtkBooleanMacro(Discretize, int);

  // Description:
  // Get/Set if log scale must be used while mapping scalars
  // to colors. The default is 0.
  virtual void SetUseLogScale(int useLogScale);
  vtkGetMacro(UseLogScale, int);

  // Description:
  // Set the number of values i.e. colors to be generated in the
  // discrete lookup table. This has no effect if Discretize is off.
  // The default is 256.
  vtkSetMacro(NumberOfValues, vtkIdType);
  vtkGetMacro(NumberOfValues, vtkIdType);

  // Description:
  // Map one value through the lookup table and return a color defined
  // as a RGBA unsigned char tuple (4 bytes).
  virtual unsigned char *MapValue(double v);

  // Description:
  // Map one value through the lookup table and return the color as
  // an RGB array of doubles between 0 and 1.
  virtual void GetColor(double v, double rgb[3]);

  // Description:
  // Return the opacity of a given scalar.
  virtual double GetOpacity(double v);

  // Description:
  // An internal method maps a data array into a 4-component, unsigned char
  // RGBA array. The color mode determines the behavior of mapping. If
  // VTK_COLOR_MODE_DEFAULT is set, then unsigned char data arrays are
  // treated as colors (and converted to RGBA if necessary); otherwise,
  // the data is mapped through this instance of ScalarsToColors. The offset
  // is used for data arrays with more than one component; it indicates
  // which component to use to do the blending.
  // When the component argument is -1, then the this object uses its
  // own selected technique to change a vector into a scalar to map.
  //
  // When \a IndexedLookup (inherited from vtkScalarsToColors) is true,
  // the scalar opacity function is not used regardless of
  // \a EnableOpacityMapping.
  virtual vtkUnsignedCharArray *MapScalars(vtkDataArray *scalars, int colorMode,
                                   int component);

  // Description:
  // Returns the (x, r, g, b) values as an array.
  double* GetRGBPoints();

  // Description:
  // Specify an additional opacity (alpha) value to blend with. Values
  // != 1 modify the resulting color consistent with the requested
  // form of the output. This is typically used by an actor in order to
  // blend its opacity.
  // Overridden to pass the alpha to the internal vtkLookupTable.
  virtual void SetAlpha(double alpha);


  // Description:
  // Set the color to use when a NaN (not a number) is encountered.  This is an
  // RGB 3-tuple color of doubles in the range [0,1].
  // Overridden to pass the NanColor to the internal vtkLookupTable.
  virtual void SetNanColor(double r, double g, double b);
  virtual void SetNanColor(double rgb[3]) {
    this->SetNanColor(rgb[0], rgb[1], rgb[2]);
  }


  // Description:
  // This should return 1 is the subclass is using log scale for mapping scalars
  // to colors.
  virtual int UsingLogScale()
    { return this->UseLogScale; }

  // Description:
  // Get the number of available colors for mapping to.
  virtual vtkIdType GetNumberOfAvailableColors();

  // Description:
  // Set/get the opacity function to use.
  virtual void SetScalarOpacityFunction(vtkPiecewiseFunction *function);
  virtual vtkPiecewiseFunction* GetScalarOpacityFunction() const;

  // Description:
  // Enable/disable the usage of the scalar opacity function.
  vtkSetMacro(EnableOpacityMapping, bool)
  vtkGetMacro(EnableOpacityMapping, bool)
  vtkBooleanMacro(EnableOpacityMapping, bool)

  // Description:
  // Overridden to include the ScalarOpacityFunction's MTime.
  virtual unsigned long GetMTime();

protected:
  vtkDiscretizableColorTransferFunction();
  ~vtkDiscretizableColorTransferFunction();

  int Discretize;
  int UseLogScale;

  vtkIdType NumberOfValues;
  vtkLookupTable* LookupTable;

  vtkTimeStamp BuildTime;

  bool EnableOpacityMapping;
  vtkSmartPointer<vtkPiecewiseFunction> ScalarOpacityFunction;
  unsigned long ScalarOpacityFunctionObserverId;

  void MapDataArrayToOpacity(
    vtkDataArray *scalars, int component, vtkUnsignedCharArray* colors);

private:
  vtkDiscretizableColorTransferFunction(const vtkDiscretizableColorTransferFunction&); // Not implemented.
  void operator=(const vtkDiscretizableColorTransferFunction&); // Not implemented.
  template<typename T, typename VectorGetter>
    void MapVectorToOpacity (
      VectorGetter getter, T* scalars, int component,
      int numberOfComponents, vtkIdType numberOfTuples, unsigned char* colors);
  template<template<class> class VectorGetter>
    void AllTypesMapVectorToOpacity (
      int scalarType,
      void* scalarsPtr, int component,
      int numberOfComponents, vtkIdType numberOfTuples, unsigned char* colors);


  // Pointer used by GetRGBPoints().
  double* Data;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
