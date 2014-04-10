/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataSetAlgorithmSelectorFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataSetAlgorithmSelectorFilter.h"

#include "vtkAlgorithm.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridAlgorithm.h"

#include <assert.h>
#include <map>
#include <set>
#include <vector>

class vtkPVDataSetAlgorithmSelectorFilter::vtkInternals
{
public:
  std::vector<vtkSmartPointer<vtkAlgorithm> > RegisteredFilters;
  int ActiveFilter;
  vtkObject* Owner;

  vtkInternals() : ActiveFilter(0) { }

};


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDataSetAlgorithmSelectorFilter);
//----------------------------------------------------------------------------
vtkPVDataSetAlgorithmSelectorFilter::vtkPVDataSetAlgorithmSelectorFilter()
{
  this->Internal = new vtkInternals();
  this->SetNumberOfOutputPorts(1);
  this->SetNumberOfInputPorts(1);
  this->OutputType = VTK_DATA_SET;
}
//----------------------------------------------------------------------------
vtkPVDataSetAlgorithmSelectorFilter::~vtkPVDataSetAlgorithmSelectorFilter()
{
  delete this->Internal; this->Internal = NULL;
}
//----------------------------------------------------------------------------
int vtkPVDataSetAlgorithmSelectorFilter::ProcessRequest(
    vtkInformation *request,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  // Make sure the output object is created with the correct type
  if(request->Has(vtkStreamingDemandDrivenPipeline::
     REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(NULL, NULL, outputVector);
    }
  else
    {
    // Use the real filter to process the input
    vtkAlgorithm* activeFilterToUse = this->GetActiveFilter();
    vtkDebugMacro("Executing: " << activeFilterToUse->GetClassName());

    if(activeFilterToUse)
      {
      return activeFilterToUse->ProcessRequest(request, inputVector, outputVector);
      }
    else
      {
      vtkErrorMacro("Algorithm of type " << activeFilterToUse->GetClassName() << " is not supported yet");
      }
    return 1;
    }
}
//----------------------------------------------------------------------------
int vtkPVDataSetAlgorithmSelectorFilter::ProcessRequest(
    vtkInformation *request,
    vtkCollection *inputVector,
    vtkInformationVector *outputVector)
{
  // Make sure the output object is created with the correct type
  if(request->Has(vtkStreamingDemandDrivenPipeline::
     REQUEST_DATA_OBJECT()))
    {
    this->RequestDataObject(NULL, NULL, outputVector);
    }
  else
    {
    // Use the real filter to process the input
    vtkAlgorithm* activeFilterToUse = this->GetActiveFilter();
    vtkDebugMacro("Executing: " << activeFilterToUse->GetClassName());

    if(activeFilterToUse)
      {
      return activeFilterToUse->ProcessRequest(request, inputVector, outputVector);
      }
    else
      {
      vtkErrorMacro("Algorithm of type " << activeFilterToUse->GetClassName() << " is not supported yet");
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAlgorithmSelectorFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  size_t size = this->GetNumberOfFilters();
  for(size_t i = 0; i < size; i++)
    {
    os << indent << "Filter " << i << ": "
       << this->Internal->RegisteredFilters.at(i)->GetClassName() << "\n";
    }
  if(size == 0)
    {
    os << indent << "No registered filter available\n";
    }
}
//----------------------------------------------------------------------------
int vtkPVDataSetAlgorithmSelectorFilter::RegisterFilter(vtkAlgorithm *filter)
{
  int size = static_cast<int>(this->Internal->RegisteredFilters.size());
  this->Internal->RegisteredFilters.push_back(filter);
  this->Modified();
  return size;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAlgorithmSelectorFilter::UnRegisterFilter(int index)
{
  int size = static_cast<int>(this->Internal->RegisteredFilters.size());
  if( index >= 0 && index < size )
    {
    std::vector<vtkSmartPointer<vtkAlgorithm> >::iterator iter =
        this->Internal->RegisteredFilters.begin();
    iter += index;
    this->Internal->RegisteredFilters.erase(iter);

    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAlgorithmSelectorFilter::ClearFilters()
{
  this->Internal->RegisteredFilters.clear();
}

//----------------------------------------------------------------------------
int vtkPVDataSetAlgorithmSelectorFilter::GetNumberOfFilters()
{
  return static_cast<int>(this->Internal->RegisteredFilters.size());
}

//----------------------------------------------------------------------------
vtkAlgorithm* vtkPVDataSetAlgorithmSelectorFilter::GetFilter(int index)
{
  int size = static_cast<int>(this->Internal->RegisteredFilters.size());
  if( index >= 0 && index < size )
    {
    return this->Internal->RegisteredFilters.at(index);
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkAlgorithm* vtkPVDataSetAlgorithmSelectorFilter::GetActiveFilter()
{
  return this->GetFilter(this->Internal->ActiveFilter);
}

//----------------------------------------------------------------------------
vtkAlgorithm *vtkPVDataSetAlgorithmSelectorFilter::SetActiveFilter(int index)
{
  int size = static_cast<int>(this->Internal->RegisteredFilters.size());
  if( index >= 0 && index < size )
    {
    this->Internal->ActiveFilter = index;
    this->Modified();
    return this->Internal->RegisteredFilters.at(index);
    }
  return NULL;
}
//----------------------------------------------------------------------------
// Overload standard modified time function. If cut functions is modified,
// or contour values modified, then this object is modified as well.
unsigned long vtkPVDataSetAlgorithmSelectorFilter::GetMTime()
{
  unsigned long maxMTime = this->Superclass::GetMTime(); // My MTime

  // let's check internals MTimes
  std::vector<vtkSmartPointer<vtkAlgorithm> >::iterator filterIter =
      this->Internal->RegisteredFilters.begin();
  while(filterIter != this->Internal->RegisteredFilters.end())
    {
    unsigned long filterMTime = filterIter->GetPointer()->GetMTime();
    maxMTime = (maxMTime > filterMTime) ? maxMTime : filterMTime;

    // Move forward
    filterIter++;
    }

  return maxMTime;
}
//----------------------------------------------------------------------------
int vtkPVDataSetAlgorithmSelectorFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVDataSetAlgorithmSelectorFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;
}
//----------------------------------------------------------------------------

int vtkPVDataSetAlgorithmSelectorFilter::RequestDataObject(vtkInformation *, vtkInformationVector**,
                                      vtkInformationVector* outputVector)
{
  vtkInformation *info = outputVector->GetInformationObject(0);

  switch(this->OutputType)
    {
    case VTK_POLY_DATA:
      if (vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT())))
        {
        // The output is already created
        return 1;
        }
      else
        {
        vtkPolyData* output = vtkPolyData::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->FastDelete();
        this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
                                               output->GetExtentType());
        }
      break;
    case VTK_UNSTRUCTURED_GRID:
      if (vtkUnstructuredGrid::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT())))
        {
        // The output is already created
        return 1;
        }
      else
        {
        vtkUnstructuredGrid* output = vtkUnstructuredGrid::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->FastDelete();
        this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
                                               output->GetExtentType());
        }
      break;
    default:
      vtkErrorMacro("Invalid output type");
      return 0;
    }
  return 1;
}
