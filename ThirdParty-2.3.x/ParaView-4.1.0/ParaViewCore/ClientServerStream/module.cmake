set(_dependencies)
if (PARAVIEW_ENABLE_PYTHON)
  set(_dependencies
    vtkPython
    vtkWrappingPythonCore
    vtkPythonInterpreter)
endif ()

vtk_module(vtkClientServer
  DEPENDS
    vtkCommonCore
    ${_dependencies}
  TEST_DEPENDS
    vtkCommonCore
  EXCLUDE_FROM_WRAPPING
  TEST_LABELS
    PARAVIEW
)
