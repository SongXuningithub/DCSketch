cmake_minimum_required(VERSION 3.20)

project(DCSketch)


# DCSketch lib
add_library(
    DCSketch
    SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DCSketch.cpp
)

target_include_directories(
    DCSketch
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    DCSketch
    PUBLIC
    hashfunc
    utility
)

# test executable
add_executable(test_DCSketch ${CMAKE_CURRENT_SOURCE_DIR}/src/test_DCSketch.cpp)

target_link_libraries(
    test_DCSketch
    PRIVATE
    DCSketch
    mylibpcap
)

add_executable(ParamSetting ${CMAKE_CURRENT_SOURCE_DIR}/src/ParameterSetting.cpp)

target_link_libraries(
    ParamSetting
    PRIVATE
    DCSketch
    mylibpcap
)