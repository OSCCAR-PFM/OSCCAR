/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderView.h"
#include "vtkPVView.h"
#include "vtkRenderWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUtilities.h"
#include "vtkSmartPointer.h"

#include <assert.h>

vtkStandardNewMacro(vtkSMViewProxy);
//----------------------------------------------------------------------------
vtkSMViewProxy::vtkSMViewProxy()
{
  this->SetLocation(vtkProcessModule::CLIENT_AND_SERVERS);
  this->DefaultRepresentationName = 0;
  this->Enable = true;
}

//----------------------------------------------------------------------------
vtkSMViewProxy::~vtkSMViewProxy()
{
  this->SetDefaultRepresentationName(0);
}

//----------------------------------------------------------------------------
vtkView* vtkSMViewProxy::GetClientSideView()
{
  if (this->ObjectsCreated)
    {
    return vtkView::SafeDownCast(this->GetClientSideObject());
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  // If prototype, no need to go further...
  if(this->Location == 0)
    {
    return;
    }

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "Initialize"
         << static_cast<int>(this->GetGlobalID())
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  vtkObject::SafeDownCast(this->GetClientSideObject())->AddObserver(
      vtkPVView::ViewTimeChangedEvent,
      this, &vtkSMViewProxy::ViewTimeChanged);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::ViewTimeChanged()
{
  vtkSMPropertyHelper helper1(this, "Representations");
  for (unsigned int cc=0; cc  < helper1.GetNumberOfElements(); cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      helper1.GetAsProxy(cc));
    if (repr)
      {
      repr->ViewTimeChanged();
      }
    }

  vtkSMPropertyHelper helper2(this, "HiddenRepresentations", true);
  for (unsigned int cc=0; cc  < helper2.GetNumberOfElements(); cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      helper2.GetAsProxy(cc));
    if (repr)
      {
      repr->ViewTimeChanged();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::StillRender()
{
  // bug 0013947
  // on Mac OSX don't render into invalid drawable, all subsequent
  // OpenGL calls fail with invalid framebuffer operation.
  vtkPVRenderView *rv
    = dynamic_cast<vtkPVRenderView*>(this->GetClientSideObject());

  if (rv && !rv->GetRenderWindow()->IsDrawable())
    {
    return;
    }

  int interactive = 0;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);
  // We call update separately from the render. This is done so that we don't
  // get any synchronization issues with GUI responding to the data-updated
  // event by making some data information requests(for example). If those
  // happen while StillRender/InteractiveRender is being executed on the server
  // side then we get deadlocks.
  this->Update();

  vtkTypeUInt32 render_location = this->PreRender(interactive==1);

  if (this->ObjectsCreated)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "StillRender"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, render_location);
    }

  this->PostRender(interactive==1);
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::InteractiveRender()
{
  int interactive = 1;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);

  // Interactive render will not call Update() at all. It's expected that you
  // must have either called a StillRender() or an Update() before triggering an
  // interactive render. This is critical to keep interactive rates fast when
  // working over a slow client-server connection.
  // this->Update();

  vtkTypeUInt32 render_location = this->PreRender(interactive==1);

  if (this->ObjectsCreated)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "InteractiveRender"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, render_location);
    }

  this->PostRender(interactive==1);
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::Update()
{
  if (this->ObjectsCreated && this->NeedsUpdate)
    {
    vtkClientServerStream stream;

    // To avoid race conditions in multi-client modes, we are taking a peculiar
    // approach. Any ivar that affect parallel communication are overridden
    // using the client-side values in the same ExecuteStream() call. That
    // ensures that two clients cannot enter race condition. This results in minor
    // increase in the size of the messages sent, but overall the benefits are
    // greater.
    vtkPVView* pvview = vtkPVView::SafeDownCast(this->GetClientSideObject());
    if (pvview)
      {
      int use_cache =  pvview->GetUseCache()? 1 : 0;
      stream << vtkClientServerStream::Invoke
             << VTKOBJECT(this)
             << "SetUseCache" << use_cache
             << vtkClientServerStream::End;
      }
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "Update"
           << vtkClientServerStream::End;
    this->GetSession()->PrepareProgress();
    this->ExecuteStream(stream);
    this->GetSession()->CleanupPendingProgress();

    unsigned int numProducers = this->GetNumberOfProducers();
    for (unsigned int i=0; i<numProducers; i++)
      {
      vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
        this->GetProducerProxy(i));
      if (repr)
        {
        repr->ViewUpdated(this);
        }
      else
        {
        //this->GetProducerProxy(i)->PostUpdateData();
        }
      }

    this->PostUpdateData();
    }
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* vtkNotUsed(proxy), int vtkNotUsed(opport))
{
  if (this->DefaultRepresentationName)
    {
    assert("The session should be valid" && this->Session);
    vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
    vtkSmartPointer<vtkSMProxy> p;
    p.TakeReference(pxm->NewProxy("representations", this->DefaultRepresentationName));
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(p);
    if (repr)
      {
      repr->Register(this);
      return repr;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkSMViewProxy::ReadXMLAttributes(
  vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }

  const char* repr_name = element->GetAttribute("representation_name");
  if (repr_name)
    {
    this->SetDefaultRepresentationName(repr_name);
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMViewProxy::CaptureWindow(int magnification)
{
  if (this->ObjectsCreated)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "PrepareForScreenshot"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    }

  vtkImageData* capture = this->CaptureWindowInternal(magnification);

  if (this->ObjectsCreated)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "CleanupAfterScreenshot"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    }

  if (capture)
    {
    int position[2];
    vtkSMPropertyHelper(this, "ViewPosition").Get(position, 2);

    // Update image extents based on ViewPosition
    int extents[6];
    capture->GetExtent(extents);
    for (int cc=0; cc < 4; cc++)
      {
      extents[cc] += position[cc/2]*magnification;
      }
    capture->SetExtent(extents);
    }
  return capture;
}

//-----------------------------------------------------------------------------
int vtkSMViewProxy::WriteImage(const char* filename,
  const char* writerName, int magnification)
{
  if (!filename || !writerName)
    {
    return vtkErrorCode::UnknownError;
    }

  vtkSmartPointer<vtkImageData> shot;
  shot.TakeReference(this->CaptureWindow(magnification));

  if (vtkProcessModule::GetProcessModule()->GetOptions()->GetSymmetricMPIMode())
    {
    return vtkSMUtilities::SaveImageOnProcessZero(shot, filename, writerName);
    }
  return vtkSMUtilities::SaveImage(shot, filename, writerName);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkSMViewProxy::HasDirtyRepresentation()
{
  return this->NeedsUpdate;
}
//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMViewProxy::GetRenderWindow()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetRenderWindow() : NULL;
}
