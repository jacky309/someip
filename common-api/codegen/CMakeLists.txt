
cmake_minimum_required(VERSION 2.6)

set(PACKAGE SomeIP-Codegen)
PROJECT(${PACKAGE})

set(JAR_DESTINATION_PATH share/CommonAPICodeGen)

add_subdirectory(org.genivi.commonapi.someip)

