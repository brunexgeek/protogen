project(webster)

cmake_minimum_required(VERSION 2.8)

set(PROTOGEN_MAJOR 0)
set(PROTOGEN_MINOR 3)
set(PROTOGEN_PATCH 0)

add_definitions(-DPROTOGEN_VERSION="${PROTOGEN_MAJOR}.${PROTOGEN_MINOR}.${PROTOGEN_PATCH}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors -pedantic -std=c++11 -Wl,--no-undefined -fPIC -Wall -Wextra -Wconversion -Werror=return-type -Werror=implicit-function-declaration")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DBUILD_DEBUG)
else()
    set(CMAKE_BUILD_TYPE "Release")
    add_definitions(-DBUILD_RELEASE)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

function(fileToSource inputFileName outputFileName variableName)
    FILE(READ "${inputFileName}" CONTENT)
    STRING(REPLACE "\\" "\\\\" CONTENT "${CONTENT}")
    STRING(REPLACE "\"" "\\\"" CONTENT "${CONTENT}")
    STRING(REPLACE "\n" "\\n\"\n\"" CONTENT "${CONTENT}")
    FILE(APPEND "${outputFileName}" "const char *${variableName} = \"")
    FILE(APPEND "${outputFileName}" "${CONTENT}")
    FILE(APPEND "${outputFileName}" "\";\n\n")
endfunction(fileToSource)

file(REMOVE "library/base-auto.cc")
fileToSource("library/base.cc" "library/base-auto.cc" "BASE_TEMPLATE")

add_library(libprotogen_static STATIC
    "library/proto3.cc"
    "library/printer.cc"
    "library/exception.cc"
    "library/base-auto.cc"
    "library/cppgen.cc" )
target_include_directories(libprotogen_static
    PUBLIC "include/")

add_executable(protogen
    "compiler/main.cc")
target_link_libraries(protogen libprotogen_static)
