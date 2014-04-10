/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRemoteObjectUpdateUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRemoteObjectUpdateUndoElement - vtkSMRemoteObject undo element.
// .SECTION Description
// This class keeps the before and after state of the RemoteObject in the
// vtkSMMessage form. It works with any proxy and RemoteObject. It is a very
// generic undoElement.

#ifndef __vtkSMRemoteObjectUpdateUndoElement_h
#define __vtkSMRemoteObjectUpdateUndoElement_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMUndoElement.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage
#include "vtkWeakPointer.h" //  needed for vtkWeakPointer.

class vtkSMProxyLocator;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMRemoteObjectUpdateUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMRemoteObjectUpdateUndoElement* New();
  vtkTypeMacro(vtkSMRemoteObjectUpdateUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Undo the operation encapsulated by this element.
  // \return the status of the operation, 1 on success, 0 otherwise.
  virtual int Undo();

  // Description:
  // Redo the operation encaspsulated by this element.
  // \return the status of the operation, 1 on success, 0 otherwise.
  virtual int Redo();

  // Description:
  // Set ProxyLocator to use if any.
  virtual void SetProxyLocator(vtkSMProxyLocator*);

//BTX

  // Description:
  // Set the state of the UndoElement
  virtual void SetUndoRedoState(const vtkSMMessage* before,
                                const vtkSMMessage* after);

  // Current full state of the UndoElement
  vtkSMMessage* BeforeState;
  vtkSMMessage* AfterState;

  virtual vtkTypeUInt32 GetGlobalId();

protected:
  vtkSMRemoteObjectUpdateUndoElement();
  ~vtkSMRemoteObjectUpdateUndoElement();

  // Internal method used to update proxy state based on the state info
  int UpdateState(const vtkSMMessage* state);

  vtkSMProxyLocator* ProxyLocator;

private:
  vtkSMRemoteObjectUpdateUndoElement(const vtkSMRemoteObjectUpdateUndoElement&); // Not implemented.
  void operator=(const vtkSMRemoteObjectUpdateUndoElement&); // Not implemented.

//ETX
};

#endif
