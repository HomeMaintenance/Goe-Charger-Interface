cmake_minimum_required(VERSION 3.20)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)

project(GoeCharger)

add_subdirectory(test)

include(FetchContent)
FetchContent_Declare(
    ObserverModel
    GIT_REPOSITORY https://github.com/HomeMaintenance/ObserverModel.git
    GIT_TAG main
)
FetchContent_Declare(
    PowerManagement
    GIT_REPOSITORY https://github.com/HomeMaintenance/PowerManagement.git
    GIT_TAG main
)
FetchContent_Declare(
    mqtt_interface
    GIT_REPOSITORY https://github.com/HomeMaintenance/Mqtt.git
    GIT_TAG main
)
FetchContent_MakeAvailable(ObserverModel PowerManagement mqtt_interface)

find_package(CURL CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

add_library(
    Goe
    GoeCharger.h
    GoeCharger.cpp
    Utils/Cache.h
)
target_include_directories(Goe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Goe PRIVATE
    CURL::libcurl)
target_link_libraries(Goe PUBLIC
    jsoncpp_lib
    jsoncpp_object
    JsonCpp::JsonCpp
    PowerManagement
    mqtt)
target_compile_definitions(Goe PRIVATE GOE_DEBUG=0)

if(PROJECT_IS_TOP_LEVEL)
    find_package(Doxygen
        REQUIRED dot
        OPTIONAL_COMPONENTS mscgen dia
    )
    set(DOXYGEN_EXTRACT_PRIVATE TRUE)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED FALSE)
    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C TRUE)
    set(DOXYGEN_EXCLUDE_PATTERNS */build/*)

    doxygen_add_docs(doxygen_target
        ${CMAKE_SOURCE_DIR}
        COMMENT "Run doxygen_target"
    )
endif()
