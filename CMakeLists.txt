project(webster)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic --extra -Wall -Wfatal-errors")

add_executable(protogen "library/proto3.cc")
target_include_directories(protogen
    PUBLIC "include/")