/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTileDisplayHelper - used on server side in tile display mode, to
// ensure that the tiles from multiple views are rendered correctly.
// .SECTION Description
// vtkTileDisplayHelper is used on server side in tile display mode, to
// ensure that the tiles from multiple views are rendered correctly. This is
// required since in multi-view configurations, only 1 view is rendered at a
// time, and when a view is being rendered, it may affect the renderings from
// other view which it must restore for the tile display to show the results
// correctly. We use this helper to keep buffered images from all views so that
// they can be restored.

#ifndef __vtkTileDisplayHelper_h
#define __vtkTileDisplayHelper_h

#include "vtkObject.h"
#include "vtkSynchronizedRenderers.h" // needed for vtkRawImage.
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkRenderer;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkTileDisplayHelper : public vtkObject
{
public:
  // Description:
  // Only one instance by process should be used, that's why everyone should
  // use the instance returned by the GetInstance() methods. But to allow
  // that class to be used by a proxy, we need to expose the new.
  // Beaware that a subset of methods will be available to the proxy.
  static vtkTileDisplayHelper* New();
  vtkTypeMacro(vtkTileDisplayHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the singleton.
  static vtkTileDisplayHelper* GetInstance();

  // Description:
  // Register a tile.
  void SetTile(unsigned int key,
    double viewport[4], vtkRenderer* renderer,
    vtkSynchronizedRenderers::vtkRawImage& tile);

  // Description:
  // Erase a tile.
  void EraseTile(unsigned int key);

  // Description:
  // Same as EraseTile() except erases the tile for only the specified eye.
  void EraseTile(unsigned int key, int leftEye);

  // Description:
  // Flush the tiles.
  void FlushTiles(unsigned int key, int leftEye);

  // Description:
  // Set the enabled tiles-set. Only enabled keys are "flushed".
  void ResetEnabledKeys();
  void EnableKey(unsigned int);

  // Description:
  // - Method that can be used by the proxy.
  // - The Path will be used to save the latest flush of the tiles.
  // - Basically for a 2 processes tile display setting you will have two images
  //   dumped following the given pattern: ${DumpImagePath}_${processId}.png
  // - If the DumpImagePath is set to NULL, then no image dump will occurs.
  void SetDumpImagePath(const char* newPath);

//BTX
protected:
  vtkTileDisplayHelper();
  ~vtkTileDisplayHelper();

private:
  vtkTileDisplayHelper(const vtkTileDisplayHelper&); // Not implemented
  void operator=(const vtkTileDisplayHelper&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
