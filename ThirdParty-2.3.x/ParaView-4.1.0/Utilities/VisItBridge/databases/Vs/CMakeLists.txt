
set(SOURCES
  avtVsFileFormat.C
  avtVsFileFormat.h
  VsPluginInfo.h
  VsPluginInfo.C
  VsEnginePluginInfo.C
  VsMDServerPluginInfo.C
  VsCommonPluginInfo.C
  VsFilter.C
  VsH5Reader.C
  VsSchema.C
  VsUtils.C
)

ADD_VISIT_READER(VisItVsReader "1.0" 
  VISIT_READER_TYPE "STSD"
  VISIT_READER_NAME "avtVsFileFormat"     
  VISIT_READER_USES_OPTIONS ON
  SERVER_SOURCES ${SOURCES}
  )
