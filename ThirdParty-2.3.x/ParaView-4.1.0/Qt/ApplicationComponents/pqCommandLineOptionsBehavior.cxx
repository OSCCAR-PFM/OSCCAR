/*=========================================================================

   Program: ParaView
   Module:    pqCommandLineOptionsBehavior.cxx

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
#include "pqCommandLineOptionsBehavior.h"

#include "pqActiveObjects.h"
#include "pqCollaborationEventPlayer.h"
#include "pqComponentsTestUtility.h"
#include "pqCoreUtilities.h"
#include "pqDeleteReaction.h"
#include "pqEventDispatcher.h"
#include "pqFileDialog.h"
#include "pqFixPathsInStateFilesBehavior.h"
#include "pqLoadDataReaction.h"
#include "pqLoadStateReaction.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqPersistentMainWindowStateBehavior.h"
#include "pqPVApplicationCore.h"
#include "pqPythonShellReaction.h"
#include "pqRenderView.h"
#include "pqScalarsToColors.h"
#include "pqServerConnectReaction.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QTimer>

//-----------------------------------------------------------------------------
pqCommandLineOptionsBehavior::pqCommandLineOptionsBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QTimer::singleShot(100, this, SLOT(processCommandLineOptions()));
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::processCommandLineOptions()
{
  pqOptions* options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  
  // check for --server.
  const char* serverresource_name = options->GetServerResourceName();
  // (server-url gets lower priority than --server).
  const char* server_url = options->GetServerURL();
  if (serverresource_name)
    {
    if (!pqServerConnectReaction::connectToServerUsingConfigurationName(
        serverresource_name))
      {
      qCritical() << "Could not connect to requested server \"" 
        << serverresource_name 
        << "\". Creating default builtin connection.";
      }
    }
  else if (server_url)
    {
    if(strchr(server_url, '|') != NULL)
      {
      // We should connect multiple times
      QStringList urls = QString(server_url).split(QRegExp("\\|"), QString::SkipEmptyParts);
      foreach(QString url, urls)
        {
        if (!pqServerConnectReaction::connectToServer(pqServerResource(url)))
              {
              qCritical() << "Could not connect to requested server \""
                << url << "\". Creating default builtin connection.";
              }
        }
      }
    else if (!pqServerConnectReaction::connectToServer(pqServerResource(server_url)))
      {
      qCritical() << "Could not connect to requested server \"" 
        << server_url << "\". Creating default builtin connection.";
      }
    }
  if (pqActiveObjects::instance().activeServer() == NULL)
    {
    pqServerConnectReaction::connectToServer(pqServerResource("builtin:"));
    }

  // Now we are assured that some default server connection has been made
  // (either the one requested by the user on the command line or simply the
  // default one).
  Q_ASSERT(pqActiveObjects::instance().activeServer() != 0);

  // For tile display testing lets enable the dump of images
  if(options->GetTileImagePath())
    {
    vtkSMProxy* proxy =
        vtkSMProxyManager::GetProxyManager()->NewProxy("tile_helper","TileDisplayHelper");
    vtkSMStringVectorProperty* pathProperty =
        vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("DumpImagePath"));
    pathProperty->SetElement(0, options->GetTileImagePath());
    proxy->UpdateVTKObjects();
    proxy->Delete();
    }

  // check for --data option.
  if (options->GetParaViewDataName())
    {
    // We don't directly set the data file name instead use the dialog. This
    // makes it possible to select a file group.
    pqFileDialog dialog (
      pqActiveObjects::instance().activeServer(),
      pqCoreUtilities::mainWidget(),
      tr("Internal Open File"), QString(),
      QString());
    dialog.setFileMode(pqFileDialog::ExistingFiles);
    if (!dialog.selectFile(options->GetParaViewDataName()))
      {
      qCritical() << "Cannot open data file \""
        << options->GetParaViewDataName() << "\"";
      }
    QList<QStringList> files = dialog.getAllSelectedFiles();
    QStringList file;
    foreach(file,files)
      {
      if (pqLoadDataReaction::loadData(file) == NULL)
        {
        qCritical() << "Failed to load data file: " <<
          options->GetParaViewDataName();
        }
      }
    }
  else if (options->GetStateFileName())
    {
    // check for --state option. (Bug #5711)
    // NOTE: --data and --state cannnot be specifed at the same time.

    // HACK: We need to disable popping up of the fix-filenames dialog when
    // loading a state file from the command line.
    bool prev = pqFixPathsInStateFilesBehavior::blockDialog(true);
    pqLoadStateReaction::loadState(options->GetStateFileName());
    pqFixPathsInStateFilesBehavior::blockDialog(prev);
    }

  if (options->GetPythonScript())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    pqPythonShellReaction::executeScript(options->GetPythonScript());
#else
    qCritical() << "Python support not enabled. Cannot run python scripts.";
#endif
    }

  if (options->GetDisableRegistry())
    {
    // a cout for test playback.
    cout << "Process started" << endl;
    }

  if (options->GetNumberOfTestScripts() > 0)
    {
    QTimer::singleShot(1000, this, SLOT(playTests()));
    }
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::playTests()
{
  pqOptions* options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());

  pqPersistentMainWindowStateBehavior::saveState(
    qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget()));

  bool success = true;
  for (int cc=0; success &&  cc < options->GetNumberOfTestScripts(); cc++)
    {
    if (cc > 0)
      {
      this->resetApplication();
      }
    else if (cc==0)
      {
      if (options->GetTestMaster())
        {
        pqCollaborationEventPlayer::waitForConnections(2);
        }
      else if (options->GetTestSlave())
        {
        pqCollaborationEventPlayer::waitForMaster();
        }
      }

    // Load the test plugin specified at the command line
    std::string plugin(  options->GetTestPlugin() );
    if(plugin.size())
      {
      // Make in-code plugin XML
      std::string plugin_xml;
      plugin_xml += "<Plugins><Plugin name=\"";
      plugin_xml += plugin;
      plugin_xml += "\" auto_load=\"1\" /></Plugins>\n";

      // Load the plugin into the plugin manager. Local and remote
      // loading is done here.
      vtkSMProxyManager  *pxm           = vtkSMProxyManager::GetProxyManager();
      vtkSMPluginManager *pluginManager = pxm->GetPluginManager();
      vtkSMSession       *activeSession = pxm->GetActiveSession();
      pluginManager->LoadPluginConfigurationXMLFromString(plugin_xml.c_str(),
                                                          activeSession,
                                                          true);
      pluginManager->LoadPluginConfigurationXMLFromString(plugin_xml.c_str(),
                                                          activeSession,
                                                          false);
      }

    // Play the test script if specified.
    pqTestUtility* testUtility = pqApplicationCore::instance()->testUtility();
    options->SetCurrentImageThreshold(options->GetTestImageThreshold(cc));
    cout << "Playing: " << options->GetTestScript(cc).toAscii().data() << endl;
    success = testUtility->playTests(options->GetTestScript(cc));

    if (success && !options->GetTestBaseline(cc).isEmpty())
      {
      success = pqComponentsTestUtility::CompareView(
        options->GetTestBaseline(cc), options->GetTestImageThreshold(cc),
        options->GetTestDirectory());
      }
    }

  if (options->GetExitAppWhenTestsDone())
    {
    if (options->GetTestMaster())
      {
      pqCollaborationEventPlayer::wait(1000);
      }

    // Make sure that the pqApplicationCore::prepareForQuit() method
    // get called
    QApplication::closeAllWindows();

    QApplication::instance()->exit(success? 0 : 1);
    }
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::resetApplication()
{
  BEGIN_UNDO_EXCLUDE();

  // delete all sources and representations
  pqDeleteReaction::deleteAll();

  // delete all views
  QList<pqView*> current_views = 
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqView*>();
  foreach (pqView* view, current_views)
    {
    pqApplicationCore::instance()->getObjectBuilder()->destroy(view);
    }

  // delete all looktables.
  QList<pqScalarsToColors*> luts = 
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqScalarsToColors*>();
  foreach (pqScalarsToColors* lut, luts)
    {
    pqApplicationCore::instance()->getObjectBuilder()->destroy(lut);
    }

  // reset view layout.
  pqTabbedMultiViewWidget* viewWidget = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (viewWidget)
    {
    viewWidget->reset();
    }

  // create default render view.
  pqApplicationCore::instance()->getObjectBuilder()->createView(
    pqRenderView::renderViewType(),
    pqActiveObjects::instance().activeServer());

  // reset animation time.
  pqActiveObjects::instance().activeServer()->getTimeKeeper()->setTime(0.0);

  pqEventDispatcher::processEventsAndWait(10);

  END_UNDO_EXCLUDE();
  CLEAR_UNDO_STACK();
}
