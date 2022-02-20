add_subdirectory(test)

add_library(server STATIC server_global.h Server.h Server.cpp)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(server PRIVATE SERVER_LIB=0)

find_path(CPP_HTTPLIB_INCLUDE_DIRS "httplib.h")
target_include_directories(server PRIVATE ${CPP_HTTPLIB_INCLUDE_DIRS})

target_link_libraries(server PUBLIC Goe)
