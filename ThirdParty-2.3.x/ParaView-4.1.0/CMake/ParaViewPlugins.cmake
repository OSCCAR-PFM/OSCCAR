include(ParaViewMacros)

# Requires ParaView_QT_DIR and ParaView_BINARY_DIR to be set.

# Macro to install a plugin that's included in the ParaView source directory.
# This is a macro internal to ParaView and should not be directly used by
# external applications. This may change in future without notice.
MACRO(internal_paraview_install_plugin name)
  IF (PV_INSTALL_PLUGIN_DIR)
    INSTALL(TARGETS ${name}
            DESTINATION ${PV_INSTALL_PLUGIN_DIR}
            COMPONENT Runtime)
  ENDIF (PV_INSTALL_PLUGIN_DIR)
ENDMACRO(internal_paraview_install_plugin)

# helper PV_PLUGIN_LIST_CONTAINS macro
MACRO(PV_PLUGIN_LIST_CONTAINS var value)
  SET(${var})
  FOREACH (value2 ${ARGN})
    IF (${value} STREQUAL ${value2})
      SET(${var} TRUE)
    ENDIF (${value} STREQUAL ${value2})
  ENDFOREACH (value2)
ENDMACRO(PV_PLUGIN_LIST_CONTAINS)

# helper PV_PLUGIN_PARSE_ARGUMENTS macro
MACRO(PV_PLUGIN_PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})
    PV_PLUGIN_LIST_CONTAINS(is_arg_name ${arg} ${arg_names})
    IF (is_arg_name)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name)
      PV_PLUGIN_LIST_CONTAINS(is_option ${arg} ${option_names})
      IF (is_option)
        SET(${prefix}_${arg} TRUE)
      ELSE (is_option)
        SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option)
    ENDIF (is_arg_name)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PV_PLUGIN_PARSE_ARGUMENTS)

# Macro to encode any file(s) as a string. This creates a new cxx file with a
# declaration of a "const char*" string with the same name as the file.
# Example:
# encode_files_as_strings(cxx_files
#  vtkLightingHelper_s.glsl
#  vtkColorMaterialHelper_vs.glsl
#  )
# Will create 2 cxx files with 2 strings: const char* vtkLightingHelper_s and
# const char* vtkColorMaterialHelper_vs.
MACRO(ENCODE_FILES_AS_STRINGS OUT_SRCS)
  foreach(file ${ARGN})
    GET_FILENAME_COMPONENT(file "${file}" ABSOLUTE)
    GET_FILENAME_COMPONENT(file_name "${file}" NAME_WE)
    set(src ${file})
    set(res ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.cxx)
    add_custom_command(
      OUTPUT ${res}
      DEPENDS ${src} vtkEncodeString
      COMMAND vtkEncodeString
      ARGS ${res} ${src} ${file_name}
      )
    set(${OUT_SRCS} ${${OUT_SRCS}} ${res})
  endforeach(file)
ENDMACRO(ENCODE_FILES_AS_STRINGS)

# create plugin glue code for a server manager extension
# consisting of server manager XML and VTK classes
# sets OUTSRCS with the generated code
MACRO(ADD_SERVER_MANAGER_EXTENSION OUTSRCS Name Version XMLFile)
  SET (plugin_type_servermanager TRUE)
  SET (SM_PLUGIN_INCLUDES)
  SET (XML_INTERFACES_INIT)

  # if (XMLFile) doesn't work correctly in a macro. We need to
  # set a local variable.
  set (xmlfiles ${XMLFile})
  if (xmlfiles)
    # generate a header from all the xmls specified.
    set(XML_HEADER "${CMAKE_CURRENT_BINARY_DIR}/vtkSMXML_${Name}.h")

    generate_header(${XML_HEADER}
      PREFIX "${Name}"
      SUFFIX "Interfaces"
      VARIABLE function_names
      FILES ${xmlfiles})

    foreach (func_name ${function_names})
      set (XML_INTERFACES_INIT
        "${XML_INTERFACES_INIT}  PushBack(xmls, ${func_name});\n")
    endforeach()

    set (SM_PLUGIN_INCLUDES "${SM_PLUGIN_INCLUDES}#include \"${XML_HEADER}\"\n")
  endif()

  SET(HDRS)

  FOREACH(SRC ${ARGN})
    GET_FILENAME_COMPONENT(src_name "${SRC}" NAME_WE)
    GET_FILENAME_COMPONENT(src_path "${SRC}" ABSOLUTE)
    GET_FILENAME_COMPONENT(src_path "${src_path}" PATH)
    SET(HDR)
    IF(EXISTS "${src_path}/${src_name}.h")
      SET(HDR "${src_path}/${src_name}.h")
    ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${src_name}.h")
      SET(HDR "${CMAKE_CURRENT_BINARY_DIR}/${src_name}.h")
    ENDIF()
    LIST(APPEND HDRS ${HDR})
  ENDFOREACH(SRC ${ARGN})

  SET(CS_SRCS)
  IF(HDRS)
    include(vtkWrapClientServer)

    # Plugins should not use unified bindings. The problem arises because the
    # PythonD library links to the plugin itself, but the CS wrapping code
    # lives in the plugin as well. With unified bindings, the CS wrapping
    # needs to link to the PythonD library which causes a circular
    # dependency. The solution is probably to compile all bindings for a
    # plugin into a single library.
    set(NO_PYTHON_BINDINGS_AVAILABLE TRUE)
    VTK_WRAP_ClientServer(${Name} CS_SRCS "${HDRS}")
    # only generate the instantiator code for cxx classes that'll be included in
    # the plugin
    SET(INITIALIZE_WRAPPING 1)
  ELSE(HDRS)
    SET(INITIALIZE_WRAPPING 0)
  ENDIF(HDRS)

  SET(${OUTSRCS} ${CS_SRCS} ${XML_HEADER})

ENDMACRO(ADD_SERVER_MANAGER_EXTENSION)

MACRO(ADD_PYTHON_EXTENSION OUTSRCS NAME VERSION)
  SET(PYSRCFILES ${ARGN})
  SET(WRAP_PY_HEADERS)
  SET(WRAP_PYTHON_INCLUDES)
  SET(PY_MODULE_LIST)
  SET(PY_LOADER_LIST)
  SET(PY_PACKAGE_FLAGS)
  SET(QT_COMPONENTS_GUI_RESOURCES_CONTENTS)
  FOREACH(PYFILE ${PYSRCFILES})
    GET_FILENAME_COMPONENT(PYFILE_ABSOLUTE "${PYFILE}" ABSOLUTE)
    GET_FILENAME_COMPONENT(PYFILE_PACKAGE "${PYFILE}" PATH)
    GET_FILENAME_COMPONENT(PYFILE_NAME "${PYFILE}" NAME_WE)
    SET(PACKAGE_FLAG "0")
    IF (PYFILE_PACKAGE)
      STRING(REPLACE "/" "." PYFILE_PACKAGE "${PYFILE_PACKAGE}")
      IF (${PYFILE_NAME} STREQUAL "__init__")
        SET(PYFILE_MODULE "${PYFILE_PACKAGE}")
        SET(PACKAGE_FLAG "1")
      ELSE (${PYFILE_NAME} STREQUAL "__init__")
        SET(PYFILE_MODULE "${PYFILE_PACKAGE}.${PYFILE_NAME}")
      ENDIF (${PYFILE_NAME} STREQUAL "__init__")
    ELSE (PYFILE_PACKAGE)
      SET(PYFILE_MODULE "${PYFILE_NAME}")
    ENDIF (PYFILE_PACKAGE)
    STRING(REPLACE "." "_" PYFILE_MODULE_MANGLED "${PYFILE_MODULE}")
    SET(PY_HEADER "${CMAKE_CURRENT_BINARY_DIR}/WrappedPython_${NAME}_${PYFILE_MODULE_MANGLED}.h")
    ADD_CUSTOM_COMMAND(
      OUTPUT "${PY_HEADER}"
      DEPENDS "${PYFILE_ABSOLUTE}" kwProcessXML
      COMMAND kwProcessXML
      ARGS "${PY_HEADER}" "module_${PYFILE_MODULE_MANGLED}_" "_string" "_source" "${PYFILE_ABSOLUTE}"
      )
    SET(WRAP_PY_HEADERS ${WRAP_PY_HEADERS} "${PY_HEADER}")
    SET(WRAP_PYTHON_INCLUDES
      "${WRAP_PYTHON_INCLUDES}#include \"${PY_HEADER}\"\n")
    IF(PY_MODULE_LIST)
      SET(PY_MODULE_LIST
        "${PY_MODULE_LIST},\n        \"${PYFILE_MODULE}\"")
      SET(PY_LOADER_LIST
        "${PY_LOADER_LIST},\n        module_${PYFILE_MODULE_MANGLED}_${PYFILE_NAME}_source()")
      SET(PY_PACKAGE_FLAGS "${PY_PACKAGE_FLAGS}, ${PACKAGE_FLAG}")
    ELSE(PY_MODULE_LIST)
      SET(PY_MODULE_LIST "\"${PYFILE_MODULE}\"")
      SET(PY_LOADER_LIST
        "module_${PYFILE_MODULE_MANGLED}_${PYFILE_NAME}_source()")
      SET(PY_PACKAGE_FLAGS "${PACKAGE_FLAG}")
    ENDIF(PY_MODULE_LIST)
  ENDFOREACH(PYFILE ${PYSRCFILES})

  # Create source code to get Python source from the plugin.
  SET (plugin_type_python TRUE)
  SET(${OUTSRCS} "${PY_INIT_SRC}" "${WRAP_PY_HEADERS}")

ENDMACRO(ADD_PYTHON_EXTENSION)


#------------------------------------------------------------------------------
# Register a custom pqPropertyWidget class for a property.
# pqPropertyWidget instances are used
# to create widgets for properties in the Properties Panel.
# Usage:
#   add_paraview_property_widget(OUTIFACES OUTSRCS
#     TYPE "<string identifier>"
#     CLASS_NAME "<classname>")
macro(add_paraview_property_widget outifaces outsrcs)
  set (pwi_widget 1)
  set (pwi_group 0)
  set (pwi_decorator 0)
  __add_paraview_property_widget(${outifaces} ${outsrcs} ${ARGN})
endmacro()

#------------------------------------------------------------------------------
# Register a custom pqPropertyWidget class for a property group.
# pqPropertyWidget instances are used to create widgets for properties in the
# Properties Panel.
# Usage:
#   add_paraview_property_group_widget(OUTIFACES OUTSRCS
#     TYPE "<string identifier>"
#     CLASS_NAME "<classname>")
macro(add_paraview_property_group_widget outifaces outsrcs)
  set (pwi_widget 0)
  set (pwi_group 1)
  set (pwi_decorator 0)
  __add_paraview_property_widget(${outifaces} ${outsrcs} ${ARGN})
endmacro()

#------------------------------------------------------------------------------
# Register a custom pqPropertyWidgetDecorator.
# pqPropertyWidgetDecorator instances are used to add custom logic to
# pqPropertyWidget.
# Usage:
#   add_paraview_property_widget_decorator(OUTIFACES OUTSRCS
#     TYPE "<string identifier>"
#     CLASS_NAME "<classname>")
macro(add_paraview_property_widget_decorator outifaces outsrcs)
  set (pwi_widget 0)
  set (pwi_group 0)
  set (pwi_decorator 1)
  __add_paraview_property_widget(${outifaces} ${outsrcs} ${ARGN})
endmacro()

#------------------------------------------------------------------------------
# Internal function used by add_paraview_property_widget,
# add_paraview_property_group_widget and add_paraview_property_widget_decorator
function(__add_paraview_property_widget outifaces outsrcs)
  set (_type)
  set (_classname)

  set (_doing)
  foreach (arg ${ARGN})
    if (NOT _doing AND (arg MATCHES "^(TYPE|CLASS_NAME)$"))
      set (_doing "${arg}")
    elseif ("${_doing}" STREQUAL "TYPE")
      set (_type "${arg}")
      set (_doing)
    elseif ("${_doing}" STREQUAL "CLASS_NAME")
      set (_classname "${arg}")
      set (_doing)
    else()
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    endif()
  endforeach()

  if (_type AND _classname)
    set (name ${_classname}PWI)
    set (type ${_type})
    set (classname ${_classname})
    configure_file(${ParaView_CMAKE_DIR}/pqPropertyWidgetInterface.h.in
                   ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.h
                   @ONLY)
    configure_file(${ParaView_CMAKE_DIR}/pqPropertyWidgetInterface.cxx.in
                   ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.cxx
                   @ONLY)

    set (_moc_srcs)
    qt4_wrap_cpp(_moc_srcs ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.h)

    set (${outifaces} ${name} PARENT_SCOPE)
    set (${outsrcs}
         ${_moc_srcs}
         ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.cxx
         ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.h
         PARENT_SCOPE)
  else()
    message(AUTHOR_WARNING "Missing required arguments.")
  endif()
endfunction()

# create implementation for a custom object panel interface
# ADD_PARAVIEW_OBJECT_PANEL(
#    OUTIFACES
#    OUTSRCS
#    [CLASS_NAME classname]
#    XML_NAME xmlname
#    XML_GROUP xmlgroup
#  CLASS_NAME: optional name for the class that implements pqObjectPanel
#              if none give ${XML_NAME}Panel is assumed (if XML_NAME is a list, then
#              the first name in  the list is assumed).
#  XML_GROUP : the xml group of the source/filter this panel corresponds with
#  XML_NAME  : the xml name of the source/filter this panel corresponds with.
#              XML_NAME can be single name or a list of names.
MACRO(ADD_PARAVIEW_OBJECT_PANEL OUTIFACES OUTSRCS)

  SET(ARG_CLASS_NAME)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;XML_NAME;XML_GROUP" "" ${ARGN} )

  IF(ARG_CLASS_NAME)
    SET(PANEL_NAME ${ARG_CLASS_NAME})
  ELSE(ARG_CLASS_NAME)
    LIST(GET ${ARG_XML_NAME} 0 first_xml_name)
    SET(PANEL_NAME ${first_xml_name}Panel)
  ENDIF(ARG_CLASS_NAME)
  SET(PANEL_XML_NAME ${ARG_XML_NAME})
  SET(PANEL_XML_GROUP ${ARG_XML_GROUP})
  SET(${OUTIFACES} ${PANEL_NAME})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqObjectPanelImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqObjectPanelImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx @ONLY)

  SET(PANEL_MOC_SRCS)
  QT4_WRAP_CPP(PANEL_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h)

 SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h
      ${PANEL_MOC_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_OBJECT_PANEL)

# create implementation for a custom display panel interface
# ADD_PARAVIEW_DISPLAY_PANEL(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#    XML_NAME xmlname
#  CLASS_NAME: pqDisplayPanel
#  XML_NAME : the xml name of the display this panel corresponds with
MACRO(ADD_PARAVIEW_DISPLAY_PANEL OUTIFACES OUTSRCS)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;XML_NAME" "" ${ARGN} )

  SET(PANEL_NAME ${ARG_CLASS_NAME})
  SET(PANEL_XML_NAME ${ARG_XML_NAME})
  SET(${OUTIFACES} ${PANEL_NAME})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqDisplayPanelImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqDisplayPanelImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx @ONLY)

  SET(DISPLAY_MOC_SRCS)
  QT4_WRAP_CPP(DISPLAY_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h
      ${DISPLAY_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_DISPLAY_PANEL)

# create implementation for a custom view
# Usage:
# ADD_PARAVIEW_VIEW_MODULE( OUTIFACES OUTSRCS
#     VIEW_TYPE Type
#     VIEW_XML_GROUP Group
#     [VIEW_XML_NAME Name]
#     [VIEW_NAME Name]
#     [DISPLAY_PANEL Display]
#     [DISPLAY_TYPE Display]

# for the given server manager XML
#  <SourceProxy name="MyFilter" class="MyFilter" label="My Filter">
#    ...
#    <Hints>
#      <View type="MyView" />
#    </Hints>
#  </SourceProxy>
#  ....
# <ProxyGroup name="plotmodules">
#  <ViewProxy name="MyView"
#      base_proxygroup="newviews" base_proxyname="ViewBase"
#      representation_name="MyDisplay">
#  </ViewProxy>
# </ProxyGroup>

#  VIEW_TYPE = "MyView"
#  VIEW_XML_GROUP = "plotmodules"
#  VIEW_XML_NAME is optional and defaults to VIEW_TYPE
#  VIEW_NAME is optional and gives a friendly name for the view type
#  DISPLAY_TYPE is optional and defaults to pqDataRepresentation
#  DISPLAY_PANEL gives the name of the display panel
#  DISPLAY_XML is the XML name of the display for this view and is required if
#     DISPLAY_PANEL is set
#
#  if DISPLAY_PANEL is MyDisplay, then "MyDisplayPanel.h" is looked for.
#  a class MyView derived from pqGenericViewModule is expected to be in "MyView.h"

MACRO(ADD_PARAVIEW_VIEW_MODULE OUTIFACES OUTSRCS)

  SET(PANEL_SRCS)
  SET(ARG_VIEW_TYPE)
  SET(ARG_VIEW_NAME)
  SET(ARG_VIEW_XML_GROUP)
  SET(ARG_VIEW_XML_NAME)
  SET(ARG_DISPLAY_PANEL)
  SET(ARG_DISPLAY_XML)
  SET(ARG_DISPLAY_TYPE)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "VIEW_TYPE;VIEW_XML_GROUP;VIEW_XML_NAME;VIEW_NAME;DISPLAY_PANEL;DISPLAY_TYPE;DISPLAY_XML"
                  "" ${ARGN} )

  IF(NOT ARG_VIEW_TYPE OR NOT ARG_VIEW_XML_GROUP)
    MESSAGE(ERROR " ADD_PARAVIEW_VIEW_MODULE called without VIEW_TYPE or VIEW_XML_GROUP")
  ENDIF(NOT ARG_VIEW_TYPE OR NOT ARG_VIEW_XML_GROUP)

  IF(ARG_DISPLAY_PANEL)
    IF(NOT ARG_DISPLAY_XML)
      MESSAGE(ERROR " ADD_PARAVIEW_VIEW_MODULE called with DISPLAY_PANEL but DISPLAY_XML not specified")
    ENDIF(NOT ARG_DISPLAY_XML)
  ENDIF(ARG_DISPLAY_PANEL)

  SET(${OUTIFACES} ${ARG_VIEW_TYPE})
  IF(NOT ARG_VIEW_XML_NAME)
    SET(ARG_VIEW_XML_NAME ${ARG_VIEW_TYPE})
  ENDIF(NOT ARG_VIEW_XML_NAME)
  IF(ARG_VIEW_NAME)
    SET(VIEW_TYPE_NAME ${ARG_VIEW_NAME})
  ELSE(ARG_VIEW_NAME)
    SET(VIEW_TYPE_NAME ${ARG_VIEW_TYPE})
  ENDIF(ARG_VIEW_NAME)

  IF(NOT ARG_DISPLAY_TYPE)
    SET(ARG_DISPLAY_TYPE "pqDataRepresentation")
  ENDIF(NOT ARG_DISPLAY_TYPE)

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqViewModuleImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqViewModuleImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.cxx @ONLY)

  IF(PARAVIEW_BUILD_QT_GUI)
    SET(VIEW_MOC_SRCS)
    QT4_WRAP_CPP(VIEW_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.h)
  ENDIF(PARAVIEW_BUILD_QT_GUI)

  IF(ARG_DISPLAY_PANEL)
    ADD_PARAVIEW_DISPLAY_PANEL(OUT_PANEL_IFACES PANEL_SRCS
                               CLASS_NAME ${ARG_DISPLAY_PANEL}
                               XML_NAME ${ARG_DISPLAY_XML})
    SET(${OUTIFACES} ${ARG_VIEW_TYPE} ${OUT_PANEL_IFACES})
  ENDIF(ARG_DISPLAY_PANEL)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.h
      ${VIEW_MOC_SRCS}
      ${PANEL_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_VIEW_MODULE)

# create implementation for a custom view options interface
# ADD_PARAVIEW_VIEW_OPTIONS(
#    OUTIFACES
#    OUTSRCS
#    VIEW_TYPE type
#    [ACTIVE_VIEW_OPTIONS classname]
#    [GLOBAL_VIEW_OPTIONS classname]
#
#  VIEW_TYPE: the type of view the options panels are associated with
#  ACTIVE_VIEW_OPTIONS: optional name for the class that implements pqActiveViewOptions
#                       this is to add options that are specific to a view instance
#  GLOBAL_VIEW_OPTIONS: optional name for the class that implements pqOptionsContainer
#                       this is to add options that apply to all view instances
MACRO(ADD_PARAVIEW_VIEW_OPTIONS OUTIFACES OUTSRCS)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "VIEW_TYPE;ACTIVE_VIEW_OPTIONS;GLOBAL_VIEW_OPTIONS" "" ${ARGN} )

  IF(NOT ARG_VIEW_TYPE)
    MESSAGE(ERROR " ADD_PARAVIEW_VIEW_OPTIONS called without VIEW_TYPE")
  ENDIF(NOT ARG_VIEW_TYPE)

  IF(NOT ARG_ACTIVE_VIEW_OPTIONS AND NOT ARG_GLOBAL_VIEW_OPTIONS)
    MESSAGE(ERROR " ADD_PARAVIEW_VIEW_OPTIONS called without ACTIVE_VIEW_OPTIONS or GLOBAL_VIEW_OPTIONS")
  ENDIF(NOT ARG_ACTIVE_VIEW_OPTIONS AND NOT ARG_GLOBAL_VIEW_OPTIONS)

  SET(HAVE_ACTIVE_VIEW_OPTIONS 0)
  SET(HAVE_GLOBAL_VIEW_OPTIONS 0)

  IF(ARG_ACTIVE_VIEW_OPTIONS)
    SET(HAVE_ACTIVE_VIEW_OPTIONS 1)
  ENDIF(ARG_ACTIVE_VIEW_OPTIONS)

  IF(ARG_GLOBAL_VIEW_OPTIONS)
    SET(HAVE_GLOBAL_VIEW_OPTIONS 1)
  ENDIF(ARG_GLOBAL_VIEW_OPTIONS)

  SET(${OUTIFACES} ${ARG_VIEW_TYPE}Options)

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqViewOptionsImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqViewOptionsImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.cxx @ONLY)

  SET(PANEL_MOC_SRCS)
  QT4_WRAP_CPP(PANEL_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.h)

 SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.h
      ${PANEL_MOC_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_VIEW_OPTIONS)

# create implementation for a custom menu or toolbar
# ADD_PARAVIEW_ACTION_GROUP(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#    GROUP_NAME groupName
#
#    CLASS_NAME is the name of the class that implements a QActionGroup
#    GROUP_NAME is the name of the group "MenuBar/MyMenu" or "ToolBar/MyToolBar"
MACRO(ADD_PARAVIEW_ACTION_GROUP OUTIFACES OUTSRCS)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;GROUP_NAME" "" ${ARGN} )

  SET(${OUTIFACES} ${ARG_CLASS_NAME})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqActionGroupImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqActionGroupImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_ACTION_GROUP)

# create implementation for a custom view frame action interface
# ADD_PARAVIEW_VIEW_FRAME_ACTION_GROUP(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#
#    CLASS_NAME is the name of the class that implements a QActionGroup
MACRO(ADD_PARAVIEW_VIEW_FRAME_ACTION_GROUP OUTIFACES OUTSRCS)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME" "" ${ARGN} )

  SET(${OUTIFACES} ${ARG_CLASS_NAME})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqViewFrameActionGroupImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqViewFrameActionGroupImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_VIEW_FRAME_ACTION_GROUP)

# create implementation for a dock window interface
# ADD_PARAVIEW_DOCK_WINDOW(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#    [DOCK_AREA areaname]
#
#  CLASS_NAME: is the name of the class that implements a QDockWidget
#  DOCK_AREA: option to specify the dock area (Left | Right | Top | Bottom)
#             Left is the default
MACRO(ADD_PARAVIEW_DOCK_WINDOW OUTIFACES OUTSRCS)

  SET(ARG_DOCK_AREA)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;DOCK_AREA" "" ${ARGN} )

  IF(NOT ARG_DOCK_AREA)
    SET(ARG_DOCK_AREA Left)
  ENDIF(NOT ARG_DOCK_AREA)
  SET(${OUTIFACES} ${ARG_CLASS_NAME})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqDockWindowImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqDockWindowImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_DOCK_WINDOW)


# Create implementation for an auto start interface.
# ADD_PARAVIEW_AUTO_START(
#   OUTIFACES
#   OUTSRCS
#   CLASS_NAME classname
#   [STARTUP startup callback method name]
#   [SHUTDOWN shutdown callback method name]
# )
# CLASS_NAME : is the name of the class that implements 2 methods which will be
#              called on startup and shutdown. The names of these methods can be
#              optionally specified using STARTUP and SHUTDOWN.
# STARTUP    : name of the method on class CLASS_NAME which should be called
#              when the plugins loads. Default is startup.
# SHUTDOWN   : name pf the method on class CLASS_NAME which should be called
#              when the application shuts down. Default is shutdown.
MACRO(ADD_PARAVIEW_AUTO_START OUTIFACES OUTSRCS)
  SET(ARG_STARTUP)
  SET(ARG_SHUTDOWN)
  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;STARTUP;SHUTDOWN" "" ${ARGN})

  IF (NOT ARG_STARTUP)
    SET (ARG_STARTUP startup)
  ENDIF (NOT ARG_STARTUP)

  IF (NOT ARG_SHUTDOWN)
    SET (ARG_SHUTDOWN shutdown)
  ENDIF (NOT ARG_SHUTDOWN)

  SET(${OUTIFACES} ${ARG_CLASS_NAME})
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqAutoStartImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqAutoStartImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_AUTO_START)

# Create implementation for a custom display panel decorator interface.
# Decorators are used to add additional decorations to display panels.
# ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#    PANEL_TYPES type1 type2 ..)
# CLASS_NAME   : The class name for the decorator. The decorator must be a
#                QObject subclass. The display panel is passed as the parent for
#                the object.
# PANEL_TYPES  : list of classnames for the display panel which this decorator
#                can decorate.
MACRO(ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR OUTIFACES OUTSRCS)
  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;PANEL_TYPES" "" ${ARGN})

  SET(${OUTIFACES} ${ARG_CLASS_NAME})
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqDisplayPanelDecoratorImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqDisplayPanelDecoratorImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR)


# Creates implementation for a pq3DWidgetInterface to add new 3D widgets to
# ParaView.
# ADD_3DWIDGET(OUTIFACES OUTSRCS
#   CLASS_NAME <pq3DWidget subclass being added>
#   WIDGET_TYPE <string identifying the 3DWidget typically used in the <Hints/>
#               for the proxy when specifying the PropertyGroup.
#   )
MACRO(ADD_3DWIDGET OUTIFACES OUTSRCS)
  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;WIDGET_TYPE" "" ${ARGN})

  SET(${OUTIFACES} ${ARG_CLASS_NAME})
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pq3DWidgetImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pq3DWidgetImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_3DWIDGET)


#  Macro for a GraphLayoutStrategy plugin
#  STRATEGY_TYPE = "MyStrategy"
MACRO(ADD_PARAVIEW_GRAPH_LAYOUT_STRATEGY OUTIFACES OUTSRCS)

  SET(ARG_STRATEGY_TYPE)
  SET(ARG_STRATEGY_LABEL)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "STRATEGY_TYPE;STRATEGY_LABEL"
                  "" ${ARGN} )

  IF(NOT ARG_STRATEGY_TYPE OR NOT ARG_STRATEGY_LABEL)
    MESSAGE(ERROR " ADD_PARAVIEW_GRAPH_LAYOUT_STRATEGY called without STRATEGY_TYPE")
  ENDIF(NOT ARG_STRATEGY_TYPE OR NOT ARG_STRATEGY_LABEL)

  SET(${OUTIFACES} ${ARG_STRATEGY_TYPE})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqGraphLayoutStrategyImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqGraphLayoutStrategyImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx @ONLY)

  SET(LAYOUT_MOC_SRCS)
  QT4_WRAP_CPP(LAYOUT_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h
      ${LAYOUT_MOC_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_GRAPH_LAYOUT_STRATEGY)

#  Macro for a AreaLayoutStrategy plugin
#  STRATEGY_TYPE = "MyStrategy"
MACRO(ADD_PARAVIEW_TREE_LAYOUT_STRATEGY OUTIFACES OUTSRCS)

  SET(ARG_STRATEGY_TYPE)
  SET(ARG_STRATEGY_LABEL)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "STRATEGY_TYPE;STRATEGY_LABEL"
                  "" ${ARGN} )

  IF(NOT ARG_STRATEGY_TYPE OR NOT ARG_STRATEGY_LABEL)
    MESSAGE(ERROR " ADD_PARAVIEW_TREE_LAYOUT_STRATEGY called without STRATEGY_TYPE")
  ENDIF(NOT ARG_STRATEGY_TYPE OR NOT ARG_STRATEGY_LABEL)

  SET(${OUTIFACES} ${ARG_STRATEGY_TYPE})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqTreeLayoutStrategyImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqTreeLayoutStrategyImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx @ONLY)

  SET(LAYOUT_MOC_SRCS)
  QT4_WRAP_CPP(LAYOUT_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h)

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h
      ${LAYOUT_MOC_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_TREE_LAYOUT_STRATEGY)

# create implementation for a Qt/ParaView plugin given a
# module name and a list of interfaces
# ADD_PARAVIEW_GUI_EXTENSION(OUTSRCS NAME VERSION INTERFACES iface1;iface2;iface3)
MACRO(ADD_PARAVIEW_GUI_EXTENSION OUTSRCS NAME VERSION)
  SET (plugin_type_gui TRUE)
  SET(INTERFACE_INCLUDES)
  SET(PUSH_BACK_PV_INTERFACES "#define PUSH_BACK_PV_INTERFACES(arg)\\\n")
  SET(ARG_INTERFACES)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "INTERFACES" "" ${ARGN} )

  IF(ARG_INTERFACES)
    FOREACH(IFACE ${ARG_INTERFACES})
      SET(TMP "#include \"${IFACE}Implementation.h\"")
      SET(INTERFACE_INCLUDES "${INTERFACE_INCLUDES}\n${TMP}")
      SET(TMP "  arg.push_back(new ${IFACE}Implementation(this));\\\n")
      SET(PUSH_BACK_PV_INTERFACES "${PUSH_BACK_PV_INTERFACES}${TMP}")
    ENDFOREACH(IFACE ${ARG_INTERFACES})
  ENDIF(ARG_INTERFACES)
  SET (PUSH_BACK_PV_INTERFACES "${PUSH_BACK_PV_INTERFACES}\n")

  SET(${OUTSRCS} ${PLUGIN_MOC_SRCS})

ENDMACRO(ADD_PARAVIEW_GUI_EXTENSION)

# internal macro to work around deficiency in FindQt4.cmake, will be removed in
# the future.
MACRO(PARAVIEW_QT4_ADD_RESOURCES outfiles )
  FOREACH (it ${ARGN})
    GET_FILENAME_COMPONENT(outfilename ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    GET_FILENAME_COMPONENT(rc_path ${infile} PATH)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.cxx)
    #  parse file for dependencies
    #  all files are absolute paths or relative to the location of the qrc file
    FILE(READ "${infile}" _RC_FILE_CONTENTS)
    STRING(REGEX MATCHALL "<file[^<]+" _RC_FILES "${_RC_FILE_CONTENTS}")
    SET(_RC_DEPENDS)
    FOREACH(_RC_FILE ${_RC_FILES})
      STRING(REGEX REPLACE "^<file[^>]*>" "" _RC_FILE "${_RC_FILE}")
      STRING(REGEX MATCH "^/|([A-Za-z]:/)" _ABS_PATH_INDICATOR "${_RC_FILE}")
      IF(NOT _ABS_PATH_INDICATOR)
        SET(_RC_FILE "${rc_path}/${_RC_FILE}")
      ENDIF(NOT _ABS_PATH_INDICATOR)
      SET(_RC_DEPENDS ${_RC_DEPENDS} "${_RC_FILE}")
    ENDFOREACH(_RC_FILE)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
      COMMAND ${QT_RCC_EXECUTABLE}
      ARGS ${rcc_options} -name ${outfilename} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile}
      DEPENDS ${_RC_DEPENDS})
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH (it)
ENDMACRO(PARAVIEW_QT4_ADD_RESOURCES)

# create a plugin
#  A plugin may contain only server code, only gui code, or both.
#  SERVER_MANAGER_SOURCES will be wrapped
#  SERVER_MANAGER_XML will be embedded and give to the client when loaded
#  SERVER_SOURCES is for other source files
#  PYTHON_MODULES allows you to embed python sources as modules
#  GUI_INTERFACES is to specify which GUI plugin interfaces were implemented
#  GUI_RESOURCES is to specify qrc files
#  GUI_RESOURCE_FILES is to specify xml files to create a qrc file from
#  GUI_SOURCES is to other GUI sources
#  SOURCES is deprecated, please use SERVER_SOURCES or GUI_SOURCES
#  REQUIRED_ON_SERVER is to specify whether this plugin should be loaded on server
#  REQUIRED_ON_CLIENT is to specify whether this plugin should be loaded on client
#  REQUIRED_PLUGINS is to specify the plugin names that this plugin depends on
#  CS_KITS is experimental option to add wrapped kits. This may change in
#  future.
#  DOCUMENTATION_DIR (optional) :- used to specify a directory containing
#  html/css/png/jpg files that comprise of the documentation for the plugin. In
#  addition, CMake will automatically generate documentation for any proxies
#  defined in XMLs for this plugin.
# ADD_PARAVIEW_PLUGIN(Name Version
#     [DOCUMENTATION_DIR dir]
#     [SERVER_MANAGER_SOURCES source files]
#     [SERVER_MANAGER_XML XMLFile]
#     [SERVER_SOURCES source files]
#     [PYTHON_MODULES python source files]
#     [GUI_INTERFACES interface1 interface2]
#     [GUI_RESOURCES qrc1 qrc2]
#     [GUI_RESOURCE_FILES xml1 xml2]
#     [GUI_SOURCES source files]
#     [SOURCES source files]
#     [REQUIRED_ON_SERVER]
#     [REQUIRED_ON_CLIENT]
#     [REQUIRED_PLUGINS pluginname1 pluginname2]
#     [CS_KITS kit1 kit2...]
#  )
FUNCTION(ADD_PARAVIEW_PLUGIN NAME VERSION)
  SET(QT_RCS)
  SET(GUI_SRCS)
  SET(SM_SRCS)
  SET(PY_SRCS)
  SET(ARG_GUI_INTERFACES)
  SET(ARG_GUI_RESOURCES)
  SET(ARG_GUI_RESOURCE_FILES)
  SET(ARG_SERVER_MANAGER_SOURCES)
  SET(ARG_SERVER_MANAGER_XML)
  SET(ARG_PYTHON_MODULES)
  SET(ARG_SOURCES)
  SET(ARG_SERVER_SOURCES)
  SET(ARG_GUI_SOURCES)
  SET(ARG_REQUIRED_PLUGINS)
  SET(ARG_AUTOLOAD)
  SET(ARG_CS_KITS)
  SET(ARG_DOCUMENTATION_DIR)

  SET(PLUGIN_NAME "${NAME}")
  SET(PLUGIN_VERSION "${VERSION}")
  SET(PLUGIN_REQUIRED_ON_SERVER 1)
  SET(PLUGIN_REQUIRED_ON_CLIENT 1)
  SET(PLUGIN_REQUIRED_PLUGINS)
  SET(HAVE_REQUIRED_PLUGINS 0)
  SET(BINARY_RESOURCES_INIT)
  SET(QRC_RESOURCES_INIT)
  SET(EXTRA_INCLUDES)

  # binary_resources are used to compile in icons and documentation for the
  # plugin. Note that this is not used to compile Qt resources, these are
  # directly compiled into the Qt plugin.
  # (since we don't support icons right now, this is used only for
  # documentation.
  set (binary_resources)


  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

  PV_PLUGIN_PARSE_ARGUMENTS(ARG
    "DOCUMENTATION_DIR;SERVER_MANAGER_SOURCES;SERVER_MANAGER_XML;SERVER_SOURCES;PYTHON_MODULES;GUI_INTERFACES;GUI_RESOURCES;GUI_RESOURCE_FILES;GUI_SOURCES;SOURCES;REQUIRED_PLUGINS;REQUIRED_ON_SERVER;REQUIRED_ON_CLIENT;AUTOLOAD;CS_KITS"
    "" ${ARGN} )

  PV_PLUGIN_LIST_CONTAINS(reqired_server_arg "REQUIRED_ON_SERVER" ${ARGN})
  PV_PLUGIN_LIST_CONTAINS(reqired_client_arg "REQUIRED_ON_CLIENT" ${ARGN})
  IF (reqired_server_arg)
    IF (NOT reqired_client_arg)
      SET(PLUGIN_REQUIRED_ON_CLIENT 0)
    ENDIF (NOT reqired_client_arg)
  ELSE (reqired_server_arg)
    IF (reqired_client_arg)
      SET(PLUGIN_REQUIRED_ON_SERVER 0)
    ENDIF (reqired_client_arg)
  ENDIF (reqired_server_arg)

  IF(ARG_REQUIRED_PLUGINS)
    SET(PLUGIN_REQUIRED_PLUGINS "${ARG_REQUIRED_PLUGINS}")
    SET(HAVE_REQUIRED_PLUGINS 1)
  ENDIF(ARG_REQUIRED_PLUGINS)

  IF(ARG_SERVER_MANAGER_SOURCES OR ARG_SERVER_MANAGER_XML)
    ADD_SERVER_MANAGER_EXTENSION(SM_SRCS ${NAME} ${VERSION} "${ARG_SERVER_MANAGER_XML}"
                                 ${ARG_SERVER_MANAGER_SOURCES})
    set (EXTRA_INCLUDES "${EXTRA_INCLUDES}${SM_PLUGIN_INCLUDES}\n")
  ENDIF(ARG_SERVER_MANAGER_SOURCES OR ARG_SERVER_MANAGER_XML)

  IF (ARG_PYTHON_MODULES)
    IF (PARAVIEW_ENABLE_PYTHON)
      ADD_PYTHON_EXTENSION(PY_SRCS ${NAME} ${VERSION} ${ARG_PYTHON_MODULES})
    ELSE (PARAVIEW_ENABLE_PYTHON)
      MESSAGE(STATUS "Python parameters ignored for ${NAME} plugin because PARAVIEW_ENABLE_PYTHON is off.")
    ENDIF (PARAVIEW_ENABLE_PYTHON)
  ENDIF (ARG_PYTHON_MODULES)

  IF(PARAVIEW_BUILD_QT_GUI)

    # if server-manager xmls are specified, we can generate documentation from
    # them, if Qt is enabled.
    if (ARG_SERVER_MANAGER_XML)
      generate_htmls_from_xmls(proxy_documentation_files
        "${ARG_SERVER_MANAGER_XML}"
        "" # FIXME: not sure here. How to deal with this for plugins?
        "${CMAKE_CURRENT_BINARY_DIR}/doc")
    endif()

    # generate the qch file for the plugin if any documentation is provided.
    if (proxy_documentation_files OR ARG_DOCUMENTATION_DIR)
      build_help_project(${NAME}
        DESTINATION_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc"
        DOCUMENTATION_SOURCE_DIR "${ARG_DOCUMENTATION_DIR}"
        FILEPATTERNS "*.html;*.css;*.png;*.jpg"
        DEPENDS "${proxy_documentation_files}" )

      # we don't compile the help project as a Qt resource. Instead it's
      # packaged as a SM resource. This makes it possible for
      # server-only plugins to provide documentation to the client without
      generate_header("${CMAKE_CURRENT_BINARY_DIR}/${NAME}_doc.h"
        SUFFIX "_doc"
        VARIABLE function_names
        BINARY
        FILES "${CMAKE_CURRENT_BINARY_DIR}/doc/${NAME}.qch")
      list(APPEND binary_resources ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_doc.h)
      set (EXTRA_INCLUDES "${EXTRA_INCLUDES}#include \"${CMAKE_CURRENT_BINARY_DIR}/${NAME}_doc.h\"")
      foreach (func_name ${function_names})
        set (BINARY_RESOURCES_INIT
          "${BINARY_RESOURCES_INIT}  PushBack(resources, ${func_name});\n")
      endforeach()
    endif()

    IF(ARG_GUI_RESOURCE_FILES)
      # The generated qrc file has resource prefix "/name/ParaViewResources"
      # which helps is avoiding conflicts with resources from different
      # plugins
      GENERATE_QT_RESOURCE_FROM_FILES(
        "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.qrc"
         "/${NAME}/ParaViewResources"
         "${ARG_GUI_RESOURCE_FILES}")
      SET(ARG_GUI_RESOURCES ${ARG_GUI_RESOURCES}
        "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.qrc")
    ENDIF(ARG_GUI_RESOURCE_FILES)

    IF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_SOURCES)
      ADD_PARAVIEW_GUI_EXTENSION(GUI_SRCS ${NAME} ${VERSION} INTERFACES "${ARG_GUI_INTERFACES}")
    ENDIF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_SOURCES)

    IF(ARG_GUI_RESOURCES)
      # When building statically, we need to add stub to initialize the Qt
      # resources otherwise icons, GUI configuration xmls, etc. don't get
      # loaded/initialized when the plugin is statically imported.
      if (NOT PARAVIEW_BUILD_SHARED_LIBS)
        foreach (qrc_file IN LISTS ARG_GUI_RESOURCES)
          get_filename_component(rc_name "${qrc_file}" NAME_WE)
          set (QRC_RESOURCES_INIT
            "${QRC_RESOURCES_INIT}Q_INIT_RESOURCE(${rc_name});\n")
        endforeach()
      endif()

      PARAVIEW_QT4_ADD_RESOURCES(QT_RCS ${ARG_GUI_RESOURCES})
      SET(GUI_SRCS ${GUI_SRCS} ${QT_RCS})
    ENDIF(ARG_GUI_RESOURCES)

    SET(GUI_SRCS ${GUI_SRCS} ${ARG_GUI_SOURCES})

  ELSE(PARAVIEW_BUILD_QT_GUI)

    IF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_RESOURCE_FILES)
      MESSAGE(STATUS "GUI parameters ignored for ${NAME} plugin because PARAVIEW_BUILD_QT_GUI is off.")
    ENDIF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_RESOURCE_FILES)

  ENDIF(PARAVIEW_BUILD_QT_GUI)

  SET(SM_SRCS
    ${binary_resources}
    ${ARG_SERVER_MANAGER_SOURCES}
    ${SM_SRCS}
    ${ARG_SERVER_SOURCES}
    ${PY_SRCS})

  set (extradependencies)

  SET (PLUGIN_EXTRA_CS_INITS)
  SET (PLUGIN_EXTRA_CS_INITS_EXTERNS)
  SET (INITIALIZE_EXTRA_CS_MODULES)
  IF (ARG_CS_KITS)
    FOREACH(kit ${ARG_CS_KITS})
      SET (PLUGIN_EXTRA_CS_INITS
        "${kit}CS_Initialize(interp);\n${PLUGIN_EXTRA_CS_INITS}")
      SET (PLUGIN_EXTRA_CS_INITS_EXTERNS
        "extern \"C\" void ${kit}CS_Initialize(vtkClientServerInterpreter*);\n${PLUGIN_EXTRA_CS_INITS_EXTERNS}")
    ENDFOREACH(kit)

    SET (INITIALIZE_EXTRA_CS_MODULES TRUE)
  ENDIF (ARG_CS_KITS)

  # If this plugin is being built as a part of an environment that provdes other
  # modules, we handle those.
  if (pv-plugin AND ${pv-plugin}_CS_MODULES)
    foreach(module ${${pv-plugin}_CS_MODULES})
      set (PLUGIN_EXTRA_CS_INITS
           "${module}CS_Initialize(interp);\n${PLUGIN_EXTRA_CS_INITS}")
      set (PLUGIN_EXTRA_CS_INITS_EXTERNS
           "extern \"C\" void ${module}CS_Initialize(vtkClientServerInterpreter*);\n${PLUGIN_EXTRA_CS_INITS_EXTERNS}")
      list(APPEND extradependencies ${module} ${module}CS)
    endforeach()
    set(INITIALIZE_EXTRA_CS_MODULES TRUE)
  endif()

  IF(GUI_SRCS OR SM_SRCS OR ARG_SOURCES OR ARG_PYTHON_MODULES)
    CONFIGURE_FILE(
      ${ParaView_CMAKE_DIR}/pqParaViewPlugin.h.in
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.h @ONLY)
    CONFIGURE_FILE(
      ${ParaView_CMAKE_DIR}/pqParaViewPlugin.cxx.in
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.cxx @ONLY)

    SET (plugin_sources
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.h
    )
    IF (plugin_type_gui)
      set (__plugin_sources_tmp)
      QT4_WRAP_CPP(__plugin_sources_tmp ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.h)
      SET (plugin_sources ${plugin_sources} ${__plugin_sources_tmp})
    ENDIF (plugin_type_gui)

   if (MSVC)
      # Do not generate manifests for the plugins - caused issues loading plugins
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
    endif(MSVC)

    IF (PARAVIEW_BUILD_SHARED_LIBS)
      ADD_LIBRARY(${NAME} SHARED ${GUI_SRCS} ${SM_SRCS} ${ARG_SOURCES} ${plugin_sources})
    ELSE (PARAVIEW_BUILD_SHARED_LIBS)
      ADD_LIBRARY(${NAME} ${GUI_SRCS} ${SM_SRCS} ${ARG_SOURCES} ${plugin_sources})
      # When building plugins for static builds, Qt requires this flag to be
      # defined. If not defined, when we link the executable against all the
      # plugins, we get redefinied symbols from the plugins.
      set_target_properties(${NAME} PROPERTIES
                                    COMPILE_DEFINITIONS QT_STATICPLUGIN)
    ENDIF (PARAVIEW_BUILD_SHARED_LIBS)

    IF(MSVC)
      # Do not generate manifests for the plugins - caused issues loading plugins
      set_target_properties(${NAME} PROPERTIES LINK_FLAGS "/MANIFEST:NO")
    ENDIF(MSVC)

    IF(plugin_type_gui OR GUI_SRCS)
      target_link_libraries(${NAME} LINK_PUBLIC pqComponents)
    ENDIF(plugin_type_gui OR GUI_SRCS)
    IF(SM_SRCS)
      target_link_libraries(${NAME} LINK_PUBLIC vtkPVServerManagerApplication
        vtkPVServerManagerDefault
        vtkPVServerManagerApplicationCS)
    ENDIF(SM_SRCS)

    if (extradependencies)
      target_link_libraries(${NAME} LINK_PUBLIC ${extradependencies})
    endif()

    # Add install rules for the plugin. Currently only the plugins in ParaView
    # source are installed.
    internal_paraview_install_plugin(${NAME})

    IF(ARG_AUTOLOAD)
      message(WARNING "AUTOLOAD option is deprecated. Plugins built within"
        " ParaView source should use pv_plugin(..) macro with AUTOLOAD argument.")
    ENDIF(ARG_AUTOLOAD)
  ENDIF(GUI_SRCS OR SM_SRCS OR ARG_SOURCES OR ARG_PYTHON_MODULES)

ENDFUNCTION(ADD_PARAVIEW_PLUGIN)

# wrap a Plugin into Python so that it can be called from pvclient and pvbatch
#it will produce lib${NAME}Python.so, which you can then
#import in your python script before calling servermanager.LoadPlugin to get
#python access to the classes from the plugin
MACRO(WRAP_PLUGIN_FOR_PYTHON NAME WRAP_LIST WRAP_EXCLUDE_LIST)
  #this was taken from Servers/ServerManager/CMakeLists.txt.
  #I did the same setup and then just inlined the call to
  #VTK/Common/KitCommonPythonWrapBlock so that plugin's name
  #does not to start with "vtk".

  SET_SOURCE_FILES_PROPERTIES(
    ${WRAP_EXCLUDE_LIST}
    WRAP_EXCLUDE)

  SET(Kit_PYTHON_EXTRA_SRCS)

  SET(KIT_PYTHON_LIBS
    vtkPVServerManagerCorePythonD
    ${NAME})

  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(KIT_PYTHON_DEPS)
  SET(VTK_INSTALL_NO_LIBRARIES 1)
  IF(VTKPythonWrapping_INSTALL_BIN_DIR)
    SET(VTK_INSTALL_NO_LIBRARIES)
  ENDIF(VTKPythonWrapping_INSTALL_BIN_DIR)

  SET(VTK_INSTALL_LIB_DIR_CM24 "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR_CM24 "${VTKPythonWrapping_INSTALL_BIN_DIR}")

  #INCLUDE(KitCommonPythonWrapBlock) takes over here
  # Create custom commands to generate the python wrappers for this kit.
  VTK_WRAP_PYTHON3(${NAME}Python KitPython_SRCS "${WRAP_LIST}")

  # Create a shared library containing the python wrappers.  Executables
  # can link to this but it is not directly loaded dynamically as a
  # module.
  ADD_LIBRARY(${NAME}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(${NAME}PythonD ${NAME} ${KIT_PYTHON_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL(TARGETS ${NAME}PythonD
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  SET(KIT_LIBRARY_TARGETS ${KIT_LIBRARY_TARGETS} ${NAME}PythonD)

  # On some UNIX platforms the python library is static and therefore
  # should not be linked into the shared library.  Instead the symbols
  # are exported from the python executable so that they can be used by
  # shared libraries that are linked or loaded.  On Windows and OSX we
  # want to link to the python libray to resolve its symbols
  # immediately.
  IF(WIN32 OR APPLE)
    TARGET_LINK_LIBRARIES (${NAME}PythonD ${VTK_PYTHON_LIBRARIES})
  ENDIF(WIN32 OR APPLE)

  # Add dependencies that may have been generated by VTK_WRAP_PYTHON3 to
  # the python wrapper library.  This is needed for the
  # pre-custom-command hack in Visual Studio 6.
  IF(KIT_PYTHON_DEPS)
    ADD_DEPENDENCIES(${NAME}PythonD ${KIT_PYTHON_DEPS})
  ENDIF(KIT_PYTHON_DEPS)

  # Create a python module that can be loaded dynamically.  It links to
  # the shared library containing the wrappers for this kit.
  PYTHON_ADD_MODULE(${NAME}Python ${NAME}PythonInit.cxx)
  IF(PYTHON_ENABLE_MODULE_${NAME}Python)
    TARGET_LINK_LIBRARIES(${NAME}Python ${NAME}PythonD)

    # Python extension modules on Windows must have the extension ".pyd"
    # instead of ".dll" as of Python 2.5.  Older python versions do support
    # this suffix.
    IF(WIN32 AND NOT CYGWIN)
      SET_TARGET_PROPERTIES(${NAME}Python PROPERTIES SUFFIX ".pyd")
    ENDIF(WIN32 AND NOT CYGWIN)

    # The python modules are installed by a setup.py script which does
    # not know how to adjust the RPATH field of the binary.  Therefore
    # we must simply build the modules with no RPATH at all.  The
    # vtkpython executable in the build tree should have the needed
    # RPATH anyway.
    SET_TARGET_PROPERTIES(${NAME}Python PROPERTIES SKIP_BUILD_RPATH 1)

    IF(WIN32 OR APPLE)
      TARGET_LINK_LIBRARIES (${NAME}Python ${VTK_PYTHON_LIBRARIES})
    ENDIF(WIN32 OR APPLE)

    # Install the extension module at the same location as other libraries.
    IF (NOT VTK_INSTALL_NO_LIBRARIES)
      INSTALL(TARGETS ${NAME}Python
        RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
        LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
        ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
    ENDIF (NOT VTK_INSTALL_NO_LIBRARIES)
  ENDIF(PYTHON_ENABLE_MODULE_${NAME}Python)

ENDMACRO(WRAP_PLUGIN_FOR_PYTHON)

#------------------------------------------------------------------------------
# locates module.cmake files under the current source directory and registers
# them as modules. All identified modules are treated as enabled and are built.
macro(pv_process_modules)
  if (VTK_WRAP_PYTHON)
    # this is needed to ensure that the PYTHON_INCLUDE_DIRS variable is set when
    # we process the plugins.
    find_package(PythonLibs)
  endif()

  unset (VTK_MODULES_ALL)
  file(GLOB_RECURSE files RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/module.cmake")
  foreach (module_cmake IN LISTS files)
    get_filename_component(base "${module_cmake}" PATH)
    vtk_add_module(
      "${CMAKE_CURRENT_SOURCE_DIR}/${base}"
      module.cmake
      "${CMAKE_CURRENT_BINARY_DIR}/${base}"
      Cxx)
  endforeach()

  set (current_module_set ${VTK_MODULES_ALL})
  list(APPEND VTK_MODULES_ENABLED ${VTK_MODULES_ALL})

  # sort the modules based on depedencies. This will endup bringing in
  # VTK-modules too. We raise errors if required VTK modules are not already
  # enabled.
  include(TopologicalSort)
  topological_sort(VTK_MODULES_ALL "" _DEPENDS)

  set (current_module_set_sorted)
  foreach(module IN LISTS VTK_MODULES_ALL)
    list(FIND current_module_set ${module} _found)
    if (_found EQUAL -1)
      # this is a VTK module and must have already been enabled. Otherwise raise
      # error.
      list(FIND VTK_MODULES_ENABLED ${module} _found)
      if (_found EQUAL -1)
        message(FATAL_ERROR
          "Requested modules not available: ${module}")
      endif()
    else ()
      list(APPEND current_module_set_sorted ${module})
    endif ()
  endforeach()

  set (plugin_cs_modules)
  foreach(_module IN LISTS current_module_set_sorted)
    if (NOT ${_module}_IS_TEST)
      set(vtk-module ${_module})
    else()
      set(vtk-module ${${_module}_TESTS_FOR})
    endif()
    add_subdirectory("${${_module}_SOURCE_DIR}" "${${_module}_BINARY_DIR}")
    if (NOT ${_module}_EXCLUDE_FROM_WRAPPING AND
        NOT ${_module}_IS_TEST AND
        NOT ${_module}_THIRD_PARTY)
        set(NO_PYTHON_BINDINGS_AVAILABLE TRUE)
        vtk_add_cs_wrapping(${_module})
        list(APPEND plugin_cs_modules ${_module})
    endif()
    unset(vtk-module)
  endforeach()

  # save the modules so any new plugins added, we can automatically make them
  # depend on these new modules.
  set (${pv-plugin}_CS_MODULES ${plugin_cs_modules})

  unset (VTK_MODULES_ALL)
  unset (current_module_set)
  unset (current_module_set_sorted)
  unset (plugin_cs_modules)
endmacro()

# this macro is used to setup the environment for loading/building VTK modules
# within ParaView plugins. This is only needed when building plugins outside of
# ParaVIew's source tree.
macro(pv_setup_module_environment _name)
  # Setup enviroment to build VTK modules outside of VTK source tree.
  set (BUILD_SHARED_LIBS ${VTK_BUILD_SHARED_LIBS})

  if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  endif()
  if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  endif()
  if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  endif()

  set (VTK_INSTALL_RUNTIME_DIR "bin")
  set (VTK_INSTALL_LIBRARY_DIR "lib")
  set (VTK_INSTALL_ARCHIVE_DIR "lib")
  set (VTK_INSTALL_INCLUDE_DIR "include")
  set (VTK_INSTALL_PACKAGE_DIR "lib/cmake/${_name}")

  if (NOT VTK_FOUND)
    set (VTK_FOUND ${ParaView_FOUND})
  endif()
  if (VTK_FOUND)
    set (VTK_VERSION
      "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")
  endif()

  include(vtkExternalModuleMacros)
  include(vtkClientServerWrapping)
  if (PARAVIEW_ENABLE_PYTHON)
    include(vtkPythonWrapping)
  endif()

  # load information about existing modules.
  foreach (mod IN LISTS VTK_MODULES_ENABLED)
    vtk_module_load("${mod}")
  endforeach()

  # Set this so that we can track all the modules we're building for this
  # plugin. add_paraview_plugin() call will then add logic to automatically link
  # and (do CS init) for all modules that are built for the plugin. Note
  # pv_setup_module_environment() is not called for plugin being built as part
  # of the ParaView build, in that case pv-plugin is set when processing the
  # plugin.cmake file, and hence this logic still works!
  set (pv-plugin "${_name}")
endmacro()
