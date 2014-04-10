/*=========================================================================

   Program:   ParaView
   Module:    pqImageUtil.cxx

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

#include "pqImageUtil.h"

#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkSMUtilities.h"

#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPrinter>

// NOTES:
// - QImage pixel data is 32 bit aligned, whereas vtkImageData's pixel data 
//   is not necessarily 32 bit aligned
// - QImage is a mirror of vtkImageData, which is taken care of by scanning
//   a row at a time and copying opposing rows

//-----------------------------------------------------------------------------
bool pqImageUtil::toImageData(const QImage& img, vtkImageData* vtkimage)
{
  int height = img.height();
  int width = img.width();
  int numcomponents = img.hasAlphaChannel() ? 4 : 3;

  vtkimage->SetExtent(0, width-1, 0, height-1, 0, 0);
  vtkimage->SetSpacing(1.0, 1.0, 1.0);
  vtkimage->SetOrigin(0.0, 0.0, 0.0);
  vtkimage->AllocateScalars(VTK_UNSIGNED_CHAR, numcomponents);
  for(int i=0; i<height; i++)
    {
    unsigned char* row;
    row = static_cast<unsigned char*>(vtkimage->GetScalarPointer(0, height-i-1, 0));
    const QRgb* linePixels = reinterpret_cast<const QRgb*>(img.scanLine(i));
    for(int j=0; j<width; j++)
      {
      const QRgb& col = linePixels[j];
      row[j*numcomponents] = qRed(col);
      row[j*numcomponents+1] = qGreen(col);
      row[j*numcomponents+2] = qBlue(col);
      if(numcomponents == 4)
        {
        row[j*numcomponents+3] = qAlpha(col);
        }
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
bool pqImageUtil::fromImageData(vtkImageData* vtkimage, QImage& img)
{
  if (vtkimage->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    return false;
    }

  int extent[6];
  vtkimage->GetExtent(extent);
  int width = extent[1]-extent[0]+1;
  int height = extent[3]-extent[2]+1;
  int numcomponents = vtkimage->GetNumberOfScalarComponents();
  if(!(numcomponents == 3 || numcomponents == 4))
    {
    return false;
    }
  
  QImage newimg(width, height, QImage::Format_ARGB32);

  for(int i=0; i<height; i++)
    {
    QRgb* bits = reinterpret_cast<QRgb*>(newimg.scanLine(i));
    unsigned char* row;
    row = static_cast<unsigned char*>(
      vtkimage->GetScalarPointer(extent[0], extent[2] + height-i-1, extent[4]));
    for(int j=0; j<width; j++)
      {
      unsigned char* data = &row[j*numcomponents];
      bits[j] = numcomponents == 4 ? 
        qRgba(data[0], data[1], data[2], data[3]) :
        qRgb(data[0], data[1], data[2]);
      }
    }

  img = newimg;
  return true;
}

//-----------------------------------------------------------------------------
int pqImageUtil::saveImage(vtkImageData* vtkimage, const QString& filename, 
  int quality/*=-1*/)
{
  int error_code = vtkErrorCode::NoError;
  if (!vtkimage)
    {
    return vtkErrorCode::UnknownError;
    }
  if (filename.isEmpty())
    {
    return vtkErrorCode::NoFileNameError;
    }

  const QFileInfo file(filename);
  if (file.suffix() == "pdf")
    {
    // For pdf saving, we need to use Qt.
    QImage qimage;
    if (pqImageUtil::fromImageData(vtkimage, qimage))
      {
      error_code = pqImageUtil::saveImage(qimage, filename);
      }
    else
      {
      error_code = vtkErrorCode::UnknownError;
      }
    }
  else
    {
    error_code = vtkSMUtilities::SaveImage(vtkimage, filename.toAscii().data(), 
      quality);
    }

  return error_code;
}

//-----------------------------------------------------------------------------
int pqImageUtil::saveImage(const QImage& qimage, const QString& filename,
  int quality/*=-1*/)
{
  int error_code = vtkErrorCode::NoError;
  if (qimage.isNull())
    {
    return vtkErrorCode::UnknownError;
    }

  if (filename.isEmpty())
    {
    return vtkErrorCode::NoFileNameError;
    }

  const QFileInfo file(filename);
  if (file.suffix() == "pdf")
    {
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filename);

    QPainter painter;
    painter.begin(&printer);
    QSize viewport_size(qimage.size());
    viewport_size.scale(printer.pageRect().size(), Qt::KeepAspectRatio);
    painter.setWindow(qimage.rect());
    painter.setViewport(QRect(0,0, viewport_size.width(),
        viewport_size.height()));
    painter.drawImage(QPointF(0.0, 0.0), qimage);
    painter.end();

    // TODO: add some error checking.
    error_code = vtkErrorCode::NoError;
    }
  else
    {
    // Use VTK for saving image, so that 3 component images are saved as needed
    // by vtk for testing etc.
    vtkImageData* vtkimage = vtkImageData::New();
    if (pqImageUtil::toImageData(qimage, vtkimage))
      {
      error_code = pqImageUtil::saveImage(vtkimage, filename, quality);
      }
    else
      {
      error_code = vtkErrorCode::UnknownError;
      }
    }

  return error_code;
}

