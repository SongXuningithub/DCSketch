CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

project(Dist_Estimation)

add_executable(test_MLE ${CMAKE_CURRENT_SOURCE_DIR}/src/Bayesian_Estimation.cpp)

target_link_libraries(
    test_MLE
    PUBLIC
    hashfunc
)
