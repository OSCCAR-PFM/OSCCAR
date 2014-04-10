/*=========================================================================

  Program:   ParaView
  Module:    vtkBlockDeliveryPreprocessor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBlockDeliveryPreprocessor.h"

#include "vtkAttributeDataToTableFilter.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkExtractBlock.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSplitColumnComponents.h"
#include "vtkTable.h"
#include "vtkUniformGridAMRDataIterator.h"

vtkStandardNewMacro(vtkBlockDeliveryPreprocessor);
//----------------------------------------------------------------------------
vtkBlockDeliveryPreprocessor::vtkBlockDeliveryPreprocessor()
{
  this->CompositeDataSetIndex = 0;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  this->FlattenTable = 0;
  this->GenerateOriginalIds = true;
  this->GenerateCellConnectivity = false;
}

//----------------------------------------------------------------------------
vtkBlockDeliveryPreprocessor::~vtkBlockDeliveryPreprocessor()
{
}

//----------------------------------------------------------------------------
int vtkBlockDeliveryPreprocessor::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }

  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::GetData(inInfo);
  vtkDataObject* newOutput = 0;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (inputCD)
    {
    if (vtkMultiBlockDataSet::GetData(outInfo))
      {
      return 1;
      }
    newOutput = vtkMultiBlockDataSet::New();
    }
  else
    {
    if (vtkTable::GetData(outInfo))
      {
      return 1;
      }
    newOutput = vtkTable::New();
    }
  if (newOutput)
    {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    newOutput->Delete();
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkBlockDeliveryPreprocessor::RequestData(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  //cout << "vtkBlockDeliveryPreprocessor::CompositeDataSetIndex: "
  // << this->CompositeDataSetIndex << endl;
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  vtkSmartPointer<vtkDataObject> clone;
  clone.TakeReference(inputDO->NewInstance());
  clone->ShallowCopy(inputDO);

  vtkSmartPointer<vtkAttributeDataToTableFilter> adtf =
    vtkSmartPointer<vtkAttributeDataToTableFilter>::New();
  adtf->SetInputData(clone);
  adtf->SetAddMetaData(true);
  adtf->SetGenerateCellConnectivity(this->GenerateCellConnectivity);
  adtf->SetFieldAssociation(this->FieldAssociation);
  adtf->SetGenerateOriginalIds(this->GenerateOriginalIds);
  adtf->Update();

  // Create a pointer of the base class type, so that later stages need not be
  // concerned with whether the data was flattened or not.
  vtkAlgorithm* filter = adtf;

  vtkSmartPointer<vtkSplitColumnComponents> split;
  if (this->FlattenTable)
    {
    split = vtkSmartPointer<vtkSplitColumnComponents>::New();
    vtkCompositeDataPipeline *pipeline = vtkCompositeDataPipeline::New();
    split->SetExecutive(pipeline);
    pipeline->Delete();
    filter = split;
    split->SetInputConnection(adtf->GetOutputPort());
    split->Update();
    }

  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::SafeDownCast(
    outputDO);
  if (!output)
    {
    outputDO->ShallowCopy(filter->GetOutputDataObject(0));
    return 1;
    }

  if (this->CompositeDataSetIndex != 0)
    {
    vtkSmartPointer<vtkExtractBlock> eb = vtkSmartPointer<vtkExtractBlock>::New();
    eb->SetInputConnection(filter->GetOutputPort());
    eb->AddIndex(this->CompositeDataSetIndex);
    eb->PruneOutputOff();
    eb->Update();
    output->ShallowCopy(eb->GetOutput());
    }
  else
    {
    output->ShallowCopy(filter->GetOutputDataObject(0));
    }

  // Add meta-data about composite-index/hierarchical index to help
  // vtkSelectionStreamer.
  vtkCompositeDataIterator* iter = output->NewIterator();
  vtkUniformGridAMRDataIterator* ugIter =
    vtkUniformGridAMRDataIterator::SafeDownCast(iter);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    vtkInformation* metaData = iter->GetCurrentMetaData();
    if(metaData)
      {
      metaData->Set(
        vtkSelectionNode::COMPOSITE_INDEX(), iter->GetCurrentFlatIndex());
      if(ugIter)
        {
        metaData->Set(
          vtkSelectionNode::HIERARCHICAL_LEVEL(), ugIter->GetCurrentLevel());
        metaData->Set(
          vtkSelectionNode::HIERARCHICAL_INDEX(), ugIter->GetCurrentIndex());
        }
      }
    }
  iter->Delete();
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkBlockDeliveryPreprocessor::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void vtkBlockDeliveryPreprocessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
