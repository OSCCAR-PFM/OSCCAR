set(Kit_SRCS
vtkAvtFileFormatAlgorithm.h
vtkAvtFileFormatAlgorithm.cxx
vtkAvtMTSDFileFormatAlgorithm.h
vtkAvtMTSDFileFormatAlgorithm.cxx
vtkAvtMTMDFileFormatAlgorithm.h
vtkAvtMTMDFileFormatAlgorithm.cxx
vtkAvtSTSDFileFormatAlgorithm.h
vtkAvtSTSDFileFormatAlgorithm.cxx
vtkAvtSTMDFileFormatAlgorithm.h
vtkAvtSTMDFileFormatAlgorithm.cxx
)

set(FullPath_SRCS)
foreach(fname ${Kit_SRCS})
  list(APPEND FullPath_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${fname}")
endforeach()

set(VISIT_SERVER_SOURCES ${FullPath_SRCS}
    CACHE INTERNAL "vtk classes to wrap for client server")
