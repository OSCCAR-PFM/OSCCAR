/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorSeries.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkColorSeries - stores a list of colors.
//
// .SECTION Description
// The vtkColorSeries stores palettes of colors. There are several default
// palettes (or schemes) available and functions to control several aspects
// of what colors are returned. In essence a color scheme is set and then
// the number of colors and individual color values may be requested.
//
// It is also possible to add schemes beyond the default palettes.
// Whenever \a SetColorScheme is called with a string for which no palette
// already exists, a new, empty palette is created.
// You may then use \a SetNumberOfColors and \a SetColor to populate the
// palette.
// You may not extend default palettes by calling functions that alter
// a scheme; if called while a predefined palette is in use, they
// will create a new non-default scheme and populate it with the current
// palette before continuing.
//
// The "Brewer" palettes are courtesy of
// Cynthia A. Brewer (Dept. of Geography, Pennsylvania State University)
// and present under the Apache License. See the source code for details.

#ifndef __vtkColorSeries_h
#define __vtkColorSeries_h

#include "vtkCommonColorModule.h" // For export macro
#include "vtkObject.h"
#include "vtkColor.h" // Needed for vtkColor[34]ub
#include "vtkStdString.h" // Needed for arguments

class vtkLookupTable;

class VTKCOMMONCOLOR_EXPORT vtkColorSeries : public vtkObject
{
public:
  vtkTypeMacro(vtkColorSeries, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Create a new vtkColorSeries with the SPECTRUM color scheme.
  static vtkColorSeries* New();

//BTX
  // Description:
  // Enum of the available color schemes
  enum {
    /// 7 different hues.
    SPECTRUM = 0,
    /// 6 warm colors (red to yellow).
    WARM,
    /// 7 cool colors (green to purple).
    COOL,
    /// 7 different blues.
    BLUES,
    /// 7 colors from blue to magenta.
    WILD_FLOWER,
    /// 6 colors from green to orange.
    CITRUS,
    /// purple-grey-orange diverging ColorBrewer scheme (11 colors)
    BREWER_DIVERGING_PURPLE_ORANGE_11,
    /// purple-grey-orange diverging ColorBrewer scheme (10 colors)
    BREWER_DIVERGING_PURPLE_ORANGE_10,
    /// purple-grey-orange diverging ColorBrewer scheme (9 colors)
    BREWER_DIVERGING_PURPLE_ORANGE_9,
    /// purple-grey-orange diverging ColorBrewer scheme (8 colors)
    BREWER_DIVERGING_PURPLE_ORANGE_8,
    /// purple-grey-orange diverging ColorBrewer scheme (7 colors)
    BREWER_DIVERGING_PURPLE_ORANGE_7,
    /// purple-grey-orange diverging ColorBrewer scheme (6 colors)
    BREWER_DIVERGING_PURPLE_ORANGE_6,
    /// purple-grey-orange diverging ColorBrewer scheme (5 colors)
    BREWER_DIVERGING_PURPLE_ORANGE_5,
    /// purple-grey-orange diverging ColorBrewer scheme (4 colors)
    BREWER_DIVERGING_PURPLE_ORANGE_4,
    /// purple-grey-orange diverging ColorBrewer scheme (3 colors)
    BREWER_DIVERGING_PURPLE_ORANGE_3,
    /// diverging spectral ColorBrewer scheme (11 colors)
    BREWER_DIVERGING_SPECTRAL_11,
    /// diverging spectral ColorBrewer scheme (10 colors)
    BREWER_DIVERGING_SPECTRAL_10,
    /// diverging spectral ColorBrewer scheme (9 colors)
    BREWER_DIVERGING_SPECTRAL_9,
    /// diverging spectral ColorBrewer scheme (8 colors)
    BREWER_DIVERGING_SPECTRAL_8,
    /// diverging spectral ColorBrewer scheme (7 colors)
    BREWER_DIVERGING_SPECTRAL_7,
    /// diverging spectral ColorBrewer scheme (6 colors)
    BREWER_DIVERGING_SPECTRAL_6,
    /// diverging spectral ColorBrewer scheme (5 colors)
    BREWER_DIVERGING_SPECTRAL_5,
    /// diverging spectral ColorBrewer scheme (4 colors)
    BREWER_DIVERGING_SPECTRAL_4,
    /// diverging spectral ColorBrewer scheme (3 colors)
    BREWER_DIVERGING_SPECTRAL_3,
    /// brown-blue-green diverging ColorBrewer scheme (11 colors)
    BREWER_DIVERGING_BROWN_BLUE_GREEN_11,
    /// brown-blue-green diverging ColorBrewer scheme (10 colors)
    BREWER_DIVERGING_BROWN_BLUE_GREEN_10,
    /// brown-blue-green diverging ColorBrewer scheme (9 colors)
    BREWER_DIVERGING_BROWN_BLUE_GREEN_9,
    /// brown-blue-green diverging ColorBrewer scheme (8 colors)
    BREWER_DIVERGING_BROWN_BLUE_GREEN_8,
    /// brown-blue-green diverging ColorBrewer scheme (7 colors)
    BREWER_DIVERGING_BROWN_BLUE_GREEN_7,
    /// brown-blue-green diverging ColorBrewer scheme (6 colors)
    BREWER_DIVERGING_BROWN_BLUE_GREEN_6,
    /// brown-blue-green diverging ColorBrewer scheme (5 colors)
    BREWER_DIVERGING_BROWN_BLUE_GREEN_5,
    /// brown-blue-green diverging ColorBrewer scheme (4 colors)
    BREWER_DIVERGING_BROWN_BLUE_GREEN_4,
    /// brown-blue-green diverging ColorBrewer scheme (3 colors)
    BREWER_DIVERGING_BROWN_BLUE_GREEN_3,
    /// blue to green sequential ColorBrewer scheme (9 colors)
    BREWER_SEQUENTIAL_BLUE_GREEN_9,
    /// blue to green sequential ColorBrewer scheme (8 colors)
    BREWER_SEQUENTIAL_BLUE_GREEN_8,
    /// blue to green sequential ColorBrewer scheme (7 colors)
    BREWER_SEQUENTIAL_BLUE_GREEN_7,
    /// blue to green sequential ColorBrewer scheme (6 colors)
    BREWER_SEQUENTIAL_BLUE_GREEN_6,
    /// blue to green sequential ColorBrewer scheme (5 colors)
    BREWER_SEQUENTIAL_BLUE_GREEN_5,
    /// blue to green sequential ColorBrewer scheme (4 colors)
    BREWER_SEQUENTIAL_BLUE_GREEN_4,
    /// blue to green sequential ColorBrewer scheme (3 colors)
    BREWER_SEQUENTIAL_BLUE_GREEN_3,
    /// yellow-orange-brown sequential ColorBrewer scheme (9 colors)
    BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_9,
    /// yellow-orange-brown sequential ColorBrewer scheme (8 colors)
    BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_8,
    /// yellow-orange-brown sequential ColorBrewer scheme (7 colors)
    BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_7,
    /// yellow-orange-brown sequential ColorBrewer scheme (6 colors)
    BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_6,
    /// yellow-orange-brown sequential ColorBrewer scheme (5 colors)
    BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_5,
    /// yellow-orange-brown sequential ColorBrewer scheme (4 colors)
    BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_4,
    /// yellow-orange-brown sequential ColorBrewer scheme (3 colors)
    BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_3,
    /// blue to purple sequential ColorBrewer scheme (9 colors)
    BREWER_SEQUENTIAL_BLUE_PURPLE_9,
    /// blue to purple sequential ColorBrewer scheme (8 colors)
    BREWER_SEQUENTIAL_BLUE_PURPLE_8,
    /// blue to purple sequential ColorBrewer scheme (7 colors)
    BREWER_SEQUENTIAL_BLUE_PURPLE_7,
    /// blue to purple sequential ColorBrewer scheme (6 colors)
    BREWER_SEQUENTIAL_BLUE_PURPLE_6,
    /// blue to purple sequential ColorBrewer scheme (5 colors)
    BREWER_SEQUENTIAL_BLUE_PURPLE_5,
    /// blue to purple sequential ColorBrewer scheme (4 colors)
    BREWER_SEQUENTIAL_BLUE_PURPLE_4,
    /// blue to purple sequential ColorBrewer scheme (3 colors)
    BREWER_SEQUENTIAL_BLUE_PURPLE_3,
    /// qualitative ColorBrewer scheme good for accenting
    BREWER_QUALITATIVE_ACCENT,
    /// a dark set of qualitative colors from ColorBrewer
    BREWER_QUALITATIVE_DARK2,
    /// a qualitative ColorBrewer scheme useful for color set members
    BREWER_QUALITATIVE_SET2,
    /// a qualitative ColorBrewer scheme composed of pastel colors
    BREWER_QUALITATIVE_PASTEL2,
    /// a qualitative ColorBrewer scheme composed of pastel colors
    BREWER_QUALITATIVE_PASTEL1,
    /// a qualitative ColorBrewer scheme useful for color set members
    BREWER_QUALITATIVE_SET1,
    /// a qualitative ColorBrewer scheme with pairs of matching colors
    BREWER_QUALITATIVE_PAIRED,
    /// a qualitative ColorBrewer scheme useful for color set members
    BREWER_QUALITATIVE_SET3,
    /// User specified color scheme.
    CUSTOM
    };
//ETX

  // Description:
  // Set the color scheme that should be used.
  // The variant of this function that takes an integer should pass a
  // number from those in the enum, or a value returned by the string variant.
  // The variant that accepts a string returns the integer index
  // of the resulting palette (whether it already existed or is newly-created).
  virtual void SetColorScheme(int scheme);
  virtual int SetColorSchemeByName(const vtkStdString& schemeName);

  // Description:
  // Return the number of schemes currently defined.
  int GetNumberOfColorSchemes() const;

  // Description:
  // Get the color scheme that is currently being used.
  virtual vtkStdString GetColorSchemeName() const;

  // Description:
  // Set the name of the current color scheme
  virtual void SetColorSchemeName(const vtkStdString& scheme);

  // Description:
  // Return the ID of the color scheme currently in use.
  virtual int GetColorScheme() const;

  // Description:
  // Get the number of colors available in the current color scheme.
  virtual int GetNumberOfColors() const;

  // Description:
  // Set the number of colors to be stored in a non-default color scheme.
  // Calling this function on a predefined color scheme will cause the scheme to be duplicated to a new custom scheme.
  virtual void SetNumberOfColors(int numColors);

//BTX
  // Description:
  // Get the color at the specified index. If the index is out of range then
  // black will be returned.
  vtkColor3ub GetColor(int index) const;

  // Description:
  // Get the color at the specified index. If the index is out of range then
  // the call wraps around, i.e. uses the mod operator.
  vtkColor3ub GetColorRepeating(int index) const;

  // Description:
  // Set the color at the specified index. Does nothing if the index is out of
  // range.
  virtual void SetColor(int index, const vtkColor3ub &color);

  // Description:
  // Adds the color to the end of the list.
  virtual void AddColor(const vtkColor3ub &color);

  // Description:
  // Inserts the color at the specified index in the list.
  virtual void InsertColor(int index, const vtkColor3ub &color);
//ETX

  // Description:
  // Removes the color at the specified index in the list.
  virtual void RemoveColor(int index);

  // Description:
  // Clears the list of colors.
  virtual void ClearColors();

  // Description:
  // Make a deep copy of the supplied object.
  virtual void DeepCopy(vtkColorSeries *chartColors);

  // Description:
  // Populate a lookup table with all the colors in the current scheme.
  void BuildLookupTable(vtkLookupTable* lkup);

  // Description:
  // Create a new lookup table with all the colors in the current scheme.
  // The caller is responsible for deleting the table after use.
  vtkLookupTable* CreateLookupTable();

//BTX
protected:
  vtkColorSeries();
  virtual ~vtkColorSeries();

  // Description:
  // If the current scheme is a predefined (read-only) scheme,
  // copy the current colors to a new scheme and modify the new scheme instead.
  virtual void CopyOnWrite();

  // Description:
  // Private data pointer of the class, stores the color list.
  class Private;
  Private* Storage;

  // Description:
  // The color scheme being used.
  int ColorScheme;

  /// The color scheme being used.
  vtkStdString ColorSchemeName;

private:
  vtkColorSeries(const vtkColorSeries &); // Not implemented.
  void operator=(const vtkColorSeries &);   // Not implemented.
//ETX
};

#endif //__vtkColorSeries_h
