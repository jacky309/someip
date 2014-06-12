
#INSTALL(FILES codegen/org.genivi.commonapi.someip/src/SomeIPSpecification.fdepl
#  DESTINATION include/franca_idls
#)

add_subdirectory (include)
add_subdirectory (src)

if(NOT DISABLE_CODE_GEN)
	add_subdirectory (codegen)
endif()
