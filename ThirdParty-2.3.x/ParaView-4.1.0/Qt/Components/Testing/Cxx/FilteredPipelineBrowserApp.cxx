// A Test of a very simple app based on pqCore
#include "FilteredPipelineBrowserApp.h"

#include <QTimer>
#include <QApplication>
#include <QVBoxLayout>
#include <QDebug>
#include <QStringList>

#include "QVTKWidget.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMPropertyHelper.h"

#include "pqApplicationCore.h"
#include "pqCoreTestUtility.h"
#include "pqInterfaceTracker.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqPipelineSource.h"
#include "pqPipelineAnnotationFilterModel.h"
#include "pqServer.h"
#include "pqStandardViewModules.h"
#include "vtkProcessModule.h"

MainPipelineWindow::MainPipelineWindow()
{
  // Init ParaView
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* ob = core->getObjectBuilder();
  pqInterfaceTracker* pgm = pqApplicationCore::instance()->interfaceTracker();
  pgm->addInterface(new pqStandardViewModules(pgm));

  // Set Filter/Annotation list
  this->FilterNames.append("No filtering");
  this->FilterNames.append("Filter 1");
  this->FilterNames.append("Filter 2");
  this->FilterNames.append("Filter 3");
  this->FilterNames.append("Filter 4");

  // Set Filter selector
  this->FilterSelector = new QComboBox();
  this->FilterSelector->addItems(this->FilterNames);
  QObject::connect( this->FilterSelector, SIGNAL(currentIndexChanged(int)),
                    this, SLOT(updateSelectedFilter(int)),
                    Qt::QueuedConnection);

  // Set Pipeline Widget
  this->PipelineWidget = new pqPipelineBrowserWidget(NULL);

  // Create server only after a pipeline browser get created...
  pqServer* server = ob->createServer(pqServerResource("builtin:"));

  // Init and layout the UI
  QWidget *container = new QWidget(this);
  QVBoxLayout *internalLayout = new QVBoxLayout();
  internalLayout->addWidget(this->FilterSelector);
  internalLayout->addWidget(this->PipelineWidget);
  container->setLayout(internalLayout);
  this->setCentralWidget(container);

  // Create a complex pipeline with different annotations
  createPipelineWithAnnotation(server);

  QTimer::singleShot(100, this, SLOT(processTest()));
}

//-----------------------------------------------------------------------------
void MainPipelineWindow::updateSelectedFilter(int filterIndex)
{
  if(filterIndex == 0)
    {
    this->PipelineWidget->disableAnnotationFilter();
    }
  else
    {
    this->PipelineWidget->enableAnnotationFilter(
        this->FilterNames.at(filterIndex));
    }
}

//-----------------------------------------------------------------------------
void MainPipelineWindow::processTest()
{
  if (pqOptions* const options = pqApplicationCore::instance()->getOptions())
    {
    bool test_succeeded = true;

    // ---- Do the testing here ----

    // ---- Do the testing here ----

    if (options->GetExitAppWhenTestsDone())
      {
      QApplication::instance()->exit(test_succeeded ? 0 : 1);
      }
    }
}
//-----------------------------------------------------------------------------
void MainPipelineWindow::createPipelineWithAnnotation(pqServer* server)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* ob = core->getObjectBuilder();

  // create source and elevation filter
  pqPipelineSource* wavelet;
  pqPipelineSource* cone;
  pqPipelineSource* clip1;
  pqPipelineSource* clip2;
  pqPipelineSource* append;
  pqPipelineSource* groupDS;

  wavelet = ob->createSource("sources", "RTAnalyticSource", server);
  vtkSMSourceProxy::SafeDownCast(wavelet->getProxy())->UpdatePipeline();

  clip1 = ob->createFilter("filters", "Clip", wavelet);
  vtkSMSourceProxy::SafeDownCast(clip1->getProxy())->UpdatePipeline();

  clip2 = ob->createFilter("filters", "Clip", clip1);
  vtkSMSourceProxy::SafeDownCast(clip2->getProxy())->UpdatePipeline();

  cone = ob->createSource("sources", "ConeSource", server);
  vtkSMSourceProxy::SafeDownCast(cone->getProxy())->UpdatePipeline();

  append = ob->createFilter("filters", "Append", clip2);
  vtkSMPropertyHelper(append->getProxy(), "Input").Add(cone->getProxy(), 0);
  vtkSMSourceProxy::SafeDownCast(append->getProxy())->UpdatePipeline();

  groupDS = ob->createFilter("filters", "GroupDataSets", clip2);
  vtkSMPropertyHelper(groupDS->getProxy(), "Input").Add(cone->getProxy(), 0);
  vtkSMSourceProxy::SafeDownCast(groupDS->getProxy())->UpdatePipeline();

  // Setup annotations:
  wavelet->getProxy()->SetAnnotation(this->FilterNames.at(1).toAscii().data(), "-");
  clip1->getProxy()->SetAnnotation(this->FilterNames.at(1).toAscii().data(), "-");
  clip2->getProxy()->SetAnnotation(this->FilterNames.at(1).toAscii().data(), "-");

  append->getProxy()->SetAnnotation(this->FilterNames.at(2).toAscii().data(), "-");
  groupDS->getProxy()->SetAnnotation(this->FilterNames.at(2).toAscii().data(), "-");

  wavelet->getProxy()->SetAnnotation(this->FilterNames.at(3).toAscii().data(), "-");
  clip1->getProxy()->SetAnnotation(this->FilterNames.at(3).toAscii().data(), "-");
  clip2->getProxy()->SetAnnotation(this->FilterNames.at(3).toAscii().data(), "-");
  append->getProxy()->SetAnnotation(this->FilterNames.at(3).toAscii().data(), "-");

  wavelet->getProxy()->SetAnnotation(this->FilterNames.at(4).toAscii().data(), "-");
  cone->getProxy()->SetAnnotation(this->FilterNames.at(4).toAscii().data(), "-");

  // Tooltip
  wavelet->getProxy()->SetAnnotation("tooltip", "1+3+4");
  clip1->getProxy()->SetAnnotation("tooltip", "1+3");
  clip2->getProxy()->SetAnnotation("tooltip", "1+3");
  cone->getProxy()->SetAnnotation("tooltip", "4");
  append->getProxy()->SetAnnotation("tooltip", "2+3");
  groupDS->getProxy()->SetAnnotation("tooltip", "2");
}
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  pqApplicationCore appCore(argc, argv);
  MainPipelineWindow window;
  window.resize(200, 150);
  window.show();
  return app.exec();
}
