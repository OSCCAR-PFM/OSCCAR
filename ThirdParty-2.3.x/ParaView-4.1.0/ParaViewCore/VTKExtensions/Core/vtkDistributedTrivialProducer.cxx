/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrivialProducerStaticInternal.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistributedTrivialProducer.h"

#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVTrivialExtentTranslator.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>
#include <map>
#include <string>

//----------------------------------------------------------------------------
// Internal Static data structure
//----------------------------------------------------------------------------
struct vtkPVTrivialProducerStaticInternal
{
  std::map<std::string, vtkSmartPointer<vtkDataObject> > RegisteredDataObjectMap;

  bool HasKey(const char* key)
  {
    if(key == NULL)
      {
      return false;
      }
    return this->RegisteredDataObjectMap.find(key) != this->RegisteredDataObjectMap.end();
  }

  vtkDataObject* GetDataObject(const char* key)
  {
    if(this->HasKey(key))
      {
      return this->RegisteredDataObjectMap[key].GetPointer();
      }
    return NULL;
  }

  void Print(ostream& os, vtkIndent indent)
  {
    std::map<std::string, vtkSmartPointer<vtkDataObject> >::iterator iter = this->RegisteredDataObjectMap.begin();
    while(iter != this->RegisteredDataObjectMap.end())
    {
      os << indent << iter->first.c_str() << "\n";
      iter->second.GetPointer()->PrintSelf(os, indent.GetNextIndent());
    }
  }
};
static vtkPVTrivialProducerStaticInternal Value;
vtkPVTrivialProducerStaticInternal* vtkDistributedTrivialProducer::InternalStatic = &Value;
//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkDistributedTrivialProducer);
//----------------------------------------------------------------------------
vtkDistributedTrivialProducer::vtkDistributedTrivialProducer()
{
}

//----------------------------------------------------------------------------
vtkDistributedTrivialProducer::~vtkDistributedTrivialProducer()
{
}
//----------------------------------------------------------------------------
void vtkDistributedTrivialProducer::SetGlobalOutput(const char* key, vtkDataObject* output)
{
  if(key)
    {
    vtkDistributedTrivialProducer::InternalStatic->RegisteredDataObjectMap[key] = output;
    cout << "Set Global Dataset for " << key << endl;
    }
}

//----------------------------------------------------------------------------
void vtkDistributedTrivialProducer::ReleaseGlobalOutput(const char* key)
{
  if(key)
    {
    vtkDistributedTrivialProducer::InternalStatic->RegisteredDataObjectMap.erase(key);
    }
  else
    {
    vtkDistributedTrivialProducer::InternalStatic->RegisteredDataObjectMap.clear();
    }
}

//----------------------------------------------------------------------------
void vtkDistributedTrivialProducer::UpdateFromGlobal(const char* key)
{
  cout << "Update DS with key " << key << endl;
  if(vtkDistributedTrivialProducer::InternalStatic->GetDataObject(key))
    {
    //vtkDistributedTrivialProducer::InternalStatic->GetDataObject(key)->PrintSelf(cout, vtkIndent(5));
    }
  else
    {
    cout << "No dataset" << endl;
    }
  this->SetOutput(vtkDistributedTrivialProducer::InternalStatic->GetDataObject(key));
}

//----------------------------------------------------------------------------
void vtkDistributedTrivialProducer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkDistributedTrivialProducer::InternalStatic->Print(os, indent);
}
