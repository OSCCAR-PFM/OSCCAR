/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLModelViewProjectionMonitor

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLModelViewProjectionMonitor -- A helper for painters that
// tracks state of OpenGL model-view and projection matrices.
//
// .SECTION Description:
// vtkOpenGLModelViewProjectionMonitor -- A helper for painters that
// tracks state of OpenGL model-view and projection matrices. A Painter
// could use this to skip expensive processing that is only needed when
// the model-view or projection matrices change.
//
// this is not intended to be shared. each object should use it's
// own instance of this class. it's intended to be called once per
// render.

#ifndef __vtkOpenGLModelViewProjectionMonitor_H
#define __vtkOpenGLModelViewProjectionMonitor_H

#include "vtkRenderingOpenGLModule.h" // for export macro
#include "vtkObject.h"

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLModelViewProjectionMonitor : public vtkObject
{
public:
  static vtkOpenGLModelViewProjectionMonitor* New();
  vtkTypeMacro(vtkOpenGLModelViewProjectionMonitor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Fetches the current GL state and updates the
  // internal copies of the data. returns true if
  // any of the tracked OpenGL matrices have changed.
  // Typically this is the only function a user needs
  // to call.
  bool StateChanged();

  // Description:
  // Fetch and store OpenGL model view matrix. Note,
  // this is done automatically in SateChanged.
  void Update();

  //BTX
  // Description:
  // Set the matrix data.
  void SetProjection(float *val);
  void SetModelView(float *val);
  //ETX

protected:
  vtkOpenGLModelViewProjectionMonitor() : UpTime(0)
  { this->Initialize(); }

  ~vtkOpenGLModelViewProjectionMonitor(){}

  void Initialize();

private:
  float Projection[16];
  float ModelView[16];
  long long UpTime;

private:
  vtkOpenGLModelViewProjectionMonitor(const vtkOpenGLModelViewProjectionMonitor&); // Not implemented
  void operator=(const vtkOpenGLModelViewProjectionMonitor &); // Not implemented
};

#endif
