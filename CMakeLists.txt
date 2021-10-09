cmake_minimum_required(VERSION 3.20)

project(DCSketch)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mylibpcap)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MurmurHash3)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/get_groundtruth)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/DCSketch)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/rerskt)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/calc_accuracy)