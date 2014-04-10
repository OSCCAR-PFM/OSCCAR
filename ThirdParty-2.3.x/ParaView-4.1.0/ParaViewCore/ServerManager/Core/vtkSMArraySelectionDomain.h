/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArraySelectionDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMArraySelectionDomain - used on properties that allow users to
// select arrays.
// .SECTION Description
// vtkSMArraySelectionDomain is a domain that can be for used for properties
// that allow users to set selection-statuses for multiple arrays (or similar
// items). This is similar to vtkSMArrayListDomain, the only different is that
// vtkSMArrayListDomain is designed to work with data-information obtained
// from the required Input property, while vtkSMArraySelectionDomain depends on
// a required information-only property ("ArrayList") that provides the 
// arrays available.
//
// Supported Required-Property functions:
// \li \c ArrayList : points a string-vector property that produces the
// (array_name, status) tuples. This is typically an information-only property.
#ifndef __vtkSMArraySelectionDomain_h
#define __vtkSMArraySelectionDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMArraySelectionDomain : public vtkSMStringListDomain
{
public:
  static vtkSMArraySelectionDomain* New();
  vtkTypeMacro(vtkSMArraySelectionDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMArraySelectionDomain();
  ~vtkSMArraySelectionDomain();

private:
  vtkSMArraySelectionDomain(const vtkSMArraySelectionDomain&); // Not implemented
  void operator=(const vtkSMArraySelectionDomain&); // Not implemented
};

#endif
