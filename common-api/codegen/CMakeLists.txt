cmake_minimum_required(VERSION 2.6)

project(SomeIP-Codegen)

set(CMAKE_MODULES_DESTINATION_DIR lib/cmake)

set(FRANCA_VERSION "0.8.9.201308271211")
set(FRANCA_PACKAGE_URL "https://franca.eclipselabs.org.codespot.com/files/site_franca_${FRANCA_VERSION}.zip")

set(FRANCA_PATH ${CMAKE_CURRENT_BINARY_DIR}/site_franca)
set(GENERATED_OUTPUT_JAR ${CMAKE_CURRENT_SOURCE_DIR}/common-api-command-line-codegen-plugin/target/common-api-command-line-codegen-plugin-1.0.0.jar)
set(OUTPUT_JAR common-api-command-line-codegen.jar)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/site_franca_${FRANCA_VERSION}.zip
	COMMAND wget ${FRANCA_PACKAGE_URL} --no-check-certificate
)

add_custom_command(
	OUTPUT ${FRANCA_PATH}/site.xml
	COMMAND unzip -o site_franca_${FRANCA_VERSION}.zip -d ${FRANCA_PATH} 
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/site_franca_${FRANCA_VERSION}.zip
)

add_custom_command(
	OUTPUT ${GENERATED_OUTPUT_JAR}
	COMMAND mvn -Dfranca.update.site.location=${FRANCA_PATH} -f ${CMAKE_CURRENT_SOURCE_DIR}/pom.xml install
	DEPENDS ${FRANCA_PATH}/site.xml
)

add_custom_target(
	${OUTPUT_JAR} ALL
	COMMAND cp ${GENERATED_OUTPUT_JAR} ${OUTPUT_JAR}
	DEPENDS ${GENERATED_OUTPUT_JAR}
)

set(JAR_DESTINATION_PATH share/commonAPI)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_JAR} DESTINATION ${JAR_DESTINATION_PATH})

install(FILES CommonAPICodeGenConfig.cmake DESTINATION ${CMAKE_MODULES_DESTINATION_DIR}/CommonAPICodeGen)

configure_file(commonAPICodeGen.in commonAPICodeGen @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/commonAPICodeGen DESTINATION bin
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
