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
// .NAME vtkPVStreamingPiecesInformation - information object used by
// vtkSMDataDeliveryManager to get information about representations that have
// pieces to stream from the data-server.
// .SECTION Description
// vtkPVStreamingPiecesInformation is an information object used by
// vtkSMDataDeliveryManager to get information about representations that have
// pieces to stream from the data-server. 

#ifndef __vtkPVStreamingPiecesInformation_h
#define __vtkPVStreamingPiecesInformation_h

#include "vtkPVInformation.h"
#include "vtkPVClientServerCoreRenderingModule.h" // needed for export macro

//BTX
#include <vector> // needed for internal API
//ETX

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVStreamingPiecesInformation : public vtkPVInformation
{
public:
  static vtkPVStreamingPiecesInformation* New();
  vtkTypeMacro(vtkPVStreamingPiecesInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object. Calls AddInformation(info, 0).
  virtual void AddInformation(vtkPVInformation* info);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

//BTX
  // Description:
  // API to access the internal keys. 
  void GetKeys(std::vector<unsigned int>& keys) const;
//ETX

//BTX
protected:
  vtkPVStreamingPiecesInformation();
  ~vtkPVStreamingPiecesInformation();

private:
  vtkPVStreamingPiecesInformation(const vtkPVStreamingPiecesInformation&); // Not implemented
  void operator=(const vtkPVStreamingPiecesInformation&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
