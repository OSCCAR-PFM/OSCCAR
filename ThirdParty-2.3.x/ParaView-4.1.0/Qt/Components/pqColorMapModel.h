/*=========================================================================

   Program: ParaView
   Module:    pqColorMapModel.h

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

/// \file pqColorMapModel.h
/// \date 3/9/2007

#ifndef _pqColorMapModel_h
#define _pqColorMapModel_h


#include "pqComponentsModule.h"
#include <QObject>
#include <QPixmap> // Needed for return type.
#include <QString> // Needed for return type.
#include <QVariant> // Needed for setAnnotations(), getAnnotations().

class pqChartValue;
class pqColorMapModelInternalPts;
class pqColorMapModelInternalTxt;
class QColor;
class QSize;

/// Model for colormaps used to hold presets, import from XML, and export to XML.
class PQCOMPONENTS_EXPORT pqColorMapModel : public QObject
{
  Q_OBJECT

public:
  enum ColorSpace
    {
    RgbSpace,
    HsvSpace,
    WrappedHsvSpace,
    LabSpace,
    DivergingSpace
    };

public:
  pqColorMapModel(QObject *parent=0);
  pqColorMapModel(const pqColorMapModel &other);
  virtual ~pqColorMapModel();

  ColorSpace getColorSpace() const {return this->Space;}
  void setColorSpace(ColorSpace space);

  int getColorSpaceAsInt() const;
  void setColorSpaceFromInt(int space);

  int getNumberOfPoints() const;
  void addPoint(const pqChartValue &value, const QColor &color);
  void addPoint(const pqChartValue &value, const QColor &color,
      const pqChartValue &opacity);
  void removePoint(int index);
  void removeAllPoints();
  void startModifyingData();
  bool isDataBeingModified() const {return this->InModify;}
  void finishModifyingData();

  void getPointValue(int index, pqChartValue &value) const;
  void setPointValue(int index, const pqChartValue &value);
  void getPointColor(int index, QColor &color) const;
  void setPointColor(int index, const QColor &color);
  void getPointOpacity(int index, pqChartValue &opacity) const;
  void setPointOpacity(int index, const pqChartValue &opacity);

  bool isRangeNormalized() const;

  void getValueRange(pqChartValue &min, pqChartValue &max) const;

  void getNanColor(QColor &color) const;
  void setNanColor(const QColor &color);

  bool getIndexedLookup() const;
  void setIndexedLookup( bool isCategorical );

  int getNumberOfAnnotations() const;
  QString getAnnotatedValue( int ) const;
  QString getAnnotation( int ) const;
  QList<QVariant> getAnnotations() const;
  int addAnnotation( const QString& value, const QString& note );
  int insertAnnotation( int index, const QString& value, const QString& note );
  void removeAnnotation( int index );
  void removeAllAnnotations();

  /// \brief
  ///   Scales the current points to fit in the given range.
  /// \note
  ///   If there are no points, this method does nothing.
  /// \param min The minimum value.
  /// \param max The maximum value.
  void setValueRange(const pqChartValue &min, const pqChartValue &max);

  QPixmap generateGradient(const QSize &size) const;

  pqColorMapModel &operator=(const pqColorMapModel &other);

  static void RGBToLab(double red, double green, double blue,
                       double *L, double *a, double *b);
  static void LabToRGB(double L, double a, double b,
                       double *red, double *green, double *blue);

  static void RGBToMsh(double red, double green, double blue,
                       double *M, double *s, double *h);
  static void MshToRGB(double M, double s, double h,
                       double *red, double *green, double *blue);

protected:
  virtual QPixmap generateIntervalPreview(const QSize &size) const;
  virtual QPixmap generateCategoricalPreview(const QSize &size) const;

signals:
  /// Emitted when the color space changes.
  void colorSpaceChanged();

  /// Emitted when the table size changes.
  void tableSizeChanged();

  /// \brief
  ///   Emitted when the color for a point changes.
  /// \param index The point index.
  /// \param color The point's new color.
  void colorChanged(int index, const QColor &color);

  /// Emitted when the NaN color changes.
  /// \param color The new NaN color.
  void nanColorChanged(const QColor &color);

  /// Emitted when all or many of the points have changed.
  void pointsReset();

  /// \brief
  ///   Emitted after a point has been added.
  /// \param index The index of the new point.
  void pointAdded(int index);

  /// \brief
  ///   Emitted before a point is removed.
  /// \param index The index of the point to be removed.
  void removingPoint(int index);

  /// \brief
  ///   Emitted after a point is removed.
  /// \param index The index of the removed point.
  void pointRemoved(int index);

  /// \brief
  ///   Emitted when the value of a point has been changed.
  /// \param index The index of the point.
  /// \param value The new value for the point.
  void valueChanged(int index, const pqChartValue &value);

  /// \brief
  ///   Emitted when the opacity of a point has been changed.
  /// \param index The index of the point.
  /// \param opacity The new opacity for the point.
  void opacityChanged(int index, const pqChartValue &opacity);

  /// \brief
  ///   Emitted when the color loookup mode changes.
  /// \param The new value for the lookup mode (true for categorical/indexed lookup, false otherwise)
  void indexedLookupChanged( bool newIndexLookupValue );

  /// Emitted when all or many of the annotations have changed.
  void annotationsReset();

  /// \brief
  ///   Emitted after an annotation has been added.
  /// \param index The index of the new annotation.
  void annotationAdded( int index );

  /// \brief
  ///   Emitted before an annotation is removed.
  /// \param index The index of the annotation to be removed.
  void removingAnnotation( int index );

  /// \brief
  ///   Emitted after an annotation is removed.
  /// \param index The index of the removed annotation.
  void annotationRemoved( int index );

  /// \brief
  ///   Emitted when an annotated value has been changed.
  /// \param index The index of the annotation.
  /// \param value The new annotated value.
  void annotatedValueChanged( int index, const QString& value );

  /// \brief
  ///   Emitted when the annotation text of an annotation has been changed.
  /// \param index The index of the annotation.
  /// \param note The new text of the annotation.
  void annotationChanged( int index, const QString& note );

private:
  pqColorMapModelInternalPts* InternalPts; ///< Stores the points.
  pqColorMapModelInternalTxt* InternalTxt; ///< Stores the annotations.
  ColorSpace Space;                        ///< Stores the color space.
  QColor NanColor;                         ///< Stores the NaN color.
  bool IndexedLookup;                      ///< True if annotations should be mapped by index to colors.
  bool InModify;                           ///< True if in modify mode.
};

#endif
