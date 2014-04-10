/*=========================================================================

   Program: ParaView
   Module:    pqCoreUtilities.h

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
#ifndef __pqCoreUtilities_h 
#define __pqCoreUtilities_h

#include "pqCoreModule.h"
#include "pqEventDispatcher.h"

#include <QEventLoop>
#include <QPointer>
#include <QWidget>
#include <QDir>
#include <QStringList>
#include <QString>
#include <QFile>
#include <QFileInfo>

class vtkObject;

/// INTERNAL CLASS (DO NOT USE). This is used by
/// pqCoreUtilities::connectWithVTK() methods.
class PQCORE_EXPORT pqCoreUtilitiesEventHelper : public QObject
{
  Q_OBJECT;
  typedef QObject Superclass;
public:
  pqCoreUtilitiesEventHelper(QObject* parent);
  ~pqCoreUtilitiesEventHelper();

signals:
  void eventInvoked(vtkObject*, unsigned long, void*);

private:
  Q_DISABLE_COPY(pqCoreUtilitiesEventHelper);

  void executeEvent(vtkObject*, unsigned long, void*);
  class pqInternal;
  pqInternal* Interal;
  friend class pqCoreUtilities;
};

/// pqCoreUtilities is a collection of arbitrary utility functions that can be
/// used by the application.
class PQCORE_EXPORT pqCoreUtilities 
{
public:
  /// When popuping up dialogs, it's generally better if we set the parent
  /// widget for those dialogs to be the QMainWindow so that the dialogs show up
  /// centered correctly in the application. For that purpose this convenience
  /// method is provided. It locates a QMainWindow and returns it.
  static void setMainWidget(QWidget* widget)
    {
    pqCoreUtilities::MainWidget = widget;
    }
  static QWidget* mainWidget() 
    { 
    if (!pqCoreUtilities::MainWidget)
      {
      pqCoreUtilities::MainWidget = pqCoreUtilities::findMainWindow();
      }
    return pqCoreUtilities::MainWidget; 
    }

  /// Call QApplication::processEvents plus make sure the testing framework
  /// is 
  static void processEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents)
    {
    pqEventDispatcher::processEvents(flags);
    }

  /// Return the path of the root ParaView user specific configuration directory
  static QString getParaViewUserDirectory();

  /// Return the path of the launched application
  static QString getParaViewApplicationDirectory();

  /// Return the list of full available path that exists inside the shared
  /// application path and the user specific one
  static QStringList findParaviewPaths(QString directoryOrFileName,
                                       bool lookupInAppDir,
                                       bool lookupInUserDir);
  static QString getNoneExistingFileName(QString expectedFilePath);

  /// Method used to connect VTK events to Qt slots (or signals).
  /// This is an alternative to using vtkEventQtSlotConnect. This method gives a
  /// cleaner API to connect vtk-events to Qt slots. It manages cleanup
  /// correctly i.e. either vtk-object or the qt-object can be deleted and the
  /// observers will be cleaned up correctly. One can disconnect the connection
  /// made explicitly by vtk_object->RemoveObserver(eventId) where eventId is
  /// the returned value.
  static unsigned long connect(
    vtkObject* vtk_object, int vtk_event_id,
    QObject* qobject, const char* signal_or_slot,
    Qt::ConnectionType type = Qt::AutoConnection);

private:
  static QWidget* findMainWindow();
  static QPointer<QWidget> MainWidget;
};

#endif


