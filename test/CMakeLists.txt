INCLUDE_DIRECTORIES(../common-api/include)

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

add_gtest_test(someip_test_offline "offlineTests.cpp" CommonAPI-SomeIP)
add_gtest_test(someip_test_online "onlineTests.cpp" CommonAPI-SomeIP)
