
INCLUDE_DIRECTORIES( 
	${CMAKE_CURRENT_SOURCE_DIR} 
	${CMAKE_CURRENT_BINARY_DIR}
)

set(LIBRARY_FILES
)

INCLUDE (CheckIncludeFiles)

CHECK_INCLUDE_FILES (boost/variant.hpp HAVE_BOOST_H)
#IF(NOT HAVE_BOOST_H)
#  message( FATAL_ERROR "Boost development package is not found" )
#ENDIF()

add_library( someip_lib SHARED
	ipc/UDS.cpp
	ipc/IPC.cpp
	Message.cpp
	lib.cpp
	Client.cpp
	Dispatcher.cpp
	TCPManager.cpp
	TCPClient.cpp
	TCPServer.cpp
	RemoteServiceListener.cpp
	ServiceAnnouncer.cpp
)

message("LOGGING_LIBRARIES : ${LOGGING_LIBRARIES}")
 
TARGET_LINK_LIBRARIES( someip_lib 
   utilLib
   ${COMMONAPI_LIBRARIES}
   ${GLIB_LIBRARIES}
   ${CPP_LIBS}
   ${LOGGING_LIBRARIES}
)

set_target_properties(someip_lib PROPERTIES VERSION ${VERSION} SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION})

INSTALL(TARGETS someip_lib
  DESTINATION lib
)


set(INCLUDE_FILES
	SomeIP-log.h
	SomeIP-common.h
	SomeIP.h
	SomeIP-Serialization.h
	Message.h
	ipc.h
	SocketStreamConnection.h
)

install(FILES ${INCLUDE_FILES} DESTINATION ${PUBLIC_HEADERS_LOCATION})

install(FILES ipc/UDSConnection.h DESTINATION ${PUBLIC_HEADERS_LOCATION}/ipc)

configure_file(SomeIP-Config.h.in SomeIP-Config.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SomeIP-Config.h DESTINATION ${PUBLIC_HEADERS_LOCATION})
