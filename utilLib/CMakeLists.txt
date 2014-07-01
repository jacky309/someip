
set(INCLUDE_FILES
common.h
serialization.h
MainLoopApplication.h
GlibIO.h
CommandLineParser.h
)

add_library( utilLib STATIC
	lib.cpp
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"  )

TARGET_LINK_LIBRARIES( utilLib 
   ${GLIB_LIBRARIES}
   ${DLT_LIBRARIES}
   ${CPP_LIBS}
   ${LOGGING_LIBRARIES}
)

install(FILES ${INCLUDE_FILES} DESTINATION ${PUBLIC_HEADERS_LOCATION}/utilLib)
