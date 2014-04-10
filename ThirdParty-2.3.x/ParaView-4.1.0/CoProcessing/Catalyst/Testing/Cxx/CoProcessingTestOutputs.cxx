#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPPipeline.h>
#include <vtkCPProcessor.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

namespace
{
  class vtkCPTestPipeline : public vtkCPPipeline
  {
  public:
    vtkTypeMacro(vtkCPTestPipeline,vtkCPPipeline);
    static vtkCPTestPipeline* New();

    virtual int RequestDataDescription(vtkCPDataDescription* dataDescription)
    {
      return this->ShouldOutput(dataDescription, false);
    }

    /// Execute the pipeline. Returns 1 for success and 0 for failure.
    virtual int CoProcess(vtkCPDataDescription* dataDescription)
    {
      this->OutputCounter++;
      if(this->ShouldOutput(dataDescription, true) == 0)
        {
        vtkErrorMacro("Calling CoProcess but shouldn't be.");
        return 0;
        }
      return 1;
    }

    void SetParameters(int firstInputFrequency, int secondInputFrequency,
                       int expectedOutputCounter)
    {
      this->FirstInputFrequency = firstInputFrequency;
      this->SecondInputFrequency = secondInputFrequency;
      this->ExpectedOutputCounter = expectedOutputCounter;
    }

  protected:
    vtkCPTestPipeline()
    {
      this->OutputCounter = 0;
      this->FirstInputFrequency = -1;
      this->SecondInputFrequency = -1;
      this->ExpectedOutputCounter = -1;
    }
    virtual ~vtkCPTestPipeline()
    {
      // check that we've outputted the proper amount of times
      if(this->OutputCounter != this->ExpectedOutputCounter)
        {
        cerr << "CoProcessingTestOutputs.cxx: number of outputs was "
             << this->OutputCounter << " but the number should have been "
             << this->ExpectedOutputCounter << endl;
        throw 1;
        }
    }
    // We have two input data sets and we have an frequency for
    // each one of them.
    int FirstInputFrequency;
    int SecondInputFrequency;
    // We keep track of how many times we've outputted and compare that to
    // the expected number of times to verify things are working properly.
    int ExpectedOutputCounter;
    int OutputCounter;

    // This method determines if we should output or not. For
    // RequestDataDescription() we don't check whether the output is
    // forced or not because that should have been checked already.
    // For CoProcess we do also use that for the check.
    int ShouldOutput(vtkCPDataDescription* dataDescription,
                     bool checkForceOutput)
    {
      if(checkForceOutput && dataDescription->GetForceOutput())
        {
        return 1;
        }
      vtkIdType timeStep = dataDescription->GetTimeStep();
      int retVal = 0;
      if(timeStep%this->FirstInputFrequency==0)
        {
        vtkCPInputDataDescription* inputDescription =
          dataDescription->GetInputDescriptionByName("firstinput");
        inputDescription->AllFieldsOn();
        inputDescription->GenerateMeshOn();
        retVal = 1;
        }
      if(timeStep%this->SecondInputFrequency==0)
        {
        vtkCPInputDataDescription* inputDescription =
          dataDescription->GetInputDescriptionByName("secondinput");
        inputDescription->AllFieldsOn();
        inputDescription->GenerateMeshOn();
        retVal = 1;
        }
      return retVal;
    }

  private:
    vtkCPTestPipeline(const vtkCPTestPipeline&); // Not implemented
    void operator=(const vtkCPTestPipeline&); // Not implemented
  };

  vtkStandardNewMacro(vtkCPTestPipeline);
}

int main()
{
  vtkSmartPointer<vtkCPProcessor> processor =
    vtkSmartPointer<vtkCPProcessor>::New();
  processor->Initialize();

  vtkSmartPointer<vtkCPTestPipeline> pipeline1 =
    vtkSmartPointer<vtkCPTestPipeline>::New();
  processor->AddPipeline(pipeline1);
  pipeline1->SetParameters(2, 3, 21);

  vtkSmartPointer<vtkCPTestPipeline> pipeline2 =
    vtkSmartPointer<vtkCPTestPipeline>::New();
  processor->AddPipeline(pipeline2);
  pipeline2->SetParameters(5, 7, 12);

  if(processor->GetNumberOfPipelines() != 2)
    {
    vtkGenericWarningMacro("Wrong number of pipelines");
    return 1;
    }

  vtkIdType numberOfTimeSteps = 30;
  for(vtkIdType timeStep=0;timeStep<numberOfTimeSteps;timeStep++)
    {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    vtkSmartPointer<vtkCPDataDescription> dataDescription =
      vtkSmartPointer<vtkCPDataDescription>::New();
    dataDescription->AddInput("firstinput");
    dataDescription->AddInput("secondinput");
    dataDescription->SetTimeData(time, timeStep);
    if(timeStep%11==0)
      {
      dataDescription->ForceOutputOn();
      }
    if(processor->RequestDataDescription(dataDescription))
      {
      if(processor->CoProcess(dataDescription) == 0)
        {
        vtkGenericWarningMacro("Co-processing problem.");
        return 1;
        }
      }
    }

  processor->Finalize();

  return 0;
}
