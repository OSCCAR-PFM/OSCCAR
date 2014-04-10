/*=========================================================================

  Program:   ParaView
  Module:    vtkQuerySelectionSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuerySelectionSource.h"

#include "vtkObjectFactory.h"
#include "vtkSelectionNode.h"
#include "vtkSelection.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>
#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>

class vtkQuerySelectionSource::vtkInternals
{
};

vtkStandardNewMacro(vtkQuerySelectionSource);
//----------------------------------------------------------------------------
vtkQuerySelectionSource::vtkQuerySelectionSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Internals = new vtkInternals();

  this->FieldType = vtkSelectionNode::CELL;
  this->QueryString = 0;
  this->CompositeIndex = -1;
  this->HierarchicalIndex = -1;
  this->HierarchicalLevel = -1;
  this->ProcessID = -1;
  this->Inverse = 0;
}

//----------------------------------------------------------------------------
vtkQuerySelectionSource::~vtkQuerySelectionSource()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
int vtkQuerySelectionSource::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // We can handle multiple piece request.
  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkQuerySelectionSource::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  vtkSelection* output = vtkSelection::GetData(outputVector);
  vtkSelectionNode* selNode = vtkSelectionNode::New();
  output->AddNode(selNode);
  selNode->Delete();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = 0;
  if (outInfo->Has(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }

  if (this->ProcessID >= 0 && piece != this->ProcessID)
    {
    return 1;
    }

  vtkInformation* props = selNode->GetProperties();

  // Add qualifiers.
  if (this->CompositeIndex >= 0)
    {
    props->Set(vtkSelectionNode::COMPOSITE_INDEX(), this->CompositeIndex);
    }

  if (this->HierarchicalLevel >= 0)
    {
    props->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(), this->HierarchicalLevel);
    }

  if (this->HierarchicalIndex >= 0)
    {
    props->Set(vtkSelectionNode::HIERARCHICAL_INDEX(), this->HierarchicalIndex);
    }

  props->Set(vtkSelectionNode::FIELD_TYPE(), this->FieldType);
  props->Set(vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::QUERY);
  props->Set(vtkSelectionNode::INVERSE(), this->Inverse);

  selNode->SetQueryString(this->QueryString);

  return 1;
}

//----------------------------------------------------------------------------
const char* vtkQuerySelectionSource::GetUserFriendlyText()
{
  return this->GetQueryString();
}

//----------------------------------------------------------------------------
void vtkQuerySelectionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
