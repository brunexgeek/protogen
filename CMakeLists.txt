project(webster)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors -pedantic -std=c++11 -Wl,--no-undefined -fPIC -Wall -Wextra -Wconversion -Werror=return-type -Werror=implicit-function-declaration")

add_library(libprotogen_static STATIC "library/proto3.cc")
target_include_directories(libprotogen_static
    PUBLIC "include/")

add_executable(protogen "compiler/main.cc" "compiler/json.cc")
target_link_libraries(protogen libprotogen_static)
