vtk_module(vtkIOVisItBridge
  DEPENDS
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkFiltersAMR
    vtkParallelCore
  PRIVATE_DEPENDS
    VisItLib
    vtkhdf5
    vtknetcdf
    vtkzlib
  TEST_DEPENDS
  EXCLUDE_FROM_WRAP_HIERARCHY
)

# paraview-sepecfic extensions to a module to bring in proxy xmls.
set_property (GLOBAL PROPERTY
  vtkIOVisItBridge_SERVERMANAGER_XMLS
  ${CMAKE_CURRENT_LIST_DIR}/visit_readers.xml)
