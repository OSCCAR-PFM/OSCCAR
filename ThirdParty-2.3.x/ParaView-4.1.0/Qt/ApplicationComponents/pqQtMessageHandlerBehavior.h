/*=========================================================================

   Program: ParaView
   Module:    pqQtMessageHandlerBehavior.h

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
#ifndef __pqQtMessageHandlerBehavior_h 
#define __pqQtMessageHandlerBehavior_h

#include <QObject>
#include "pqApplicationComponentsModule.h"

/// @ingroup Behaviors
/// pqQtMessageHandlerBehavior install a message handler to capture Qt
/// error/debug messages i.e. install a event handler using qInstallMsgHandler
/// and post them as VTK messages.
class PQAPPLICATIONCOMPONENTS_EXPORT pqQtMessageHandlerBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqQtMessageHandlerBehavior(QObject* parent=0);
  ~pqQtMessageHandlerBehavior();

private:
  Q_DISABLE_COPY(pqQtMessageHandlerBehavior)
};

#endif


