#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPVDataEncoder.h"
#include "vtkTestUtilities.h"
#include "vtkSmartPointer.h"
#include "vtkImageData.h"

#include <vtksys/SystemTools.hxx>

#include <string>
#include <vector>

// returns 0 on success.
int TestDataEncoder(int argc, char* argv[])
{
  char* dataroot = vtkTestUtilities::GetDataRoot(argc, argv);
  if (!dataroot)
    {
    vtkGenericWarningMacro("Missing Data Root!");
    return 1;
    }

  // Open image files from the PARAVIEW_DATA_ROOT/Baseline and try to
  // encode/compress them and then verify that they are indeed the same.
  const char* filenames[] = {
    "/Baseline/Clip.png",
    "/Baseline/Contour.png",
    "/Baseline/ContourRange.png",
    "/Baseline/EnSight.png",
    "/Baseline/ExtractBlock.png",
    "/Baseline/ExtractGrid.png",
    NULL
  };

  std::vector<vtkSmartPointer<vtkImageData> > images;
  for (int cc=0; filenames[cc] != NULL; cc++)
    {
    vtkNew<vtkPNGReader> reader;
    std::string filename(dataroot);
    filename += filenames[cc];
    reader->SetFileName(filename.c_str());
    reader->Update();
    images.push_back(reader->GetOutput());
    }

  int errors = 0;
  vtkNew<vtkPVDataEncoder> encoder;
  for (size_t cc=0; cc < images.size(); cc++)
    {
    vtkImageData* img = images[cc].GetPointer();
    img->Register(NULL);
    images[cc] = NULL;
    encoder->PushAndTakeReference(cc % 3, img, 100);
    vtksys::SystemTools::Delay(10); // 0.2s
    }

  vtkSmartPointer<vtkUnsignedCharArray> data;
  encoder->GetLatestOutput(0, data);
  encoder->GetLatestOutput(1, data);
  encoder->GetLatestOutput(2, data);
  encoder->GetLatestOutput(3, data);
  encoder->Flush(0);
  delete [] dataroot;
  return errors;
}
