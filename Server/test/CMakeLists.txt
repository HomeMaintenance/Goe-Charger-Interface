add_executable(test_server testServer.cpp)
target_link_libraries(test_server PRIVATE server)
target_include_directories(test_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
