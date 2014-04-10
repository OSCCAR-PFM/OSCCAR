/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionCoreInterpreterHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSessionCoreInterpreterHelper
// .SECTION Description
// vtkPVSessionCoreInterpreterHelper is used by vtkPVSessionCore to avoid a
// circular reference between the vtkPVSessionCore instance and its Interpreter.

#ifndef __vtkPVSessionCoreInterpreterHelper_h
#define __vtkPVSessionCoreInterpreterHelper_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkObject;
class vtkSIObject;
class vtkPVProgressHandler;
class vtkProcessModule;
class vtkPVSessionCore;
class vtkMPIMToNSocketConnection;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkPVSessionCoreInterpreterHelper : public vtkObject
{
public:
  static vtkPVSessionCoreInterpreterHelper* New();
  vtkTypeMacro(vtkPVSessionCoreInterpreterHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the vtkSIObject for the global-id. This is used by SIOBJECT() and
  // SIPROXY() stream (vtkClientServerStream) manipulator macros.
  vtkSIObject* GetSIObject(vtkTypeUInt32 gid);

  // Description:
  // Returns the vtkObject corresponding to the global id. This is used by the
  // VTKOBJECT() stream (vtkClientServerStream) manipulator macros.
  vtkObjectBase* GetVTKObject(vtkTypeUInt32 gid);

  // Description:
  // Reserve a global id block.
  vtkTypeUInt32 GetNextGlobalIdChunk(vtkTypeUInt32 chunkSize);

  // Description:
  // Provides access to the process module.
  vtkProcessModule* GetProcessModule();

  // Description:
  // Provides access to the progress handler.
  vtkPVProgressHandler* GetActiveProgressHandler();

  // Description:
  // Sets and initializes the MPIMToNSocketConnection for communicating between
  // data-server and render-server.
  void SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection*);

  // Description:
  // Used by vtkPVSessionCore to pass the core. This is not reference counted.
  void SetCore(vtkPVSessionCore*);

  // Description:
  // Switch from 0:vtkErrorMacro to 1:vtkWarningMacro
  vtkSetMacro(LogLevel, int);

//BTX
protected:
  vtkPVSessionCoreInterpreterHelper();
  ~vtkPVSessionCoreInterpreterHelper();

  vtkWeakPointer<vtkPVSessionCore> Core;
  int LogLevel;
private:
  vtkPVSessionCoreInterpreterHelper(const vtkPVSessionCoreInterpreterHelper&); // Not implemented
  void operator=(const vtkPVSessionCoreInterpreterHelper&); // Not implemented
//ETX
};

#endif
