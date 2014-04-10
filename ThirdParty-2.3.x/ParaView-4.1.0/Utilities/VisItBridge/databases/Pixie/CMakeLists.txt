
SET(SOURCES
avtPixieFileFormat.h
avtPixieFileFormat.C
)

ADD_VISIT_READER(VisItPixieReader "1.0"  
  VISIT_READER_TYPE "MTSD"
  VISIT_READER_NAME "avtPixieFileFormat"    
  SERVER_SOURCES ${SOURCES}  
  )
