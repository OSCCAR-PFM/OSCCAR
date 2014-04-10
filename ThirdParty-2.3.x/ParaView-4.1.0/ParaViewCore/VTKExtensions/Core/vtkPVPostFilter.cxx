/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVPostFilter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPostFilter.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataObjectTypes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPVPostFilterExecutive.h"

#include <vtksys/SystemTools.hxx>
#include <string>
#include <assert.h>
#include <set>
#include <sstream>

namespace
{
  // Demangles a mangled string containing an array name and a component name.
  void DeMangleArrayName(const std::string &mangledName,
                         vtkDataSet *dataSet,
                         std::string &demangledName,
                         std::string &demangledComponentName)
    {
    std::vector<vtkDataSetAttributes *> attributesArray;
    attributesArray.push_back(dataSet->GetCellData());
    attributesArray.push_back(dataSet->GetPointData());

    for(size_t index = 0; index < attributesArray.size(); index++)
      {
      vtkDataSetAttributes *dataSetAttributes = attributesArray[index];
      if(!dataSetAttributes)
        {
        continue;
        }

      for(int arrayIndex = 0; arrayIndex < dataSetAttributes->GetNumberOfArrays(); arrayIndex++)
        {
        // check for matching array name at the start of the mangled name
        const char *arrayName = dataSetAttributes->GetArrayName(arrayIndex);
        size_t arrayNameLength = strlen(arrayName);
        if(strncmp(mangledName.c_str(), arrayName, arrayNameLength) == 0)
          {
          if(mangledName.size() == arrayNameLength)
            {
            // the mangled name is just the array name
            demangledName = mangledName;
            demangledComponentName = std::string();
            return;
            }
          else if(mangledName.size() > arrayNameLength + 1)
            {
            vtkAbstractArray *array = dataSetAttributes->GetAbstractArray(arrayIndex);
            int componentCount = array->GetNumberOfComponents();

            // check the for a matching component name
            //has to be from -1 as -1 represents the Magnitude component
            for(int componentIndex = -1; componentIndex < componentCount; componentIndex++)
              {
              vtkStdString componentNameString;
              const char *componentName = array->GetComponentName(componentIndex);
              if(componentName)
                {
                componentNameString = componentName;
                }
              else
                {
                // use the default component name if the component has no name set
                componentNameString = vtkPVPostFilter::DefaultComponentName(componentIndex, componentCount);
                }

              if(componentNameString.empty())
                {
                continue;
                }

              // check component name from the end of array name string after the underscore
              const char *mangledComponentName = &mangledName[arrayNameLength+1];
              if(componentNameString == mangledComponentName)
                {
                // found a match
                demangledName = arrayName;
                demangledComponentName = mangledComponentName;
                return;
                }
              }
            }
          }
        }
      }

    // return original name
    demangledName = mangledName;
    demangledComponentName = std::string();
    }
}

vtkStandardNewMacro(vtkPVPostFilter);
//----------------------------------------------------------------------------
vtkPVPostFilter::vtkPVPostFilter()
{
  vtkPVPostFilterExecutive* exec = vtkPVPostFilterExecutive::New();
  this->SetExecutive(exec);
  exec->FastDelete();

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_DATASET(), 1);
}

//----------------------------------------------------------------------------
vtkPVPostFilter::~vtkPVPostFilter()
{
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVPostFilter::CreateDefaultExecutive()
{
  return vtkPVPostFilterExecutive::New();
}

//----------------------------------------------------------------------------
vtkStdString vtkPVPostFilter::DefaultComponentName(int componentNumber, int componentCount)
{
  if (componentCount <= 1)
    {
    return "";
    }
  else if (componentNumber == -1)
    {
    return "Magnitude";
    }
  else if (componentCount <= 3 && componentNumber < 3)
    {
    const char* titles[] = {"X", "Y", "Z"};
    return titles[componentNumber];
    }
  else if (componentCount == 6)
    {
    const char* titles[] = {"XX", "YY", "ZZ", "XY", "YZ", "XZ"};
    // Assume this is a symmetric matrix.
    return titles[componentNumber];
    }
  else
    {
    std::ostringstream buffer;
    buffer << componentNumber;
    return buffer.str();
    }
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // We want to exclude vtkTemporalDataSet from being accepted as an input,
  // everything else is acceptable.

  std::string currentDataObject;

  const std::string invalid("UnknownClass");

  std::set<std::string> exemptClasses;

  //too properly exclude datasets you need to exclude all abstract
  //classes so that only thing left in the valid input list
  //is concrete implementations. This listing is of all abstract data objects
  exemptClasses.insert("vtkDataObject");
  exemptClasses.insert("vtkCompositeDataSet");
  exemptClasses.insert("vtkDataSet");
  exemptClasses.insert("vtkGraph");

  //now exclude concrete classes, that we don't want the post
  //filter to work on
  exemptClasses.insert("vtkTemporalDataSet");

  int i=0;
  while(currentDataObject != invalid)
    {
    currentDataObject = vtkDataObjectTypes::GetClassNameFromTypeId(i++);
    if (exemptClasses.count(currentDataObject)==0)
      {
      //if the set doesn't contain this dataobject
      //it is a failed input type
      vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()->Append(
            info,currentDataObject.c_str());
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
      if (!output || !output->IsA(input->GetClassName()))
        {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
        }
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestData(vtkInformation *,
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{

  //we need to just copy the data, so we can fixup the output as needed
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input= inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output= outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (output && input)
      {
      vtkCompositeDataSet *csInput = vtkCompositeDataSet::SafeDownCast(input);
      vtkCompositeDataSet *csOutput = vtkCompositeDataSet::SafeDownCast(output);
      if (!csInput && !csOutput)
        {
        //vtkDataSet
        output->ShallowCopy(input);
        }
      else
        {
        csOutput->CopyStructure(csInput);
        vtkCompositeDataIterator* iter = csInput->NewIterator();
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
          {
          vtkDataObject* obj = iter->GetCurrentDataObject()->NewInstance();
          obj->ShallowCopy(iter->GetCurrentDataObject());
          csOutput->SetDataSet(iter,obj);
          obj->FastDelete();
          }
        iter->Delete();
        }
      if (this->Information->Has(vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS()) )
        {
        this->DoAnyNeededConversions(output);
        }
      }
    return 1;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::DoAnyNeededConversions(vtkDataObject* output)
{
  //get the array to convert info
  vtkInformationVector* postVector =
    this->Information->Get(vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS());
  vtkInformation *postArrayInfo = postVector->GetInformationObject(0);

  const char* name = postArrayInfo->Get(vtkDataObject::FIELD_NAME());
  int fieldAssociation = postArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());

  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(output);
  if (cd)
    {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
      iter->GoToNextItem())
      {
      vtkDataSet* dataset = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (dataset)
        {
        std::string demangled_name, demagled_component_name;
        DeMangleArrayName(name, dataset, demangled_name, demagled_component_name);

        this->DoAnyNeededConversions(dataset, name, fieldAssociation,
          demangled_name.c_str(), demagled_component_name.c_str());
        }
      }
    iter->Delete();
    return 1;
    }
  else
    {
    vtkDataSet* dataset = vtkDataSet::SafeDownCast(output);
    if (dataset)
      {
      std::string demangled_name, demagled_component_name;
      DeMangleArrayName(name, dataset, demangled_name, demagled_component_name);

      return this->DoAnyNeededConversions(dataset,
                                          name,
                                          fieldAssociation,
                                          demangled_name.c_str(),
                                          demagled_component_name.c_str());
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::DoAnyNeededConversions(vtkDataSet* output,
  const char* requested_name, int fieldAssociation,
  const char* demangled_name, const char* demagled_component_name)
{
  vtkDataSetAttributes* dsa = NULL;
  vtkDataSetAttributes* pointData = output->GetPointData();
  vtkDataSetAttributes* cellData = output->GetCellData();

  switch (fieldAssociation)
    {
  case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    dsa = pointData;
    break;

  case vtkDataObject::FIELD_ASSOCIATION_CELLS:
    dsa = cellData;
    break;

  case vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
    vtkWarningMacro("Case not handled");

  default:
    return 0;
    }

  if (dsa->GetAbstractArray(requested_name))
    {
    // requested array is present. Don't bother doing anything.
    return 0;
    }

  if (dsa->GetAbstractArray(demangled_name))
    {
    // demangled_name is present, implies that component extraction is needed.
    return this->ExtractComponent(dsa, requested_name,
      demangled_name, demagled_component_name);
    }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    if (cellData->GetAbstractArray(requested_name) ||
      cellData->GetAbstractArray(demangled_name))
      {
      this->CellDataToPointData(output);
      }
    }
  else if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    if (pointData->GetAbstractArray(requested_name) ||
      pointData->GetAbstractArray(demangled_name))
      {
      this->PointDataToCellData(output);
      }
    }

  if (dsa->GetAbstractArray(requested_name))
    {
    // after the conversion the requested array is present. Don't bother doing
    // anything more.
    return 1;
    }

  if (dsa->GetAbstractArray(demangled_name))
    {
    // demangled_name is present, implies that component extraction is needed.
    return this->ExtractComponent(dsa, requested_name,
      demangled_name, demagled_component_name);
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVPostFilter::CellDataToPointData(vtkDataSet* output)
{
  vtkDataObject* clone = output->NewInstance();
  clone->ShallowCopy(output);

  vtkCellDataToPointData *converter = vtkCellDataToPointData::New();
  converter->SetInputData(clone);
  converter->PassCellDataOn();
  converter->Update();
  output->ShallowCopy(converter->GetOutputDataObject(0));
  converter->Delete();
  clone->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPostFilter::PointDataToCellData(vtkDataSet* output)
{
  vtkDataObject* clone = output->NewInstance();
  clone->ShallowCopy(output);

  vtkPointDataToCellData *converter = vtkPointDataToCellData::New();
  converter->SetInputData(clone);
  converter->PassPointDataOn();
  converter->Update();
  output->ShallowCopy(converter->GetOutputDataObject(0));
  converter->Delete();
  clone->Delete();
}

//----------------------------------------------------------------------------
namespace
{
  template <class T, class U>
  void CopyComponent(T* outIter, U* inIter, int compNo)
    {
    vtkDataArray* inDa = vtkDataArray::SafeDownCast(inIter->GetArray());
    vtkIdType numTuples = inIter->GetNumberOfTuples();

    if (compNo == -1 && inDa == NULL)
      {
      compNo = 0;
      }

    if (compNo == -1)
      {
      vtkDataArray* outDa = vtkDataArray::SafeDownCast(outIter->GetArray());
      int numcomps = inIter->GetNumberOfComponents();
      for (vtkIdType cc=0; cc < numTuples; cc++)
        {
        double mag=0.0;
        double* tuple = inDa->GetTuple(cc);
        for (int comp=0; comp < numcomps; comp++)
          {
          mag += tuple[comp]*tuple[comp];
          }
        outDa->SetTuple1(cc, sqrt(mag));
        }
      }
    else
      {
      for (vtkIdType cc=0; cc < numTuples; cc++)
        {
        outIter->SetValue(cc, inIter->GetTuple(cc)[compNo]);
        }
      }
    }

  template <>
  void CopyComponent(vtkArrayIteratorTemplate<double>* /*outIter*/,
                     vtkArrayIteratorTemplate<vtkStdString>* /*inIter*/,
                     int /*compNo*/)
    {
    //Because the vtkTemplateMacro is coded to attempt to call
    //the function with each use case, we have to handle the string to double
    //conversion, which in reality should never happen. So for know we
    //are leaving it empty
    }
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::ExtractComponent(vtkDataSetAttributes* dsa,
  const char* requested_name, const char* demangled_name,
  const char* demangled_component_name)
{
  vtkAbstractArray* array = dsa->GetAbstractArray(demangled_name);
  assert(array != NULL && demangled_name && demangled_component_name);

  int cIndex = -1;
  bool found = false;
  // demagled_component_name can be a real component name OR
  // X,Y,Z for the first 3 components OR
  // 0,...N i.e. an integer for the index OR
  // Magnitude to indicate vector magnitude.
  // Now to the trick is to decide what way this particular request has been
  // made.

  //First pass is to match the demangled name to a component name
  //Component names take highest priority so if somebody named component 4 to be "1"
  //that should match before resorting to using atoi
  for (int cc=0; cc < array->GetNumberOfComponents() && array->HasAComponentName(); cc++)
    {
    const char* comp_name = array->GetComponentName(cc);
    if (comp_name && strcmp(comp_name, demangled_component_name) == 0)
      {
      cIndex = cc;
      found = true;
      break;
      }
    }

  //if we still haven't found a match we will check the component agianst the
  //the default names.
  int numComps = array->GetNumberOfComponents();
  //compare agianst cIndex to only run this if component names didn't match
  for(int i=-1; i < numComps && cIndex == -1;i++)
    {
    vtkStdString defaultName = vtkPVPostFilter::DefaultComponentName(i,numComps);
    if(vtksys::SystemTools::Strucmp(
         defaultName.c_str(),demangled_component_name) == 0)
      {
      cIndex = i;
      found = true;
      break;
      }
    }

  //None of the component names or default names matched so we
  //go onto doing a pure conversion of the string to integer and using that.
  if(!found)
    {
    cIndex = atoi(demangled_component_name);
    }

  //when we compute the magnitude we must place
  //the result in a double array, since we don't the size of the
  //resulting data.
  bool isMagnitude = (cIndex == -1);
  vtkAbstractArray* newArray = isMagnitude ? vtkDoubleArray::New() :
                                                array->NewInstance();

  newArray->SetNumberOfComponents(1);
  newArray->SetNumberOfTuples(array->GetNumberOfTuples());
  newArray->SetName(requested_name);

  vtkArrayIterator* inIter = array->NewIterator();
  vtkArrayIterator* outIter = newArray->NewIterator();


  if(isMagnitude)
    {
    switch (array->GetDataType())
      {
      vtkArrayIteratorTemplateMacro(
        ::CopyComponent(static_cast< vtkArrayIteratorTemplate<double>* >(outIter),
          static_cast<VTK_TT*>(inIter), cIndex);
      );
      }
    }
  else
    {
    switch (array->GetDataType())
      {
      vtkArrayIteratorTemplateMacro(
        ::CopyComponent(static_cast<VTK_TT*>(outIter),
          static_cast<VTK_TT*>(inIter), cIndex);
      );
      }
    }

  inIter->Delete();
  outIter->Delete();
  dsa->AddArray(newArray);
  newArray->FastDelete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPostFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
