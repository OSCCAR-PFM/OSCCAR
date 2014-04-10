/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMultiServerDataSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMultiServerDataSource
// .SECTION Description
// VTK class that handle the fetch of remote data
#include "vtkPVMultiServerDataSource.h"

#include "vtkDataObjectTypes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkType.h"
#include "vtkWeakPointer.h"

#include <string>

//*****************************************************************************
struct vtkPVMultiServerDataSource::vtkInternal
{
  vtkNew<vtkPVDataInformation> OutputPortInformation;
  vtkWeakPointer<vtkSMSourceProxy> ExternalProxy;
  int PortToExport;
  int DataTypeToUse;
  unsigned long LastUpdatedDataTimeStamp;

  vtkInternal() : DataTypeToUse(VTK_DATA_OBJECT) {};

  void InitializeDataStructure(vtkSMSourceProxy* proxyFromAnotherServer, int portNumber)
  {
    this->LastUpdatedDataTimeStamp = 0;
    this->PortToExport = portNumber;
    this->ExternalProxy = proxyFromAnotherServer;
    this->OutputPortInformation->Initialize();
    if(this->ExternalProxy)
      {
      this->OutputPortInformation->AddInformation(this->ExternalProxy->GetDataInformation());
      this->DataTypeToUse = this->OutputPortInformation->GetDataSetType();
      if(this->OutputPortInformation->GetCompositeDataInformation()->GetDataIsComposite())
        {
        this->DataTypeToUse = this->OutputPortInformation->GetCompositeDataSetType();
        }
      }
    else
      {
      this->DataTypeToUse = VTK_DATA_OBJECT;
      }
  }
};
vtkStandardNewMacro(vtkPVMultiServerDataSource);
//----------------------------------------------------------------------------
vtkPVMultiServerDataSource::vtkPVMultiServerDataSource()
{
  this->Internal = new vtkInternal();
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVMultiServerDataSource::~vtkPVMultiServerDataSource()
{
  delete this->Internal;
  this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkPVMultiServerDataSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
void vtkPVMultiServerDataSource::SetExternalProxy(
    vtkSMSourceProxy* proxyFromAnotherServer, int portNumber)
{
  this->Internal->InitializeDataStructure(proxyFromAnotherServer, portNumber);
  this->Modified();
}
//---------------------------------------------------------------------------
void vtkPVMultiServerDataSource::FetchData(vtkDataObject *dataObjectToFill)
{
  if( this->Internal->ExternalProxy &&
      this->Internal->LastUpdatedDataTimeStamp < this->GetMTime())
    {
    this->Internal->LastUpdatedDataTimeStamp = this->GetMTime();

    // If fetching data from a distributed server handle that with a reducer
    vtkSmartPointer<vtkSMSourceProxy> reductionFilter;
    reductionFilter.TakeReference(
          vtkSMSourceProxy::SafeDownCast(
            this->Internal->ExternalProxy->GetSessionProxyManager()->NewProxy(
              "filters", "ReductionFilter", NULL)));

    // Set default appender
    std::string postGatherHelperName = "vtkAppendFilter";
    int datasetType = this->Internal->OutputPortInformation->GetDataSetType();

    // Handle custom ones
    if(this->Internal->OutputPortInformation->GetCompositeDataInformation()->GetDataIsComposite())
      {
      postGatherHelperName = "vtkMultiBlockDataGroupFilter";
      }
    else if(datasetType == VTK_POLY_DATA)
      {
      postGatherHelperName = "vtkAppendPolyData";
      }
    else if(datasetType == VTK_RECTILINEAR_GRID)
      {
      postGatherHelperName = "vtkAppendRectilinearGrid";
      }

    // Set the post gather helper if any
    if(!postGatherHelperName.empty())
      {
      vtkSMPropertyHelper(reductionFilter, "PostGatherHelperName").Set(
            postGatherHelperName.c_str());
      }

    // Reduce the data
    vtkSMPropertyHelper(reductionFilter, "Input").Set(
          this->Internal->ExternalProxy, this->Internal->PortToExport);
    reductionFilter->UpdateVTKObjects();
    reductionFilter->UpdatePipeline();
    vtkPVDataInformation* dataInfo = reductionFilter->GetDataInformation();

    // Handle type
    int dataType = dataInfo->GetDataSetType();
    if(dataInfo->GetCompositeDataSetType() > 0)
      {
      dataType = dataInfo->GetCompositeDataSetType();
      }

    // Handle extent
    int extent[6];
    dataInfo->GetExtent(extent);

    // Based on the reduced data fetch it
    vtkSmartPointer<vtkSMSourceProxy> fetcher;
    fetcher.TakeReference(
          vtkSMSourceProxy::SafeDownCast(
            reductionFilter->GetSessionProxyManager()->NewProxy(
              "filters", "ClientServerMoveData", NULL)));
    vtkSMPropertyHelper(fetcher, "Input").Set(reductionFilter);
    vtkSMPropertyHelper(fetcher, "OutputDataType").Set(dataType);
    vtkSMPropertyHelper(fetcher, "WholeExtent").Set(extent, 6);
    fetcher->UpdateVTKObjects();
    fetcher->UpdatePipeline();

    // Shallow copy the local fetched data into our local output
    vtkDataObjectAlgorithm* localObj =
        vtkDataObjectAlgorithm::SafeDownCast(fetcher->GetClientSideObject());
    dataObjectToFill->ShallowCopy(localObj->GetOutputDataObject(0));
    }
}

//---------------------------------------------------------------------------
int vtkPVMultiServerDataSource::RequestDataObject(
    vtkInformation *, vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);
  int datasetType = this->Internal->DataTypeToUse;

  // Make sure our output is of the correct type
  if( output == NULL || output->GetDataObjectType() != datasetType )
    {
    output = vtkDataObjectTypes::NewDataObject(datasetType);
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->FastDelete();
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkPVMultiServerDataSource::RequestInformation(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  // Provide the time informations if any
  vtkInformation *info = outputVector->GetInformationObject(0);

  // Deal with Number of pieces
  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);

  return 1;
}

//---------------------------------------------------------------------------
int vtkPVMultiServerDataSource::RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  // As we don't support Image data we don't care
  return 1;
}

//---------------------------------------------------------------------------
int vtkPVMultiServerDataSource::RequestData(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  // The real fetch will only happen if its really needed
  this->FetchData(output);

  return 1;
}
