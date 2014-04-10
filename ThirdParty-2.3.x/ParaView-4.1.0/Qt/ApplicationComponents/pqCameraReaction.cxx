/*=========================================================================

   Program: ParaView
   Module:    pqCameraReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqCameraReaction.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"
#include "vtkSMRenderViewProxy.h"
#include "pqPipelineRepresentation.h"

//-----------------------------------------------------------------------------
pqCameraReaction::pqCameraReaction(QAction* parentObject,
  pqCameraReaction::Mode mode)
  : Superclass(parentObject)
{
  this->ReactionMode = mode;
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this, SLOT(updateEnableState()), Qt::QueuedConnection);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqCameraReaction::updateEnableState()
{
  pqView* view = pqActiveObjects::instance().activeView();
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  if (view && this->ReactionMode == RESET_CAMERA)
    {
    this->parentAction()->setEnabled(true);
    }
  else if (rview)
    {
    if(this->ReactionMode == ZOOM_TO_DATA)
      {
      this->parentAction()->setEnabled(source != 0);
      }
    else
      {
      this->parentAction()->setEnabled(true);
      }
    }
  else
    {
    this->parentAction()->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::onTriggered()
{
  switch (this->ReactionMode)
    {
  case RESET_CAMERA:
    this->resetCamera();
    break;

  case RESET_POSITIVE_X:
    this->resetPositiveX();
    break;

  case RESET_POSITIVE_Y:
    this->resetPositiveY();
    break;

  case RESET_POSITIVE_Z:
    this->resetPositiveZ();
    break;

  case RESET_NEGATIVE_X:
    this->resetNegativeX();
    break;

  case RESET_NEGATIVE_Y:
    this->resetNegativeY();
    break;
  case RESET_NEGATIVE_Z:
    this->resetNegativeZ();
    break;
  case ZOOM_TO_DATA:
    this->zoomToData();
    break;
    }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetCamera()
{
  pqView* view = pqActiveObjects::instance().activeView();
  if (view)
    {
    view->resetDisplay();
    }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetDirection(
  double look_x, double look_y, double look_z,
  double up_x, double up_y, double up_z)
{
  pqRenderView* ren = qobject_cast<pqRenderView*>(
    pqActiveObjects::instance().activeView());
  if (ren)
    {
    ren->resetViewDirection(look_x, look_y, look_z, up_x, up_y, up_z);
    }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetPositiveX()
{
  pqCameraReaction::resetDirection(1, 0, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetNegativeX()
{
  pqCameraReaction::resetDirection(-1, 0, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetPositiveY()
{
  pqCameraReaction::resetDirection(0, 1, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetNegativeY()
{
  pqCameraReaction::resetDirection(0, -1, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetPositiveZ()
{
  pqCameraReaction::resetDirection(0, 0, 1, 0, 1, 0);
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetNegativeZ()
{
  pqCameraReaction::resetDirection(0, 0, -1, 0, 1, 0);
}

//-----------------------------------------------------------------------------
void pqCameraReaction::zoomToData()
{
  pqRenderView* renModule = qobject_cast<pqRenderView*>(
    pqActiveObjects::instance().activeView());
  pqPipelineRepresentation *repr = qobject_cast<pqPipelineRepresentation*>(
    pqActiveObjects::instance().activeRepresentation());
  if (renModule && repr)
    {
    vtkSMRenderViewProxy* rm = renModule->getRenderViewProxy();
    rm->ZoomTo(repr->getProxy());
    renModule->render();
    }
}
