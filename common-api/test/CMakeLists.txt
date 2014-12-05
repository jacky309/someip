include_directories(../include)
add_gtest_test(common_api_tests "commonAPITests.cpp" "-Wl,--no-as-needed;CommonAPI-SomeIP;-Wl,--as-needed;pthread")
