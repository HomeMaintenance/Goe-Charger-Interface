add_executable(Goe_test test.cpp)
target_link_libraries(Goe_test PUBLIC Goe)


find_package(GTest CONFIG REQUIRED)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_executable(GoeCharger_test GoeCharger_test.cpp)
target_link_libraries(GoeCharger_test PUBLIC Goe)
target_link_libraries(GoeCharger_test PUBLIC GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(GoeCharger_test)
