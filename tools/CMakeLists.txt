
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

add_executable( someip_ctl
ctl.cpp
)

TARGET_LINK_LIBRARIES( someip_ctl
   ${COMMONAPI_LIBRARIES}
   ${GLIB_LIBRARIES}
   someip_client_lib
)

INSTALL(TARGETS someip_ctl
  DESTINATION bin
)
