cmake_minimum_required(VERSION 3.20)
project(stress_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(stress_test stress.cpp)
target_link_libraries(stress_test pthread)
