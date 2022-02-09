CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

project(DCS)

add_library(
    ${PROJECT_NAME}
    SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dcs.cpp
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    hashfunc
    utility
)

add_executable(
    test_DCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/test_dcs.cpp
)

target_link_libraries(
    test_DCS
    PUBLIC
    ${PROJECT_NAME}
    mylibpcap
)