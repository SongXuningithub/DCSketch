cmake_minimum_required(VERSION 3.20)

project(calc_accuracy)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/test_acc.cpp)

target_link_libraries(
    calc_accuracy
    PUBLIC
    utility
)