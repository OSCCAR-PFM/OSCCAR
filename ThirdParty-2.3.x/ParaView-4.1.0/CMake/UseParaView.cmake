# This file sets up include directories, link directories, and
# compiler settings for a project to use ParaView.  It should not be
# included directly, but rather through the ParaView_USE_FILE setting
# obtained from ParaViewConfig.cmake.

if(PARAVIEW_USE_FILE_INCLUDED)
  return()
endif()
set(PARAVIEW_USE_FILE_INCLUDED 1)

# Update CMAKE_MODULE_PATH so includes work.
list(APPEND CMAKE_MODULE_PATH ${ParaView_CMAKE_DIR})
include("${VTK_USE_FILE}")

# VTK_USE_FILE adds definitions for ${module}_AUTOINIT for all enabled modules.
# This is okay for VTK, with ParaView, AUTOINIT is not useful since one needs to
# init the CS streams separately. Also use AUTOINIT is defined, any
# application needs to link against all enabled VTK modules which ends up being
# a very long list and hard to keep up-to-date. We over come this issue by
# simply not setting the ${module}_AUTOINIT definies.
# So we get the current COMPILE_DEFINITIONS on the directory and remove
# references to AUTOINIT.
get_property(cur_compile_definitions DIRECTORY PROPERTY COMPILE_DEFINITIONS)
set (new_compile_definition)
foreach (defn IN LISTS cur_compile_definitions)
  string(REGEX MATCH "_AUTOINIT=" out-var "${defn}")
  if (NOT out-var)
    list(APPEND new_compile_definition "${defn}")
  endif()
endforeach()
set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS ${new_compile_definition})

if (PARAVIEW_ENABLE_QT_SUPPORT)
  set(QT_QMAKE_EXECUTABLE ${PARAVIEW_QT_QMAKE_EXECUTABLE})
  find_package(Qt4)
  if (QT4_FOUND)
    include("${QT_USE_FILE}")
  endif()
endif()

# Import some commonly used cmake modules
include (ParaViewMacros)
include (ParaViewPlugins)
include (ParaViewBranding)

# FIXME: there was additional stuff about coprocessing and visit bridge here.
