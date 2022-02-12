cmake_minimum_required(VERSION 3.20)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)

project(GoeCharger)

include(FetchContent)
FetchContent_Declare(
    ObserverModel
    GIT_REPOSITORY https://github.com/HomeMaintenance/ObserverModel.git
    GIT_TAG main
)
FetchContent_MakeAvailable(ObserverModel)

find_package(CURL CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

add_library(
    Goe
    GoeCharger.h
    GoeCharger.cpp
    PowerRange.h
    PowerRange.cpp
)
target_include_directories(Goe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Goe PRIVATE CURL::libcurl)
target_link_libraries(Goe PUBLIC jsoncpp_lib jsoncpp_object JsonCpp::JsonCpp)
target_compile_definitions(Goe PRIVATE GOE_DEBUG=1)

add_executable(test test.cpp)
target_link_libraries(test PRIVATE Goe)
