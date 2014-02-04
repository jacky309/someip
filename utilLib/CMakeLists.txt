
set(INCLUDE_FILES
common.h
SomeIP-log-null.h
serialization.h
MainLoopApplication.h
SomeIP-log.h
GlibIO.h
CommandLineParser.h
)

add_library( utilLib STATIC
	lib.cpp
)

TARGET_LINK_LIBRARIES( utilLib 
   ${GLIB_LIBRARIES}
   ${DLT_LIBRARIES}
   ${CPP_LIBS}
   pelagicore-logging
)

install(FILES ${INCLUDE_FILES} DESTINATION ${PUBLIC_HEADERS_LOCATION}/utilLib)
