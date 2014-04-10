/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqFixPathsInStateFilesBehavior.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFixStateFilenamesDialog.h"
#include "vtkPVXMLElement.h"

bool pqFixPathsInStateFilesBehavior::BlockDialog = false;

//-----------------------------------------------------------------------------
pqFixPathsInStateFilesBehavior::pqFixPathsInStateFilesBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance(),
    SIGNAL(aboutToLoadState(vtkPVXMLElement*)),
    this, SLOT(onLoadState(vtkPVXMLElement*)));
}

//-----------------------------------------------------------------------------
pqFixPathsInStateFilesBehavior::~pqFixPathsInStateFilesBehavior()
{
}

//-----------------------------------------------------------------------------
bool pqFixPathsInStateFilesBehavior::blockDialog(bool val)
{
  bool cur_val = pqFixPathsInStateFilesBehavior::BlockDialog;
  pqFixPathsInStateFilesBehavior::BlockDialog = val;
  return cur_val;
}

//-----------------------------------------------------------------------------
void pqFixPathsInStateFilesBehavior::fixFileNames(vtkPVXMLElement* xml)
{
  Q_ASSERT(xml != NULL);
  pqFixStateFilenamesDialog dialog(xml, pqCoreUtilities::mainWidget());
  if (dialog.hasFileNames())
    {
    dialog.exec();
    }
}

//-----------------------------------------------------------------------------
void pqFixPathsInStateFilesBehavior::onLoadState(vtkPVXMLElement* xml)
{
  Q_ASSERT(xml != NULL);
  if (pqFixPathsInStateFilesBehavior::BlockDialog == false)
    {
    pqFixPathsInStateFilesBehavior::fixFileNames(xml);
    }
}
