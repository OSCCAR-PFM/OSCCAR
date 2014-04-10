/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXYChartView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVXYChartView.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkChartWarning.h"
#include "vtkChartLegend.h"
#include "vtkChartParallelCoordinates.h"
#include "vtkChartXY.h"
#include "vtkCommand.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPVPlotTime.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkXYChartRepresentation.h"
#include "vtkContextMouseEvent.h"

#include <string>
#include <vtksys/ios/sstream>

class vtkPVXYChartView::vtkInternals
{
public:
  // Keeps track of custom labels for each of the axis.
  vtkNew<vtkDoubleArray> CustomLabelPositions[4];
  bool UseCustomLabels[4];
  vtkInternals()
    {
    this->UseCustomLabels[0] = this->UseCustomLabels[1]
      = this->UseCustomLabels[2] = this->UseCustomLabels[3] = false;

    this->AxisRanges[0][0] =
      this->AxisRanges[1][0] =
      this->AxisRanges[2][0] =
      this->AxisRanges[3][0] = 0.0;
    this->AxisRanges[0][1] =
      this->AxisRanges[1][1] =
      this->AxisRanges[2][1] =
      this->AxisRanges[3][1] = 6.66;
    }

  double AxisRanges[4][2];
};


vtkStandardNewMacro(vtkPVXYChartView);

//----------------------------------------------------------------------------
vtkPVXYChartView::vtkPVXYChartView()
{
  this->Internals = new vtkInternals();

  this->Chart = NULL;
  this->InternalTitle = NULL;
  this->PlotTime = vtkPVPlotTime::New();

  // Use the buffer id - performance issues are fixed.
  this->ContextView->GetScene()->SetUseBufferId(true);
  this->ContextView->GetScene()->SetScaleTiles(false);
  this->LogScaleWarningLabel = NULL;
}

//----------------------------------------------------------------------------
vtkPVXYChartView::~vtkPVXYChartView()
{
  if (this->Chart)
    {
    this->Chart->Delete();
    this->Chart = NULL;
    }
  if (LogScaleWarningLabel)
    {
    this->LogScaleWarningLabel->Delete();
    this->LogScaleWarningLabel = NULL;
    }
  this->PlotTime->Delete();
  this->PlotTime = NULL;

  this->SetInternalTitle(NULL);

  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkAbstractContextItem* vtkPVXYChartView::GetContextItem()
{
  return this->GetChart();
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetSelection(
  vtkChartRepresentation* repr, vtkSelection* selection)
{
  (void)repr;

  if (this->Chart)
    {
    // we don't support multiple selection for now.
    this->Chart->GetAnnotationLink()->SetCurrentSelection(selection);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetChartType(const char *type)
{
  if (this->Chart)
    {
    this->Chart->Delete();
    this->Chart = NULL;
    }
  if (LogScaleWarningLabel)
    {
    this->LogScaleWarningLabel->Delete();
    this->LogScaleWarningLabel = NULL;
    }

  // Construct the correct type of chart
  if (strcmp(type, "Line") == 0 || strcmp(type, "Bar") == 0)
    {
    this->Chart = vtkChartXY::New();
    }
  else if (strcmp(type, "ParallelCoordinates") == 0)
    {
    this->Chart = vtkChartParallelCoordinates::New();
    }

  if (this->Chart)
    {
    // Default to empty axis titles
    this->SetAxisTitle(0, "");
    this->SetAxisTitle(1, "");
    this->Chart->AddPlot(this->PlotTime);

    this->Chart->AddObserver(vtkCommand::SelectionChangedEvent,
      this, &vtkPVXYChartView::SelectionChanged);
    this->ContextView->GetScene()->AddItem(this->Chart);

    // setup the annotation link.
    // Unlike vtkScatterPlotMatrix, vtkChart doesn't have valid annotation link
    // setup on creation, so create one.
    if (!this->Chart->GetAnnotationLink())
      {
      vtkNew<vtkAnnotationLink> annLink;
      this->Chart->SetAnnotationLink(annLink.GetPointer());
      }

    // Set up a warning for when log-scaling is requested on negative values
    this->LogScaleWarningLabel = vtkChartWarning::New();
    this->LogScaleWarningLabel->SetLabel(
      "WARNING!\n"
      "One or more plot series crosses or contains\n"
      "an axis origin. Use the View Options menu to\n"
      "turn off log-scaling or specify a valid axis\n"
      "range; or scroll the view; or remove the line\n"
      "series from the chart in the Properties Tab."
      );
    this->LogScaleWarningLabel->SetVisible(1);
    this->LogScaleWarningLabel->SetDimensions(150, 150, 150, 150);
    this->Chart->AddItem(this->LogScaleWarningLabel);

    // setup default mouse actions
    this->Chart->SetActionToButton(vtkChart::PAN, vtkContextMouseEvent::LEFT_BUTTON);
    this->Chart->SetActionToButton(vtkChart::ZOOM_AXIS, vtkContextMouseEvent::RIGHT_BUTTON);

    // set default selection mode
    this->Chart->SetSelectionMode(vtkContextScene::SELECTION_DEFAULT);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTitle(const char* title)
{
  if (this->Chart)
    {
    std::string tmp(title);
    if (tmp.find("${TIME}") != std::string::npos)
      {
      this->SetInternalTitle(title);
      }
    else
      {
      this->Chart->SetTitle(title);
      this->SetInternalTitle(NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTitleFont(const char* family, int pointSize,
                                         bool bold, bool italic)
{
  if (this->Chart)
    {
    this->Chart->GetTitleProperties()->SetFontFamilyAsString(family);
    this->Chart->GetTitleProperties()->SetFontSize(pointSize);
    this->Chart->GetTitleProperties()->SetBold(static_cast<int>(bold));
    this->Chart->GetTitleProperties()->SetItalic(static_cast<int>(italic));
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTitleColor(double red, double green, double blue)
{
  if (this->Chart)
    {
    this->Chart->GetTitleProperties()->SetColor(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTitleAlignment(int alignment)
{
  if (this->Chart)
    {
    this->Chart->GetTitleProperties()->SetJustification(alignment);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetLegendVisibility(int visible)
{
  if (this->Chart)
    {
    this->Chart->SetShowLegend(visible? true : false);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetLegendLocation(int location)
{
  if (this->Chart)
    {
    vtkChartLegend *legend = this->Chart->GetLegend();
    legend->SetInline(location < 4);
    switch(location)
      {
      case 0: // TOP-LEFT
        legend->SetHorizontalAlignment(vtkChartLegend::LEFT);
        legend->SetVerticalAlignment(vtkChartLegend::TOP);
        break;
      case 1: // TOP-RIGHT
        legend->SetHorizontalAlignment(vtkChartLegend::RIGHT);
        legend->SetVerticalAlignment(vtkChartLegend::TOP);
        break;
      case 2: // BOTTOM-RIGHT
        legend->SetHorizontalAlignment(vtkChartLegend::RIGHT);
        legend->SetVerticalAlignment(vtkChartLegend::BOTTOM);
        break;
      case 3: // BOTTOM-LEFT
        legend->SetHorizontalAlignment(vtkChartLegend::LEFT);
        legend->SetVerticalAlignment(vtkChartLegend::BOTTOM);
        break;
      case 4: // LEFT
        legend->SetHorizontalAlignment(vtkChartLegend::LEFT);
        legend->SetVerticalAlignment(vtkChartLegend::CENTER);
        break;
      case 5: // TOP
        legend->SetHorizontalAlignment(vtkChartLegend::CENTER);
        legend->SetVerticalAlignment(vtkChartLegend::TOP);
        break;
      case 6: // RIGHT
        legend->SetHorizontalAlignment(vtkChartLegend::RIGHT);
        legend->SetVerticalAlignment(vtkChartLegend::CENTER);
        break;
      case 7: // BOTTOM
        legend->SetHorizontalAlignment(vtkChartLegend::CENTER);
        legend->SetVerticalAlignment(vtkChartLegend::BOTTOM);
        break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetGridVisibility(int index, bool visible)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetGridVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisColor(int index, double red, double green,
                                         double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetPen()->SetColorF(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetGridColor(int index, double red, double green,
                                         double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetGridPen()->SetColorF(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelVisibility(int index, bool visible)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetLabelsVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelFont(int index, const char* family,
                                             int pointSize, bool bold,
                                             bool italic)
{
  if (this->Chart)
    {
    vtkTextProperty *prop = this->Chart->GetAxis(index)->GetLabelProperties();
    prop->SetFontFamilyAsString(family);
    prop->SetFontSize(pointSize);
    prop->SetBold(static_cast<int>(bold));
    prop->SetItalic(static_cast<int>(italic));
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelColor(int index, double red,
                                              double green, double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetLabelProperties()->SetColor(red, green,
                                                                blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelNotation(int index, int notation)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetNotation(notation);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelPrecision(int index, int precision)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetPrecision(precision);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisRange(int index, double min, double max)
{
  // cache for later use.
  this->Internals->AxisRanges[index][0] = min;
  this->Internals->AxisRanges[index][1] = max;

  if (this->Chart)
    {
    vtkAxis* axis = this->Chart->GetAxis(index);
    if (axis->GetBehavior() == vtkAxis::FIXED)
      {
      // change only if axes behavior is indeed "FIXED" i.e.
      // SetAxisUseCustomRange(...) was set to true for this axis.
      if (axis->GetUnscaledMinimum() != min || axis->GetUnscaledMaximum() != max)
        {
        axis->SetUnscaledMinimum(min);
        axis->SetUnscaledMaximum(max);
        this->Chart->RecalculateBounds();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisUseCustomRange(int index, bool useCustomRange)
{
  if (this->Chart)
    {
    vtkAxis* axis = this->Chart->GetAxis(index);
    if (useCustomRange && (axis->GetBehavior() != vtkAxis::FIXED))
      {
      axis->SetBehavior(vtkAxis::FIXED);
      axis->SetUnscaledMinimum(this->Internals->AxisRanges[index][0]);
      axis->SetUnscaledMaximum(this->Internals->AxisRanges[index][1]);
      this->Chart->RecalculateBounds();
      }
    else if (!useCustomRange && (axis->GetBehavior() != vtkAxis::AUTO))
      {
      axis->SetBehavior(vtkAxis::AUTO);
      // set to some value so we notice when this gets used.
      axis->SetMinimum(0.0);
      axis->SetMaximum(6.66);
      this->Chart->RecalculateBounds();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLogScale(int index, bool logScale)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetLogScale(logScale);
    this->Chart->Update();
    this->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisTitle(int index, const char* title)
{
  if (this->Chart && this->Chart->GetAxis(index))
    {
    this->Chart->GetAxis(index)->SetTitle(title);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisTitleFont(int index, const char* family,
                                             int pointSize, bool bold,
                                             bool italic)
{
  if (this->Chart)
    {
    vtkTextProperty *prop = this->Chart->GetAxis(index)->GetTitleProperties();
    prop->SetFontFamilyAsString(family);
    prop->SetFontSize(pointSize);
    prop->SetBold(static_cast<int>(bold));
    prop->SetItalic(static_cast<int>(italic));
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisTitleColor(int index, double red,
                                              double green, double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetTitleProperties()->SetColor(red, green,
                                                                blue);
    }
}


//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisUseCustomLabels(int index, bool use_custom_labels)
{
  if (index >= 0 && index < 4)
    {
    this->Internals->UseCustomLabels[index] = use_custom_labels;
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsNumber(int axis, int n)
{
  if (axis >=0 && axis < 4)
    {
    this->Internals->CustomLabelPositions[axis]->SetNumberOfTuples(n);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabels(int axis, int i, double value)
{
  if (axis >=0 && axis < 4)
    {
    this->Internals->CustomLabelPositions[axis]->SetValue(i, value);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsLeftNumber(int n)
{
  this->SetAxisLabelsNumber(vtkAxis::LEFT, n);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsLeft(int i, double value)
{
  this->SetAxisLabels(vtkAxis::LEFT, i, value);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsBottomNumber(int n)
{
  this->SetAxisLabelsNumber(vtkAxis::BOTTOM, n);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsBottom(int i, double value)
{
  this->SetAxisLabels(vtkAxis::BOTTOM, i, value);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsRightNumber(int n)
{
  this->SetAxisLabelsNumber(vtkAxis::RIGHT, n);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsRight(int i, double value)
{
  this->SetAxisLabels(vtkAxis::RIGHT, i, value);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsTopNumber(int n)
{
  this->SetAxisLabelsNumber(vtkAxis::TOP, n);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsTop(int i, double value)
{
  this->SetAxisLabels(vtkAxis::TOP, i, value);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTooltipNotation(int notation)
{
  for(int i = 0; i < this->Chart->GetNumberOfPlots(); i++)
    {
    this->Chart->GetPlot(i)->SetTooltipNotation(notation);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTooltipPrecision(int precision)
{
  for(int i = 0; i < this->Chart->GetNumberOfPlots(); i++)
    {
    this->Chart->GetPlot(i)->SetTooltipPrecision(precision);
    }
}

//----------------------------------------------------------------------------
vtkChart* vtkPVXYChartView::GetChart()
{
  return this->Chart;
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::Render(bool interactive)
{
  if (!this->Chart)
    {
    return;
    }
  if (this->InternalTitle)
    {
    vtksys_ios::ostringstream new_title;
    std::string title(this->InternalTitle);
    size_t pos = title.find("${TIME}");
    if (pos != std::string::npos)
      {
      // The string was found - replace it and set the chart title.
      new_title << title.substr(0, pos)
                << this->GetViewTime()
                << title.substr(pos + strlen("${TIME}"));
      this->Chart->SetTitle(new_title.str().c_str());
      }
    }

  this->PlotTime->SetTime(this->GetViewTime());
  this->PlotTime->SetTimeAxisMode(vtkPVPlotTime::NONE);

  // Decide if time is being shown on any of the axis.
  // Iterate over all visible representations and check is they have the array
  // named "Time" on either of the axes.
  int num_reprs = this->GetNumberOfRepresentations();
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkXYChartRepresentation * repr = vtkXYChartRepresentation::SafeDownCast(
      this->GetRepresentation(cc));
    if (repr && repr->GetVisibility())
      {
      if (repr->GetXAxisSeriesName() &&
        strcmp(repr->GetXAxisSeriesName(), "Time") == 0)
        {
        this->PlotTime->SetTimeAxisMode(vtkPVPlotTime::X_AXIS);
        break;
        }
      }
    }
  // For now we only handle X-axis time. If needed we can add support for Y-axis.

  // handle custom labels. We specify custom labels in render since vtkAxis will
  // discard the custom labels when the mode was set to not use custom labels,
  // so we need to provide the labels each time to the chart.
  for (int axis=0; axis < 4 && axis < this->Chart->GetNumberOfAxes(); axis++)
    {
    vtkAxis* chartAxis= this->Chart->GetAxis(axis);
    if(!chartAxis)
      {
      continue;
      }
    if (this->Internals->UseCustomLabels[axis])
      {
      vtkSmartPointer<vtkDoubleArray> ticks =
        this->Internals->CustomLabelPositions[axis].GetPointer();
      if (chartAxis->GetLogScaleActive())
        {
        vtkNew<vtkDoubleArray> logTicks;
        logTicks->DeepCopy(ticks.GetPointer());
        double* p = logTicks->GetPointer(0);
        for (vtkIdType i = 0; i <= logTicks->GetMaxId(); ++i, ++p)
          {
          *p = log10(fabs(*p));
          }
        ticks = logTicks.GetPointer();
        }
      chartAxis->SetCustomTickPositions(ticks.GetPointer());
      }
    else
      {
      chartAxis->SetCustomTickPositions(NULL);
      }
    chartAxis->Update();
    }

  this->Superclass::Render(interactive);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SelectionChanged()
{
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
