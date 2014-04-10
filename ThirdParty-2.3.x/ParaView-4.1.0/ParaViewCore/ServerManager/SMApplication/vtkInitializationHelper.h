
/*=========================================================================

  Program:   ParaView
  Module:    vtkInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInitializationHelper - help class for python modules
// .SECTION Description
// This class is used by the python modules when they are loaded from
// python (as opposed to pvpython). It simply initializes the server
// manager so that it can be used.

#ifndef __vtkInitializationHelper_h
#define __vtkInitializationHelper_h

#include "vtkObject.h"
#include "vtkPVServerManagerApplicationModule.h" // needed for exports
class vtkPVOptions;

class VTKPVSERVERMANAGERAPPLICATION_EXPORT vtkInitializationHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkInitializationHelper,vtkObject);
  void PrintSelf(ostream&, vtkIndent);

  // Description:
  // Initializes the server manager. Do not use the server manager
  // before calling this.
  static void Initialize(const char* executable, int type);
  static void Initialize(const char* executable, int type, vtkPVOptions* options);

  // Description:
  // Alternative API to initialize the server manager. This takes in  the
  // command line arguments and the vtkPVOptions instance to use to process the
  // command line options.
  static void Initialize(int argc, char**argv, int type, vtkPVOptions* options);

  // Description:
  // Finalizes the server manager. Do not use the server manager
  // after calling this.
  static void Finalize();

  // Description:
  // Initialization for standalone executables linking against a PV
  // library. This is needed to insure that linker does not remove object
  // factories' auto init during static linking. It also cleans up after
  // protobuf.
  static void StandaloneInitialize();
  static void StandaloneFinalize();

protected:
  vtkInitializationHelper() {};
  virtual ~vtkInitializationHelper() {};

private:
  vtkInitializationHelper(const vtkInitializationHelper&); // Not implemented
  void operator=(const vtkInitializationHelper&); // Not implemented
};

#endif
