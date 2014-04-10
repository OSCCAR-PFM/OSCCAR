/*=========================================================================

  Program:   ParaView
  Module:    vtkSIWriterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIWriterProxy
// .SECTION Description
// ServerImplementation for WriterProxy

#ifndef __vtkSIWriterProxy_h
#define __vtkSIWriterProxy_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSISourceProxy.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIWriterProxy : public vtkSISourceProxy
{
public:
  static vtkSIWriterProxy* New();
  vtkTypeMacro(vtkSIWriterProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These methods are called to add/remove input connections by
  // vtkSIInputProperty. This indirection makes it possible for subclasses to
  // insert VTK-algorithms in the input pipeline.
  // Overridden to insert "CompleteArrays" filter in the pipeline.
  virtual void AddInput(int input_port,
    vtkAlgorithmOutput* connection, const char* method);
  virtual void CleanInputs(const char* method);

  // Description:
  // Update the requested time for the pipeline. This needs to be
  // separate than vtkSISourceProxy because there are no output
  // ports to do this on.
  virtual void UpdatePipelineTime(double time);

//BTX
protected:
  vtkSIWriterProxy();
  ~vtkSIWriterProxy();

  // Description:
  // Creates the VTKObjects. Overridden to add post-filters to the pipeline.
  virtual bool CreateVTKObjects(vtkSMMessage* message);

  // Description:
  // Read xml-attributes.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);

  char* FileNameMethod;
  vtkSetStringMacro(FileNameMethod);

private:
  vtkSIWriterProxy(const vtkSIWriterProxy&); // Not implemented
  void operator=(const vtkSIWriterProxy&); // Not implemented
//ETX
};

#endif
