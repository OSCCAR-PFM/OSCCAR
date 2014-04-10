

SET(SOURCES
avtSiloWriter.C
avtSiloFileFormat.C
avtSiloFileFormat.h
avtSiloOptions.C
avtSiloOptions.h
SiloPluginInfo.C
SiloPluginInfo.h
SiloCommonPluginInfo.C
SiloEnginePluginInfo.C
SiloMDServerPluginInfo.C
)
if(HAVE_LIBSILO)
  ADD_VISIT_READER(VisItSiloReader "1.0"
    VISIT_READER_TYPE "STMD"
    VISIT_READER_NAME "avtSiloFileFormat"
    VISIT_READER_USES_OPTIONS ON
    SERVER_SOURCES ${SOURCES}
    )
endif(HAVE_LIBSILO)

