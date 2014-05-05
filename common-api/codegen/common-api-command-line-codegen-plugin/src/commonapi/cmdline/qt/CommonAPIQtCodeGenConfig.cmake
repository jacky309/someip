
if(Qt5Qml_DEFINITIONS)
	set(ENABLE_QML_EXTENSION 1)
		message("Qt Qml detected")
	add_definitions(${COMMONAPI_QT_CFLAGS})
endif()

set(QT_MODULES_LOCATION lib/qt5/qml)

find_package(Qt5Qml)
add_definitions(${Qt5Qml_DEFINITIONS})

find_package(Qt5Quick)
add_definitions(${Qt5Quick_DEFINITIONS})

find_package(Qt5Script)
add_definitions(${Qt5Script_DEFINITIONS})

pkg_check_modules(COMMON_API_QT REQUIRED CommonAPI-Qt)
add_definitions(${COMMON_API_QT_CFLAGS})

find_package(CommonAPICodeGen)

set(CMAKE_AUTOMOC ON)


pkg_check_modules(COMMONAPI_QT CommonAPI-Qt)

# Generates a QML plugin which registers a proxy singleton for the given interface 
macro(add_commonapi_qml_plugin interface)

	message("QML plugin configured for " ${interface} " / CMAKE_FIND_ROOT_PATH: " ${CMAKE_FIND_ROOT_PATH})

	get_library_name(GENERATED_LIBRARY_NAME ${interface})

	# We use a variable name which contains the name of the interface in order to avoid clashes when the macro is called multiple times. TOTO : investigate why using the same variable does not work
	FIND_FILE(FRANCA_DEPLOYMENT_FILE_LOCATION${GENERATED_LIBRARY_NAME} ${interface}.fdepl PATHS "${CMAKE_INSTALL_PREFIX}/include/franca_idls")
	FIND_FILE(FRANCA_FILE_LOCATION${GENERATED_LIBRARY_NAME} ${interface}.fidl PATHS "${CMAKE_INSTALL_PREFIX}/include/franca_idls")

	message("FRANCA_FILE_LOCATION: ${interface} ${FRANCA_DEPLOYMENT_FILE_LOCATION${GENERATED_LIBRARY_NAME}}")

##	commonapi_create_artifact_targets(${FRANCA_DEPLOYMENT_FILE_LOCATION${GENERATED_LIBRARY_NAME}} ${FRANCA_FILE_LOCATION${GENERATED_LIBRARY_NAME}} ${interface})

	GET_FILENAME_COMPONENT(interfaceName ${interface} NAME )
	GET_FILENAME_COMPONENT(NAMESPACE_PATH ${interface} PATH )

	add_commonapi_proxy(COMMON_API_PROXY ${interface})

	set(PLUGIN_LIBRARY_NAME ${interfaceName})

	set(GENERATED_FILES
			${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QtProxy.h
			${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QtProxy.cpp
#			${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QtProxymoc.cpp
#			${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}moc.cpp
			${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QMLPlugin.cpp
#			${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QMLPluginmoc.cpp
	)

	message("${GENERATED_FILES} - ${FRANCA_DEPLOYMENT_FILE_LOCATION} - ${FRANCA_FILE_LOCATION} - qt")

	add_generated_files_command("${GENERATED_FILES}" ${FRANCA_DEPLOYMENT_FILE_LOCATION${GENERATED_LIBRARY_NAME}} ${FRANCA_FILE_LOCATION${GENERATED_LIBRARY_NAME}} qt)

	qt5_generate_moc(${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QtProxy.h
		${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QtProxymoc.cpp)

	qt5_generate_moc(${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QMLPlugin.cpp 
		${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QMLPluginmoc.cpp)

	# It seems like the only way to get the moc files properly generated is to add them as dependencies here
	set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QMLPlugin.cpp
 		APPEND PROPERTY OBJECT_DEPENDS 
 		${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QMLPluginmoc.cpp
 	)

	set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QtProxy.cpp
 		APPEND PROPERTY OBJECT_DEPENDS 
 		${CMAKE_CURRENT_BINARY_DIR}/${COMMONAPI_GENERATED_FILES_LOCATION}/${interface}QtProxymoc.cpp
 	)

	add_library(${PLUGIN_LIBRARY_NAME} SHARED
		${GENERATED_FILES}
	)

	qt5_use_modules(${PLUGIN_LIBRARY_NAME} Qml)
	qt5_use_modules(${PLUGIN_LIBRARY_NAME} Script)

	TARGET_LINK_LIBRARIES( ${PLUGIN_LIBRARY_NAME}
		${COMMON_API_PROXY_LIBRARIES}
		${COMMON_API_QT_LIBRARIES}
	)

	# Make sure the linker does NOT remove the dependencies when no symbol is directly needed
	set_target_properties(${PLUGIN_LIBRARY_NAME} PROPERTIES LINK_FLAGS "-Wl,--no-as-needed")

	install(TARGETS ${PLUGIN_LIBRARY_NAME} DESTINATION ${QT_MODULES_LOCATION}/${NAMESPACE_PATH})

endmacro()
