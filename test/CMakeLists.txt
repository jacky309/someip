INCLUDE_DIRECTORIES(../common-api/include)

add_gtest_test(someip_test_offline "offlineTests.cpp" CommonAPI-SomeIP)
add_gtest_test(someip_test_online "onlineTests.cpp" CommonAPI-SomeIP)
add_gtest_test(someip_test_daemonLess "onlineDaemonLessTests.cpp" CommonAPI-SomeIP)
