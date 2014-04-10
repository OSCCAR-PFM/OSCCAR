/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderbuffer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRenderbuffer - Storage for FBO's
// .SECTION Description
// Lightweight API to OpenGL Framebuffer Object EXT renderbuffers.
#ifndef __vtkRenderbuffer_h
#define __vtkRenderbuffer_h

#include "vtkObject.h"
#include "vtkRenderingOpenGLModule.h" // for export macro
#include "vtkWeakPointer.h" // for render context

class vtkRenderWindow;
class vtkTextureObject;

class VTKRENDERINGOPENGL_EXPORT vtkRenderbuffer : public vtkObject
{
public:
  static vtkRenderbuffer* New();
  vtkTypeMacro(vtkRenderbuffer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns if the context supports the required extensions.
  // Extension will be loaded when the conetxt is set.
  static bool IsSupported(vtkRenderWindow *renWin);

  // Description:
  // Get the name of the buffer for use opengl code.
  vtkGetMacro(Handle, unsigned int);

  // Description:
  // Setting the context has the side affect of initializing OpenGL
  // required extensions and allocates an OpenGL name(handle) that is
  // released when the object is destroyed. NOTE: the reference count
  // to the passed in object is not incremented. Contex must be set
  // prior to other use.
  void SetContext(vtkRenderWindow *win);
  vtkRenderWindow* GetContext();

  // Description:
  // Sets up an RGBAF renderbufffer for use as a color attachment. Use mode
  // to control READ or DRAW operation.
  int CreateColorAttachment(
        unsigned int width,
        unsigned int height);

  // Description:
  // Sets up an DEPTH renderbufffer for use as a color attachment. Use mode
  // to control READ or DRAW operation.
  int CreateDepthAttachment(
        unsigned int width,
        unsigned int height);

  // Description:
  // Sets up an renderbufffer. Use mode to control READ or DRAW operation and
  // format to control the internal format. (see OpenGL doc for more info)
  int Create(
        unsigned int format,
        unsigned int width,
        unsigned int height);

protected:
  vtkRenderbuffer();
  ~vtkRenderbuffer();

  bool LoadRequiredExtensions(vtkRenderWindow *renWin);
  void Alloc();
  void Free();

  int DepthBufferFloat;

private:
  unsigned int Handle;
  vtkWeakPointer<vtkRenderWindow> Context;

private:
  vtkRenderbuffer(const vtkRenderbuffer&); // Not implemented.
  void operator=(const vtkRenderbuffer&); // Not implemented.
};

#endif
