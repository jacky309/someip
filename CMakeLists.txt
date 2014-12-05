cmake_minimum_required(VERSION 2.6)

set(PACKAGE SomeIP)
PROJECT(${PACKAGE})

SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 10 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 1 )

set(VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_LEVEL})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
if (USE_LIBCPP)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	set(CPP_LIBS c++)
endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wextra")

add_definitions(-DCOMMONAPI_INTERNAL_COMPILATION)

find_package(IVILogging REQUIRED)
set(LOGGING_LIBRARIES ${IVILOGGING_LIBRARIES})

# Dependencies
FIND_PACKAGE(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
add_definitions(${GLIB_CFLAGS})

pkg_check_modules(COMMONAPI REQUIRED CommonAPI)
add_definitions(${COMMONAPI_CFLAGS_OTHER})
include_directories(${COMMONAPI_INCLUDE_DIRS})

include_directories(
	utilLib
	lib
	${CMAKE_CURRENT_BINARY_DIR}/lib
	ipc
	clientLib
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(
	${COMMONAPI_LIBRARY_DIRS}
	${GLIB_LIBRARY_DIRS}
)

configure_file(CommonAPI-SomeIP.pc.in CommonAPI-SomeIP.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CommonAPI-SomeIP.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)

set(DEVELOPMENT_INCLUDE_PATH " -I${CMAKE_CURRENT_BINARY_DIR}/lib -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/clientLib -I${CMAKE_CURRENT_SOURCE_DIR}/utilLib -I${CMAKE_CURRENT_SOURCE_DIR}/lib -I${CMAKE_CURRENT_SOURCE_DIR}/common-api/include #")
set(DEVELOPMENT_LIBRARY_PATH " -L${CMAKE_CURRENT_BINARY_DIR}/clientLib -L${CMAKE_CURRENT_BINARY_DIR}/lib #" )
configure_file(CommonAPI-SomeIP.pc.in CommonAPI-SomeIP-uninstalled.pc @ONLY)
if(INSTALL_PKGCONFIG_UNINSTALLED_FILE)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CommonAPI-SomeIP-uninstalled.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
endif()

if (ENABLE_SYSTEMD)
	add_definitions(-DENABLE_SYSTEMD)
	pkg_check_modules(GLIB_DBUS REQUIRED dbus-glib-1)
	add_definitions(${GLIB_DBUS_CFLAGS})
	IF(${CMAKE_INSTALL_PREFIX} STREQUAL "/usr")
		set(SYSTEMD_CONFIGURATION_FILES_DIR "/lib/systemd/system")
	ELSE()
		set(SYSTEMD_CONFIGURATION_FILES_DIR "systemd/system")
	ENDIF()
endif()

set(PUBLIC_HEADERS_LOCATION include/someip/lib)

add_subdirectory (daemon)
add_subdirectory (tools)
add_subdirectory (lib)
add_subdirectory (clientLib)
add_subdirectory (utilLib)

OPTION( ENABLE_TEST "Enable testing" OFF)

add_subdirectory (common-api)

if (ENABLE_TEST)
	
	include( CTest )
	
	macro(add_gtest_test testName files libraries)
	
		if (NOT DEFINED GTEST_DIR)
			set(GTEST_DIR $ENV{GTEST_DIR})
		endif()
	
		if (DEFINED GTEST_DIR)
		
			message("Google test found in ${GTEST_DIR}")
		
			include_directories(${GTEST_DIR}/include)
			link_directories(${GTEST_DIR}/lib/.libs)
		
			add_executable(${testName}
				${files}
			)
		
			target_link_libraries(${testName} 
				gtest
				${libraries}
			)
		
			INSTALL(TARGETS ${testName}
			  DESTINATION bin/test
			)
		
			add_test(
			    NAME ${testName}
			    COMMAND ${testName}
			)
	
		else()
			message("Google test not found")
		endif()
	
	endmacro()

    enable_testing()
	add_subdirectory(test)
	add_subdirectory(common-api/test)
endif()

if (ENABLE_DOC)
	add_subdirectory (doc)
endif()

if (ENABLE_EXAMPLES)
endif()
