/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoundsDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBoundsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

vtkStandardNewMacro(vtkSMBoundsDomain);
//---------------------------------------------------------------------------
vtkSMBoundsDomain::vtkSMBoundsDomain()
{
  this->Mode = vtkSMBoundsDomain::NORMAL;
  this->ScaleFactor = 0.1;
}

//---------------------------------------------------------------------------
vtkSMBoundsDomain::~vtkSMBoundsDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::Update(vtkSMProperty*)
{
  if (this->Mode == vtkSMBoundsDomain::ORIENTED_MAGNITUDE)
    {
    this->UpdateOriented();
    return;
    }

  vtkPVDataInformation* info = this->GetInputInformation();
  if (info)
    {
    double bounds[6];
    info->GetBounds(bounds);
    this->SetDomainValues(bounds);
    }
}

//---------------------------------------------------------------------------
vtkPVDataInformation* vtkSMBoundsDomain::GetInputInformation()
{
  vtkSMProperty* inputProperty = this->GetRequiredProperty("Input");
  if (!inputProperty)
    {
    vtkErrorMacro("Missing required property with function 'Input'");
    return NULL;
    }

  vtkSMUncheckedPropertyHelper helper(inputProperty);
  if (helper.GetNumberOfElements() > 0)
    {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0));
    if (sp)
      {
      return sp->GetDataInformation(helper.GetOutputPort());
      }
    }
  return NULL;
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::UpdateOriented()
{
  vtkPVDataInformation* inputInformation = this->GetInputInformation();
  if (!inputInformation)
    {
    return;
    }

  double bounds[6];
  inputInformation->GetBounds(bounds);

  vtkSMDoubleVectorProperty *normal = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetRequiredProperty("Normal"));
  vtkSMDoubleVectorProperty *origin = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetRequiredProperty("Origin"));
  if (normal && origin)
    {
    double points[8][3];
    
    double xmin = bounds[0];
    double xmax = bounds[1];
    double ymin = bounds[2];
    double ymax = bounds[3];
    double zmin = bounds[4];
    double zmax = bounds[5];

    points[0][0] = xmin; points[0][1] = ymin; points[0][2] = zmin;
    points[1][0] = xmax; points[1][1] = ymax; points[1][2] = zmax;
    points[2][0] = xmin; points[2][1] = ymin; points[2][2] = zmax;
    points[3][0] = xmin; points[3][1] = ymax; points[3][2] = zmax;
    points[4][0] = xmin; points[4][1] = ymax; points[4][2] = zmin;
    points[5][0] = xmax; points[5][1] = ymax; points[5][2] = zmin;
    points[6][0] = xmax; points[6][1] = ymin; points[6][2] = zmin;
    points[7][0] = xmax; points[7][1] = ymin; points[7][2] = zmax;

    double normalv[3], originv[3];

    unsigned int i;
    if (normal->GetNumberOfUncheckedElements() > 2 && 
        origin->GetNumberOfUncheckedElements() > 2)
      {
      for (i=0; i<3; i++)
        {
        normalv[i] = normal->GetUncheckedElement(i);
        originv[i] = origin->GetUncheckedElement(i); 
        }
      }
    else
      {
      return;
      }

    unsigned int j;
    double dist[8];

    for(i=0; i<8; i++)
      {
      dist[i] = 0;
      for(j=0; j<3; j++)
        {
        dist[i] += (points[i][j] - originv[j])*normalv[j];
        }
      }

    double min = dist[0], max = dist[0];
    for (i=1; i<8; i++)
      {
      if ( dist[i] < min )
        {
        min = dist[i];
        }
      if ( dist[i] > max )
        {
        max = dist[i];
        }
      }

    std::vector<vtkEntry> entries;
    entries.push_back(vtkEntry(min, max));
    this->SetEntries(entries); 
    }
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::SetDomainValues(double bounds[6])
{
  if (this->Mode == vtkSMBoundsDomain::NORMAL)
    {
    std::vector<vtkEntry> entries;
    for (int j = 0; j < 3; j++)
      {
      entries.push_back(vtkEntry(bounds[2*j], bounds[2*j+1]));
      }
    this->SetEntries(entries);
    }
  else if (this->Mode == vtkSMBoundsDomain::MAGNITUDE)
    {
    double magn = sqrt((bounds[1]-bounds[0]) * (bounds[1]-bounds[0]) +
                       (bounds[3]-bounds[2]) * (bounds[3]-bounds[2]) +
                       (bounds[5]-bounds[4]) * (bounds[5]-bounds[4]));
    // Never use 0 min/max.
    if (magn == 0)
      {
      magn = 1;
      }
    std::vector<vtkEntry> entries;
    entries.push_back(vtkEntry(-magn/2.0, magn/2.0));
    this->SetEntries(entries);
    }
  else if (this->Mode == vtkSMBoundsDomain::SCALED_EXTENT)
    {
    double maxbounds = bounds[1] - bounds[0];
    maxbounds = (bounds[3] - bounds[2] > maxbounds) ? (bounds[3] - bounds[2]) : maxbounds;
    maxbounds = (bounds[5] - bounds[4] > maxbounds) ? (bounds[5] - bounds[4]) : maxbounds;
    maxbounds *= this->ScaleFactor;
    // Never use 0 maxbounds.
    if (maxbounds == 0)
      {
      maxbounds = this->ScaleFactor;
      }
    std::vector<vtkEntry> entries;
    entries.push_back(vtkEntry(0, maxbounds));
    this->SetEntries(entries);
    }
}

//---------------------------------------------------------------------------
int vtkSMBoundsDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
    {
    return 0;
    }

  bool has_default_mode = (element->GetAttribute("default_mode") != NULL);
  const char* mode = element->GetAttribute("mode");
  if (mode)
    {
    if (strcmp(mode, "normal") == 0)
      {
      this->Mode = vtkSMBoundsDomain::NORMAL;
      }
    else if (strcmp(mode, "magnitude") == 0)
      {
      this->Mode = vtkSMBoundsDomain::MAGNITUDE;
      }
    else if (strcmp(mode, "oriented_magnitude") == 0)
      {
      this->Mode = vtkSMBoundsDomain::ORIENTED_MAGNITUDE;
      }
    else if (strcmp(mode, "scaled_extent") == 0)
      {
      this->Mode = vtkSMBoundsDomain::SCALED_EXTENT;
      if (!has_default_mode)
        {
        this->DefaultMode = vtkSMDoubleRangeDomain::MAX;
        }
      }
    else
      {
      vtkErrorMacro("Unrecognized mode: " << mode);
      return 0;
      }
    }

  const char* scalefactor = element->GetAttribute("scale_factor");
  if (scalefactor)
    {
    sscanf(scalefactor,"%lf", &this->ScaleFactor);
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Mode: " << this->Mode << endl;
  os << indent << "ScaleFactor: " << this->ScaleFactor << endl;
}
