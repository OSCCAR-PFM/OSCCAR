/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLHierarchicalBoxDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLHierarchicalBoxDataReader - Reader for hierarchical datasets
// (for backwards compatibility).
//
// .SECTION Description
// vtkXMLHierarchicalBoxDataReader is an empty subclass of
// vtkXMLUniformGridAMRReader. This is only for backwards compatibility. Newer
// code should simply use vtkXMLUniformGridAMRReader.
//
// .SECTION Caveats
// The reader supports reading v1.1 and above. For older versions, use
// vtkXMLHierarchicalBoxDataFileConverter.

#ifndef __vtkXMLHierarchicalBoxDataReader_h
#define __vtkXMLHierarchicalBoxDataReader_h

#include "vtkXMLUniformGridAMRReader.h"

class VTKIOXML_EXPORT vtkXMLHierarchicalBoxDataReader :
  public vtkXMLUniformGridAMRReader
{
public:
  static vtkXMLHierarchicalBoxDataReader* New();
  vtkTypeMacro(vtkXMLHierarchicalBoxDataReader,vtkXMLUniformGridAMRReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkXMLHierarchicalBoxDataReader();
  ~vtkXMLHierarchicalBoxDataReader();

private:
  vtkXMLHierarchicalBoxDataReader(const vtkXMLHierarchicalBoxDataReader&);  // Not implemented.
  void operator=(const vtkXMLHierarchicalBoxDataReader&);  // Not implemented.

};

#endif
