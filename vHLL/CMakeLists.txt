cmake_minimum_required(VERSION 3.20)

project(vHLL)

add_library(
    ${PROJECT_NAME}
    SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vHLL.cpp
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
    test_vHLL
    ${CMAKE_CURRENT_SOURCE_DIR}/src/test_vHLL.cpp
)

target_link_libraries(
    test_vHLL
    PUBLIC
    ${PROJECT_NAME}
    mylibpcap
)