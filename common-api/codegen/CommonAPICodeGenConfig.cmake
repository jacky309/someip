
set(COMMONAPI_CODEGEN_COMMAND_LINE commonAPICodeGen)
set(COMMONAPI_GENERATED_FILES_LOCATION FrancaGen)

find_package(PkgConfig REQUIRED)

find_package(CommonAPISomeIPCodeGen)
find_package(CommonAPIDBusCodeGen)

pkg_check_modules(COMMON_API REQUIRED CommonAPI)
add_definitions(${COMMON_API_CFLAGS})
link_directories(${COMMON_API_LIBRARY_DIRS})

macro(get_library_name variableName interface)
	set(LIBRARY_NAME ${interface}_CommonAPIGenerated)
	STRING(REGEX REPLACE "/" "_" LIBRARY_NAME ${LIBRARY_NAME})
	set ( ${variableName} ${LIBRARY_NAME})
	message ("Library name : ${${variableName}} ")
endmacro()

#add_definitions("-DCOMMONAPI_INTERNAL_COMPILATION")


macro(add_generated_files_command GENERATED_FILES deploymentFile idlFile codegenerator)
	message("Command : ${COMMONAPI_CODEGEN_COMMAND_LINE} -f ${deploymentFile} -o ${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION} ${codegenerator}")
	add_custom_command(
		OUTPUT ${GENERATED_FILES}
		COMMAND ${COMMONAPI_CODEGEN_COMMAND_LINE} -f ${deploymentFile} -o ${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION} ${codegenerator}
		MAIN_DEPENDENCY ${deploymentFile} ${idlFile}
	)
	include_directories(${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION})
endmacro()


set(FRANCA_IDLS_LOCATION ${CMAKE_INSTALL_PREFIX}/include/franca_idls)
set(SERVICE_HEADERS_LOCATION ${CMAKE_INSTALL_PREFIX}/include/CommonAPIServices/FrancaGen)

macro(install_franca_idl interfaceName deploymentFile deploymentFileDestinationName idlFile)
	install(FILES ${idlFile} DESTINATION ${FRANCA_IDLS_LOCATION}/${interfaceName}/.. )

	message("configure file ${deploymentFile} ${CMAKE_CURRENT_BINARY_DIR}/${deploymentFileDestinationName}")
	configure_file(${deploymentFile} ${CMAKE_CURRENT_BINARY_DIR}/${deploymentFileDestinationName} @ONLY)
	install( FILES ${CMAKE_CURRENT_BINARY_DIR}/${deploymentFileDestinationName} DESTINATION ${FRANCA_IDLS_LOCATION}/${interfaceName}/.. )

endmacro()


# Create an empty backend library for client to link against, without having to know which actual backend is going to be used.
# This empty library links against the backend library
macro(install_backend_link_________ backendLibrary backendLink)

	# We need to create an empty file to make CMake happy
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp "// empty")

	add_library(${backendLink} SHARED
		${CMAKE_CURRENT_BINARY_DIR}/empty.cpp
	)

	set_target_properties(${backendLink} PROPERTIES VERSION 1 SOVERSION 1)

	set_target_properties(${backendLink}
      PROPERTIES
      LINK_INTERFACE_LIBRARIES ${backendLibrary}
  	)

  	install(TARGETS ${backendLink} DESTINATION lib)

	TARGET_LINK_LIBRARIES( ${backendLink}
		${backendLibrary}
	)

endmacro()


# Generates and installs a library containing a CommonAPI stub and a proxy for the given interface
# By default, the DBUS backend is used unless the "DEFAULT_COMMONAPI_BACKEND" variable is set to "someip"
macro(add_commonapi_service variableName deploymentFilePrefix idlFile interface)

	get_library_name(BASE___ ${interface})

	set(BACKEND dbus)

	if(${DEFAULT_COMMONAPI_BACKEND} MATCHES "someip")
		set(BACKEND someip)
	endif()

	message("Using ${BACKEND} backend")
	set(deploymentFile ${deploymentFilePrefix}-${BACKEND}.fdepl)
	set(${variableName}_LIBRARIES ${${variableName}_LIBRARIES} ${BASE___}_${BACKEND})

	if(${BACKEND} MATCHES "someip")
		install_commonapi_someip_backend(${BASE___} ${variableName} ${deploymentFile} ${idlFile} ${interface})
	else()
		install_commonapi_dbus_backend(${BASE___} ${variableName} ${deploymentFile} ${idlFile} ${interface})
	endif()

	install_franca_idl(${interface} ${deploymentFile} ${deploymentFilePrefix}.fdepl ${idlFile})

endmacro()


# Generates a CommonAPI proxy for the given interface
macro(add_commonapi_proxy variableName interface)

	get_library_name(BASE___ ${interface})
	set(${variableName}_LIBRARIES ${BASE___}_Backend)

	include_directories(${SERVICE_HEADERS_LOCATION})

#	if(ENABLE_QML_EXTENSION)
#		install( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/ DESTINATION ${QT_MODULES_LOCATION})
#	endif()

endmacro()

