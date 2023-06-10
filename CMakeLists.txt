cmake_minimum_required(VERSION 3.15)
project(aspect)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-O0 -g -rdynamic")
set(CMAKE_C_FLAGS "-O0 -g -rdynamic")

add_executable(aspect main.cpp)
