/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRemoteObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRemoteObject - baseclass for all proxy-objects that have counter
// parts on server as well as client processes.
// .SECTION Description
// Abstract class involved in ServerManager class hierarchy that has a
// corresponding SIObject which can be local or remote.

#ifndef __vtkSMRemoteObject_h
#define __vtkSMRemoteObject_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMSessionObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage
#include "vtkWeakPointer.h" // needed for vtkWeakPointer

class vtkClientServerStream;
class vtkSMSession;
class vtkSMProxyLocator;
class vtkSMLoadStateContext;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMRemoteObject : public vtkSMSessionObject
{
// My friends are...
  friend class vtkSMStateHelper;    // To pull state
  friend class vtkSMStateLoader;    // To set GlobalId as the originals

public:
  vtkTypeMacro(vtkSMRemoteObject,vtkSMSessionObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the location where the underlying VTK-objects are created. The
  // value can be contructed by or-ing vtkSMSession::ServerFlags
  vtkSetMacro(Location, vtkTypeUInt32);
  vtkGetMacro(Location, vtkTypeUInt32);

  // Description:
  // Override the SetSession so if the object already have an ID
  // we automatically register it to the associated session
  virtual void SetSession(vtkSMSession*);

  // Description:
  // Get the global unique id for this object. If none is set and the session is
  // valid, a new global id will be assigned automatically.
  virtual vtkTypeUInt32 GetGlobalID();
  const char* GetGlobalIDAsString();

  // Description:
  // Allow the user to test if the RemoteObject has already a GlobalID without
  // assigning a new one to it.
  bool HasGlobalID();

  // Description:
  // Allow user to set the remote object to be discard for Undo/Redo
  // action. By default, any remote object is Undoable.
  vtkBooleanMacro(Prototype, bool);
  bool IsPrototype() {return this->Prototype;}
  vtkSetMacro(Prototype, bool);

//BTX

  // Description:
  // This method return the full object state that can be used to create that
  // object from scratch.
  // This method will be used to fill the undo stack.
  // If not overriden this will return NULL.
  virtual const vtkSMMessage* GetFullState()
    { return NULL; }

  // Description:
  // This method is used to initialise the object to the given state
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalID set. This allow to split the load process in 2 step to prevent
  // invalid state when property refere to a sub-proxy that does not exist yet.
  virtual void LoadState( const vtkSMMessage* msg, vtkSMProxyLocator* locator)
    {
    (void) msg;
    (void) locator;
    }

  // Description:
  // Allow to switch off any push of state change to the server for that
  // particular object.
  // This is used when we load a state based on a server notification. In that
  // particular case, the server is already aware of that new state, so we keep
  // those changes local.
  virtual void EnableLocalPushOnly();

  // Description:
  // Enable the given remote object to communicate its state normaly to the
  // server location.
  virtual void DisableLocalPushOnly();

  // Description:
  // Let the session be aware that even if the Location is client only,
  // the message should not be send to the server for a general broadcast
  virtual bool IsLocalPushOnly() { return this->ClientOnlyLocationFlag; }

protected:
  // Description:
  // Default constructor.
  vtkSMRemoteObject();

  // Description:
  // Destructor.
  virtual ~vtkSMRemoteObject();

  // Description:
  // Subclasses can call this method to send a message to its state
  // object on  the server processes specified.
  void PushState(vtkSMMessage* msg);

  // Description:
  // Subclasses can call this method to pull the state from the
  // state-object on the server processes specified. Returns true on successful
  // fetch. The message is updated with the fetched state.
  bool PullState(vtkSMMessage* msg);

  // Description:
  // Set the GlobalUniqueId
  void SetGlobalID(vtkTypeUInt32 guid);

  // Global-ID for this vtkSMRemoteObject. This is assigned when needed.
  // Assigned at :
  // - First push
  // - or when the RemoteObject is created by the ProcessModule remotely.
  // - or when state is loaded from protobuf messages
  vtkTypeUInt32 GlobalID;

  // Location flag identify the processes on which the vtkSIObject
  // corresponding to this vtkSMRemoteObject exist.
  vtkTypeUInt32 Location;

  // Allow remote object to be discard for any state management such as
  // Undo/Redo, Register/UnRegister (in ProxyManager) and so on...
  bool Prototype;

  // Field that store the Disable/EnableLocalPushOnly() state information
  bool ClientOnlyLocationFlag;

  // Convenient method used to return either the local Location or a filtered
  // version of it based on the ClientOnlyLocationFlag
  vtkTypeUInt32 GetFilteredLocation();

private:
  vtkSMRemoteObject(const vtkSMRemoteObject&); // Not implemented
  void operator=(const vtkSMRemoteObject&);       // Not implemented

  char* GlobalIDString;
//ETX
};

// This defines a manipulator for the vtkClientServerStream that can be used on
// the to indicate to the interpreter that the placeholder is to be replaced by
// the vtkSIProxy instance for the given vtkSMProxy instance.
// e.g.
// <code>
// vtkClientServerStream stream;
// stream << vtkClientServerStream::Invoke
//        << SIOBJECT(proxyA)
//        << "MethodName"
//        << vtkClientServerStream::End;
// </code>
// Will result in calling the vtkSIProxy::MethodName() when the stream in
// interpreted.
class VTKPVSERVERMANAGERCORE_EXPORT SIOBJECT
{
  vtkSMRemoteObject* Reference;
  friend VTKPVSERVERMANAGERCORE_EXPORT vtkClientServerStream& operator<<(
    vtkClientServerStream& stream, const SIOBJECT& manipulator);
public:
  SIOBJECT(vtkSMRemoteObject* rmobject) : Reference(rmobject) {}
};

VTKPVSERVERMANAGERCORE_EXPORT vtkClientServerStream& operator<< (vtkClientServerStream& stream,
  const SIOBJECT& manipulator);

#endif // #ifndef __vtkSMRemoteObject_h
