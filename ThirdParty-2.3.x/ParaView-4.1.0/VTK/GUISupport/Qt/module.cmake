vtk_module(vtkGUISupportQt
  GROUPS
    Qt
  DEPENDS
    vtkCommonExecutionModel
    vtkRenderingOpenGL
    vtkInteractionStyle
    vtkImagingCore
  PRIVATE_DEPENDS
    vtkFiltersExtraction
  TEST_DEPENDS
    vtkTestingCore
  EXCLUDE_FROM_WRAPPING
  )