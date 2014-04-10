/*=========================================================================

   Program: ParaView
   Module: pqStandardPropertyWidgetInterface.cxx

   Copyright (c) 2012 Sandia Corporation, Kitware Inc.
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
#include "pqStandardPropertyWidgetInterface.h"

#include "pqArrayStatusPropertyWidget.h"
#include "pqCalculatorWidget.h"
#include "pqClipScalarsDecorator.h"
#include "pqColorAnnotationsPropertyWidget.h"
#include "pqColorEditorPropertyWidget.h"
#include "pqColorOpacityEditorWidget.h"
#include "pqColorSelectorPropertyWidget.h"
#include "pqCommandButtonPropertyWidget.h"
#include "pqCTHArraySelectionDecorator.h"
#include "pqCubeAxesPropertyWidget.h"
#include "pqDisplayRepresentationWidget.h"
#include "pqDoubleRangeSliderPropertyWidget.h"
#include "pqEnableWidgetDecorator.h"
#include "pqFontPropertyWidget.h"
#include "pqInputDataTypeDecorator.h"
#include "pqListPropertyWidget.h"
#include "pqTextureSelectorPropertyWidget.h"
#include "pqTransferFunctionWidgetPropertyWidget.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProperty.h"

#include <QtDebug>

//-----------------------------------------------------------------------------
pqStandardPropertyWidgetInterface::pqStandardPropertyWidgetInterface(QObject *p)
  : QObject(p)
{
}

//-----------------------------------------------------------------------------
pqStandardPropertyWidgetInterface::~pqStandardPropertyWidgetInterface()
{
}

//-----------------------------------------------------------------------------
pqPropertyWidget*
pqStandardPropertyWidgetInterface::createWidgetForProperty(vtkSMProxy *smProxy,
                                                           vtkSMProperty *smProperty)
{
  // handle properties that specify custom panel widgets
  const char *custom_widget = smProperty->GetPanelWidget();
  if (!custom_widget)
    {
    return NULL;
    }

  std::string name = custom_widget;

  // *** NOTE: When adding new types, please update the header documentation ***
  if(name == "color_selector")
    {
    return new pqColorSelectorPropertyWidget(smProxy, smProperty);
    }
  else if(name == "display_representation_selector")
    {
    return new pqDisplayRepresentationPropertyWidget(smProxy);
    }
  else if(name == "texture_selector")
    {
    return new pqTextureSelectorPropertyWidget(smProxy);
    }
  else if (name == "calculator")
    {
    return new pqCalculatorWidget(smProxy, smProperty);
    }
  else if(name == "command_button")
    {
    return new pqCommandButtonPropertyWidget(smProxy, smProperty);
    }
  else if(name == "transfer_function_editor")
    {
    return new pqTransferFunctionWidgetPropertyWidget(smProxy, smProperty);
    }
  else if (name == "list")
    {
    return new pqListPropertyWidget(smProxy, smProperty);
    }
  else if (name == "double_range")
    {
    return new pqDoubleRangeSliderPropertyWidget(smProxy, smProperty);
    }
  // *** NOTE: When adding new types, please update the header documentation ***
  return NULL;
}

//-----------------------------------------------------------------------------
pqPropertyWidget*
pqStandardPropertyWidgetInterface::createWidgetForPropertyGroup(vtkSMProxy *proxy,
                                                                vtkSMPropertyGroup *group)
{
  // *** NOTE: When adding new types, please update the header documentation ***
  if(QString(group->GetPanelWidget()) == "ColorEditor")
    {
    return new pqColorEditorPropertyWidget(proxy);
    }
  else if(QString(group->GetPanelWidget()) == "CubeAxes")
    {
    return new pqCubeAxesPropertyWidget(proxy);
    }
  else if (QString(group->GetPanelWidget()) == "ArrayStatus")
    {
    return new pqArrayStatusPropertyWidget(proxy, group);
    }
  else if (QString(group->GetPanelWidget()) == "ColorOpacityEditor")
    {
    return new pqColorOpacityEditorWidget(proxy, group);
    }
  else if (QString(group->GetPanelWidget()) == "AnnotationsEditor")
    {
    return new pqColorAnnotationsPropertyWidget(proxy, group);
    }
  else if (QString(group->GetPanelWidget()) == "FontEditor")
    {
    return new pqFontPropertyWidget(proxy, group);
    }
  // *** NOTE: When adding new types, please update the header documentation ***

  return 0;
}

//-----------------------------------------------------------------------------
pqPropertyWidgetDecorator*
pqStandardPropertyWidgetInterface::createWidgetDecorator(
  const QString& type, vtkPVXMLElement* config, pqPropertyWidget* widget)
{
  // *** NOTE: When adding new types, please update the header documentation ***
  if (type == "ClipScalarsDecorator")
    {
    return new pqClipScalarsDecorator(config, widget);
    }
  if (type == "CTHArraySelectionDecorator")
    {
    return new pqCTHArraySelectionDecorator(config, widget);
    }
  if (type == "InputDataTypeDecorator")
    {
    return new pqInputDataTypeDecorator(config, widget);
    }
  if (type == "EnableWidgetDecorator")
    {
    return new pqEnableWidgetDecorator(config, widget);
    }
  // *** NOTE: When adding new types, please update the header documentation ***
  return NULL;
}
