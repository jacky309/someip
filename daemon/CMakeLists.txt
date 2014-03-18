
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

add_executable( someip_dispatcher
	main.cpp
	Dispatcher.cpp
	LocalServer.cpp
	RemoteServiceListener.cpp
	ServiceAnnouncer.cpp
	TCPManager.cpp
	TCPClient.cpp
	Client.cpp
	sd-daemon.c
)

TARGET_LINK_LIBRARIES( someip_dispatcher
	${COMMONAPI_LIBRARIES}
	${GLIB_LIBRARIES}
	${GLIB_DBUS_LIBRARIES}
	someip_lib
)

INSTALL(TARGETS someip_dispatcher
	DESTINATION bin
)

# systemd files
if (ENABLE_SYSTEMD)
	message("install systemd files")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/someip_dispatcher.service.in  ${CMAKE_CURRENT_BINARY_DIR}/someip_dispatcher.service)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/someip_dispatcher.socket.in  ${CMAKE_CURRENT_BINARY_DIR}/someip_dispatcher.socket)
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/someip_dispatcher.service DESTINATION ${SYSTEMD_CONFIGURATION_FILES_DIR})
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/someip_dispatcher.socket DESTINATION ${SYSTEMD_CONFIGURATION_FILES_DIR})
endif()
