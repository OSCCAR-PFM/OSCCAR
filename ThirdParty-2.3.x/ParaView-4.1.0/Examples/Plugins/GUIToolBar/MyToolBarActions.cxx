

#include "MyToolBarActions.h"

#include <QApplication>
#include <QStyle>
#include <QMessageBox>

MyToolBarActions::MyToolBarActions(QObject* p)
  : QActionGroup(p)
{
  QIcon icon = qApp->style()->standardIcon(QStyle::SP_MessageBoxCritical);

  QAction* a = this->addAction(new QAction(icon, "MyAction", this));
  QObject::connect(a, SIGNAL(triggered(bool)), this, SLOT(onAction()));
}

MyToolBarActions::~MyToolBarActions()
{
}

void MyToolBarActions::onAction()
{
  QMessageBox::information(NULL, "MyAction", "MyAction was invoked\n");
}

