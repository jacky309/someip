
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

set(LIBRARY_FILES
)

add_library( someip_client_lib SHARED
SomeIP-clientLib.cpp
)

TARGET_LINK_LIBRARIES( someip_client_lib 
   ${COMMONAPI_LIBRARIES}
   ${GLIB_LIBRARIES}
   someip_lib
)

set_target_properties(someip_client_lib PROPERTIES VERSION ${VERSION} SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION})

INSTALL(TARGETS someip_client_lib
  DESTINATION lib
)

set(INCLUDE_FILES
SomeIP-clientLib.h
)

install(FILES ${INCLUDE_FILES} DESTINATION include/someip)
