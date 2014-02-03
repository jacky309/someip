cmake_minimum_required(VERSION 2.6)

set(PACKAGE SomeIP)
PROJECT(${PACKAGE})

SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 2 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 0 )

set(VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_LEVEL})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
if (USE_LIBCPP)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	set(CPP_LIBS c++)
endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wextra")

add_definitions(-DCOMMONAPI_INTERNAL_COMPILATION)

# Dependencies
FIND_PACKAGE(PkgConfig REQUIRED)

find_package(PelagicoreUtilities)

pkg_check_modules(GLIB REQUIRED glib-2.0)
add_definitions(${GLIB_CFLAGS})

pkg_check_modules(PELAGICORE_UTILS REQUIRED pelagicore-utils)
add_definitions(${PELAGICORE_UTILS_CFLAGS})

pkg_check_modules(COMMONAPI REQUIRED CommonAPI)
add_definitions(${COMMONAPI_CFLAGS})

include_directories(
	utilLib
	lib
	${CMAKE_CURRENT_BINARY_DIR}/lib
	ipc
	clientLib
)

include_directories(
	utilLib/include
)

link_directories(
	${COMMONAPI_LIBRARY_DIRS}
	${GLIB_LIBRARY_DIRS}
)

configure_file(CommonAPI-SomeIP.pc.in CommonAPI-SomeIP.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CommonAPI-SomeIP.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)

if (ENABLE_SYSTEMD)
	add_definitions(-DENABLE_SYSTEMD)
	pkg_check_modules(GLIB_DBUS REQUIRED dbus-glib-1)
	add_definitions(${GLIB_DBUS_CFLAGS})
	set(SYSTEMD_CONFIGURATION_FILES_DIR "/lib/systemd/system")
endif()

add_subdirectory (daemon)
add_subdirectory (tools)
add_subdirectory (lib)
add_subdirectory (clientLib)

add_subdirectory (doc)

add_subdirectory (common-api/include)
add_subdirectory (common-api/src)

add_subdirectory (test)

if (ENABLE_EXAMPLES)
endif()
