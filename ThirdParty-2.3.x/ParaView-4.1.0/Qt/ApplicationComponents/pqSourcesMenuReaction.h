/*=========================================================================

   Program: ParaView
   Module:    pqSourcesMenuReaction.h

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
#ifndef __pqSourcesMenuReaction_h 
#define __pqSourcesMenuReaction_h

#include <QObject>
#include "pqApplicationComponentsModule.h"

class pqPipelineSource;
class pqProxyGroupMenuManager;

/// @ingroup Reactions
/// Reaction to handle creation of sources from the sources menu.
class PQAPPLICATIONCOMPONENTS_EXPORT pqSourcesMenuReaction : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqSourcesMenuReaction(pqProxyGroupMenuManager* menuManager);

  static pqPipelineSource* createSource(
    const QString& group, const QString& name);

public slots:
  /// Updates the enabled state. Applications need not explicitly call
  /// this.
  virtual void updateEnableState();
  void updateEnableState(bool);

protected slots:
  /// Called when the action is triggered.
  virtual void onTriggered(const QString& group, const QString& name)
    { pqSourcesMenuReaction::createSource(group, name); } 

private:
  Q_DISABLE_COPY(pqSourcesMenuReaction)
};

#endif


