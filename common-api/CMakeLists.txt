
#INSTALL(FILES codegen/org.genivi.commonapi.someip/src/SomeIPSpecification.fdepl
#  DESTINATION include/franca_idls
#)

add_subdirectory (include)
add_subdirectory (src)

if(NOT DISABLE_CODE_GEN)
	add_subdirectory (codegen)
endif()

set(CMAKE_MODULES_DESTINATION_DIR lib/cmake)

install(FILES CommonAPISomeIPCodeGenConfig.cmake DESTINATION ${CMAKE_MODULES_DESTINATION_DIR}/CommonAPISomeIPCodeGen)
