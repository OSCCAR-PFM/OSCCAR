# Specific simulation adaptors are typically built separately from the ParaView
# build. The ParaView build will generally including the vtkPVCatalyst library,
# which is all anyone needs to write adaptors. However, to keep all
# open-adaptors in one place, we place them under this directory and build them
# using a pattern similar to the ParaView/Examples so that each one of the
# adaptors can be built separately if the user wants.
if (NOT PARAVIEW_ENABLE_CATALYST)
  # sanity check.
  return()
endif()

#------------------------------------------------------------------------------
# Make sure it uses the same build configuration as ParaView.
if (CMAKE_CONFIGURATION_TYPES)
  set(build_config_arg -C "${CMAKE_CFG_INTDIR}")
else()
  set(build_config_arg)
endif()

set (extra_params)
foreach (flag CMAKE_C_FLAGS_DEBUG
              CMAKE_C_FLAGS_RELEASE
              CMAKE_C_FLAGS_MINSIZEREL
              CMAKE_C_FLAGS_RELWITHDEBINFO
              CMAKE_CXX_FLAGS_DEBUG
              CMAKE_CXX_FLAGS_RELEASE
              CMAKE_CXX_FLAGS_MINSIZEREL
              CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  if (${${flag}})
    set (extra_params ${extra_params}
        -D${flag}:STRING=${${flag}})
  endif()
endforeach()

#------------------------------------------------------------------------------
set (SOURCE_DIR "${ParaView_SOURCE_DIR}/CoProcessing/Adaptors")
set (BINARY_DIR "${ParaView_BINARY_DIR}/CoProcessing/Adaptors")
make_directory("${BINARY_DIR}")

#------------------------------------------------------------------------------
# Function to easy adding separate custom-commands to build the adaptors.
#------------------------------------------------------------------------------
function(build_adaptor name fortran_options)
  string(TOLOWER "${name}" lname)
  add_custom_command(
    OUTPUT "${BINARY_DIR}/${lname}.done"
    COMMAND ${CMAKE_CTEST_COMMAND}
            ${build_config_arg}
            --build-and-test ${SOURCE_DIR}/${name}
                             ${BINARY_DIR}/${name}
            --build-noclean
            --build-two-config
            --build-project ${name}
            --build-generator ${CMAKE_GENERATOR}
            --build-makeprogram ${CMAKE_MAKE_PROGRAM}
            --build-options -DParaView_DIR:PATH=${ParaView_BINARY_DIR}
                            -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                            -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                            -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
                            -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                            -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
                            -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                            -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                            ${fortran_options}
                            ${extra_params}
    COMMAND ${CMAKE_COMMAND}
            -E touch "${BINARY_DIR}/${lname}.done"

    ${ARGN}
  )
  add_custom_target(${name} ALL DEPENDS "${BINARY_DIR}/${lname}.done")
endfunction()


#------------------------------------------------------------------------------
# Adaptors
#------------------------------------------------------------------------------
build_adaptor(NPICAdaptor
              COMMENT "Building NPIC Adaptor"
              DEPENDS vtkPVCatalyst)

if (PARAVIEW_USE_MPI)
    build_adaptor(ParticleAdaptor
                  COMMENT "Building Particle Adaptor"
                  DEPENDS vtkPVCatalyst)
endif()

#------------------------------------------------------------------------------
# Adaptors that need Fortran -- we disable them by default because not all
# systems load all of the proper Fortran dependencies like MPI_Fortran_LIBRARIES
#------------------------------------------------------------------------------
if (CMAKE_Fortran_COMPILER_WORKS)
  cmake_dependent_option(BUILD_PHASTA_ADAPTOR
    "Build the Phasta Catalyst Adaptor" OFF
    "PARAVIEW_BUILD_CATALYST_ADAPTORS" OFF)
  mark_as_advanced(BUILD_PHASTA_ADAPTOR)
  if(BUILD_PHASTA_ADAPTOR)
    build_adaptor(PhastaAdaptor
      "-DCMAKE_Fortran_COMPILER:FILEPATH=${CMAKE_Fortran_COMPILER}"
      COMMENT "Building Phasta Adaptor"
      DEPENDS vtkPVCatalyst)
  endif()
endif()

#------------------------------------------------------------------------------
# Adaptors that need Python
#------------------------------------------------------------------------------
if (PARAVIEW_ENABLE_PYTHON AND NOT WIN32)
  # Add CTHAdaptor if Python is enabled.
  build_adaptor(CTHAdaptor
                COMMENT "Building CTH Adaptor"
                DEPENDS vtkPVPythonCatalyst)
endif()
