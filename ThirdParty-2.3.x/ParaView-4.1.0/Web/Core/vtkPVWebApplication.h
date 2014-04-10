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
// .NAME vtkPVWebApplication - defines ParaViewWeb application interface.
// .SECTION Description
// vtkPVWebApplication defines the core interface for a ParaViewWeb application.
// This exposes methods that make it easier to manage views and rendered images
// from views.

#ifndef __vtkPVWebApplication_h
#define __vtkPVWebApplication_h

#include "vtkObject.h"
#include "vtkParaViewWebCoreModule.h" // needed for exports

class vtkUnsignedCharArray;
class vtkSMViewProxy;
class vtkWebInteractionEvent;

class VTKPARAVIEWWEBCORE_EXPORT vtkPVWebApplication : public vtkObject
{
public:
  static vtkPVWebApplication* New();
  vtkTypeMacro(vtkPVWebApplication, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the encoding to be used for rendered images.
  enum
    {
    ENCODING_NONE=0,
    ENCODING_BASE64=1
    };
  vtkSetClampMacro(ImageEncoding, int, ENCODING_NONE, ENCODING_BASE64);
  vtkGetMacro(ImageEncoding, int);

  // Description:
  // Set the compression to be used for rendered images.
  enum
    {
    COMPRESSION_NONE=0,
    COMPRESSION_PNG=1,
    COMPRESSION_JPEG=2
    };
  vtkSetClampMacro(ImageCompression, int, COMPRESSION_NONE, COMPRESSION_JPEG);
  vtkGetMacro(ImageCompression, int);

  // Description:
  // Render a view and obtain the rendered image.
  vtkUnsignedCharArray* StillRender(vtkSMViewProxy* view, int quality = 100);
  vtkUnsignedCharArray* InteractiveRender(vtkSMViewProxy* view, int quality = 50);
  const char* StillRenderToString(vtkSMViewProxy* view, unsigned long time = 0, int quality = 100);

  // Description:
  // StillRenderToString() need not necessary returns the most recently rendered
  // image. Use this method to get whether there are any pending images being
  // processed concurrently.
  bool GetHasImagesBeingProcessed(vtkSMViewProxy*);

  // Description:
  // Communicate mouse interaction to a view.
  // Returns true if the interaction changed the view state, otherwise returns false.
  bool HandleInteractionEvent(
    vtkSMViewProxy* view, vtkWebInteractionEvent* event);

  // Description:
  // Invalidate view cache
  void InvalidateCache(vtkSMViewProxy* view);

  // Description:
  // Return the MTime of the last array exported by StillRenderToString.
  vtkGetMacro(LastStillRenderToStringMTime, unsigned long);

  // Description:
  // Return the Meta data description of the input scene in JSON format.
  // This is using the vtkWebGLExporter to parse the scene.
  // NOTE: This should be called before getting the webGL binary data.
  const char* GetWebGLSceneMetaData(vtkSMViewProxy* view);

  // Description:
  // Return the binary data given the part index
  // and the webGL object piece id in the scene.
  const char* GetWebGLBinaryData(
    vtkSMViewProxy* view, const char* id, int partIndex);

//BTX
protected:
  vtkPVWebApplication();
  ~vtkPVWebApplication();

  int ImageEncoding;
  int ImageCompression;
  unsigned long LastStillRenderToStringMTime;

private:
  vtkPVWebApplication(const vtkPVWebApplication&); // Not implemented
  void operator=(const vtkPVWebApplication&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;

//ETX
};

#endif
