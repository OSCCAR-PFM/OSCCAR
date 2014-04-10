vtk_module(vtkPVServerManagerCore
  GROUPS
    ParaViewCore
  DEPENDS
    # Explicitely list (rather than transiently through
    # vtkPVServerImplementationCore) because it allows us to turn of wrapping
    # of vtkPVServerImplementationCore off but still satisfy API dependcy.
    vtkCommonCore
    vtkPVServerImplementationCore
  PRIVATE_DEPENDS
    vtksys
  TEST_LABELS
    PARAVIEW
)
