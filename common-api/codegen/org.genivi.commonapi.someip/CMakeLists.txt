
set(GENERATED_OUTPUT_JAR ${CMAKE_CURRENT_SOURCE_DIR}/target/org.genivi.commonapi.someip-1.0.0.jar)

#set(OUTPUT_JAR someip.jar)

#message("Parent :" ${CMAKE_INSTALL_PREFIX}/share/CommonAPICodeGen/codegen.pom.xml)
#add_custom_command(
#	OUTPUT ${GENERATED_OUTPUT_JAR}
#	COMMAND mvn -Dcodegen.core.location=${CMAKE_INSTALL_PREFIX}/share/CommonAPICodeGen/codegen.pom.xml -f ${CMAKE_CURRENT_SOURCE_DIR}/pom.xml install
#	DEPENDS pom.xml
#)

add_custom_target(
	codegen ALL
	COMMAND mvn ${MAVEN_OPTIONS} -f ${CMAKE_CURRENT_SOURCE_DIR}/pom.xml install
	DEPENDS pom.xml
)

# Install our generator
install(FILES ${GENERATED_OUTPUT_JAR} DESTINATION ${JAR_DESTINATION_PATH})

# Install dependencies
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/target/dependencies/ DESTINATION ${JAR_DESTINATION_PATH})
