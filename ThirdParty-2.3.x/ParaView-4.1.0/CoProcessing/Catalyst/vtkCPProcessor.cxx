/*=========================================================================

  Program:   ParaView
  Module:    vtkCPProcessor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPProcessor.h"

#include "vtkCPCxxHelper.h"
#include "vtkCPDataDescription.h"
#include "vtkCPPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#include <list>

struct vtkCPProcessorInternals
{
  typedef std::list<vtkSmartPointer<vtkCPPipeline> > PipelineList;
  typedef PipelineList::iterator PipelineListIterator;
  PipelineList Pipelines;
};


vtkStandardNewMacro(vtkCPProcessor);

//----------------------------------------------------------------------------
vtkCPProcessor::vtkCPProcessor()
{
  this->Internal = new vtkCPProcessorInternals;
  this->InitializationHelper = NULL;
}

//----------------------------------------------------------------------------
vtkCPProcessor::~vtkCPProcessor()
{
  if(this->Internal)
    {
    delete this->Internal;
    this->Internal = NULL;
    }

  if(this->InitializationHelper)
    {
    this->InitializationHelper->Delete();
    this->InitializationHelper = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkCPProcessor::AddPipeline(vtkCPPipeline* pipeline)
{
  if(!pipeline)
    {
    vtkErrorMacro("Pipeline is NULL.");
    return 0;
    }

  this->Internal->Pipelines.push_back(pipeline);
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::GetNumberOfPipelines()
{
  return static_cast<int>(this->Internal->Pipelines.size());
}

//----------------------------------------------------------------------------
vtkCPPipeline* vtkCPProcessor::GetPipeline(int which)
{
  if(which < 0 || which >= this->GetNumberOfPipelines())
    {
    return NULL;
    }
  int counter=0;
  vtkCPProcessorInternals::PipelineListIterator iter =
    this->Internal->Pipelines.begin();
  while(counter <= which)
    {
    if(counter == which)
      {
      return *iter;
      }
    counter++;
    iter++;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkCPProcessor::RemovePipeline(vtkCPPipeline* pipeline)
{
  this->Internal->Pipelines.remove(pipeline);
}

//----------------------------------------------------------------------------
void vtkCPProcessor::RemoveAllPipelines()
{
  this->Internal->Pipelines.clear();
}

//----------------------------------------------------------------------------
vtkObject* vtkCPProcessor::NewInitializationHelper()
{
  return vtkCPCxxHelper::New();
}

//----------------------------------------------------------------------------
int vtkCPProcessor::Initialize()
{
  if (this->InitializationHelper == NULL)
    {
    this->InitializationHelper = this->NewInitializationHelper();

    // turn on immediate mode rendering. this helps avoid memory
    // fragmentation which can kill a run on memory constrained machines.
    vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
    vtkSMSessionProxyManager* sessionProxyManager =
      proxyManager->GetActiveSessionProxyManager();
    // Catalyst configurations may not have rendering enable and thus
    // won't have GlobalMapperProperties.
    if(sessionProxyManager->HasDefinition("misc", "GlobalMapperProperties"))
      {
      vtkSmartPointer<vtkSMProxy> globalMapperProperties;
      globalMapperProperties.TakeReference(
        sessionProxyManager->NewProxy("misc", "GlobalMapperProperties"));
      vtkSMIntVectorProperty* immediateModeRendering =
        vtkSMIntVectorProperty::SafeDownCast(
          globalMapperProperties->GetProperty("GlobalImmediateModeRendering"));
      immediateModeRendering->SetElements1(1);
      globalMapperProperties->UpdateVTKObjects();
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::RequestDataDescription(
  vtkCPDataDescription* dataDescription)
{
  if(!dataDescription)
    {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
    }
  if(dataDescription->GetForceOutput() == true)
    {
    return 1;
    }

  dataDescription->ResetInputDescriptions();
  int doCoProcessing = 0;
  for(vtkCPProcessorInternals::PipelineListIterator iter =
        this->Internal->Pipelines.begin();
      iter!=this->Internal->Pipelines.end();iter++)
    {
    if(iter->GetPointer()->RequestDataDescription(dataDescription))
      {
      doCoProcessing = 1;
      }
    }
  return doCoProcessing;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::CoProcess(vtkCPDataDescription* dataDescription)
{
  if(!dataDescription)
    {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
    }
  int success = 1;
  for(vtkCPProcessorInternals::PipelineListIterator iter =
        this->Internal->Pipelines.begin();
      iter!=this->Internal->Pipelines.end();iter++)
    {
    if(dataDescription->GetForceOutput() == true ||
       iter->GetPointer()->RequestDataDescription(dataDescription))
      {
      if(!iter->GetPointer()->CoProcess(dataDescription))
        {
        success = 0;
        }
      }
    }
  // we want to reset everything here to make sure that new information
  // is properly passed in the next time.
  dataDescription->ResetAll();
  return success;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::Finalize()
{
  this->RemoveAllPipelines();
  return 1;
}

//----------------------------------------------------------------------------
void vtkCPProcessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
