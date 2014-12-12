
cmake_minimum_required(VERSION 2.6)

set(PACKAGE SomeIP-Codegen)
PROJECT(${PACKAGE})

find_package(CommonAPICodeGenFramework)
import_maven_repository(common-api-tools)

set(GENERATED_OUTPUT_JAR ${CMAKE_CURRENT_SOURCE_DIR}/org.genivi.commonapi.someip/target/org.genivi.commonapi.someip-1.0.0.jar)

add_maven_package(qt-codegen ${CMAKE_CURRENT_SOURCE_DIR}/org.genivi.commonapi.someip/pom.xml "")

# Install our generator 
install(FILES ${GENERATED_OUTPUT_JAR} DESTINATION ${COMMON_API_CODEGEN_JAR_DESTINATION_PATH})

# Install dependencies
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/org.genivi.commonapi.someip/target/dependencies/ DESTINATION ${COMMON_API_CODEGEN_JAR_DESTINATION_PATH})

