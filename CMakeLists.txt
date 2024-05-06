cmake_minimum_required(VERSION 3.20)
project(linux_network)

set(CMAKE_CXX_STANDARD 17)

IF (${build} MATCHES "debug")
    set(CMAKE_BUILD_TYPE DEBUG)
    set(CMAKE_CXX_FLAGS "-Wall -Wno-sign-compare -O0 -pthread -g") #-m64
    add_definitions(-DLINUX -DGCC -DDEBUG -DDEBUG_LOG)

ELSE(${build} MATCHES "release")
    set(CMAKE_BUILD_TYPE RELEASE)
    set(CMAKE_CXX_FLAGS "-Wall -Wno-sign-compare -O2 -pthread")  #-m64
    add_definitions(-DLINUX -DGCC)
ENDIF(${build} MATCHES "debug")

include_directories(linux_network linux_server linux_communicator)

add_executable(linux_network main.cpp linux_server/io_epoll.cc)

target_link_libraries(linux_network pthread rt)
