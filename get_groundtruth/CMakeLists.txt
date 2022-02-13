cmake_minimum_required(VERSION 3.20)

project(get_groundtruth)

add_executable(get_groundtruth ${CMAKE_CURRENT_SOURCE_DIR}/src/get_groundtruth.cpp)

target_link_libraries(
    get_groundtruth
    PRIVATE
    mylibpcap
    utility
)