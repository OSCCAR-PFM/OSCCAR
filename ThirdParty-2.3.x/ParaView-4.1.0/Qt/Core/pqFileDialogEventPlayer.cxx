/*=========================================================================

   Program: ParaView
   Module:    pqFileDialogEventPlayer.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqFileDialogEventPlayer.h"
#include "pqCoreTestUtility.h"

#include "pqFileDialog.h"
#include "pqEventDispatcher.h"

#include <vtksys/SystemTools.hxx>

#include <QApplication>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqFileDialogEventPlayer::pqFileDialogEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

//-----------------------------------------------------------------------------
bool pqFileDialogEventPlayer::playEvent(QObject* Object, const QString& Command, 
  const QString& Arguments, bool& Error)
{
  // Handle playback for pqFileDialog and all its children ...
  pqFileDialog* object = 0;
  for(QObject* o = Object; o; o = o->parent())
    {
    object = qobject_cast<pqFileDialog*>(o);
    if(object)
      break;
    }
  if(!object)
    return false;
  
  QString fileString = Arguments;

  const QString data_directory = pqCoreTestUtility::DataRoot();
  if(fileString.contains("PARAVIEW_DATA_ROOT") && data_directory.isEmpty())
    {
    qCritical() << "You must set the PARAVIEW_DATA_ROOT environment variable to play-back file selections.";
    Error = true;
    return true;
    }

  const QString test_directory = pqCoreTestUtility::TestDirectory();
  if (fileString.contains("PARAVIEW_TEST_ROOT") && test_directory.isEmpty())
    {
    qCritical() << "You must specify --test-directory in the command line options.";
    Error = true;
    return true;
    }

  if(Command == "filesSelected")
    {
    fileString.replace("$PARAVIEW_DATA_ROOT", data_directory);
    fileString.replace("$PARAVIEW_TEST_ROOT", test_directory);

    if(object->selectFile(fileString))
      {
      pqEventDispatcher::processEventsAndWait(0);
      }
    else
      {
      qCritical() << "Dialog couldn't accept " << fileString;
      Error = true;
      }

    return true;
    }
    
  if(Command == "cancelled")
    {
    object->reject();
    return true;
    }
  if (Command == "remove")
    {
    // Delete the file.
    fileString.replace("$PARAVIEW_DATA_ROOT", data_directory);
    fileString.replace("$PARAVIEW_TEST_ROOT", test_directory);
    vtksys::SystemTools::RemoveFile(fileString.toAscii().data()); 
    return true;
    }

  qCritical() << "Unknown pqFileDialog command: " << Object << " " << Command << " " << Arguments;
  Error = true;
  return true;
}
