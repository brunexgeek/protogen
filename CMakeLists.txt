project(webster)

cmake_minimum_required(VERSION 2.8)

set(PROTOGEN_MAJOR 0)
set(PROTOGEN_MINOR 6)
set(PROTOGEN_PATCH 0)

add_definitions(-DPROTOGEN_MAJOR=${PROTOGEN_MAJOR})
add_definitions(-DPROTOGEN_MINOR=${PROTOGEN_MINOR})
add_definitions(-DPROTOGEN_PATCH=${PROTOGEN_PATCH})
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

file(REMOVE "library/cpp/base-auto.cc")
fileToSource("library/cpp/base1.cc" "library/cpp/base-auto.cc" "BASE_1_TEMPLATE")
fileToSource("library/cpp/base2.cc" "library/cpp/base-auto.cc" "BASE_2_TEMPLATE")

add_library(libprotogen_static STATIC
    "library/proto3.cc"
    "library/printer.cc"
    "library/exception.cc"
    "library/cpp/base-auto.cc"
    "library/cpp/cppgen.cc" )
target_include_directories(libprotogen_static
    PUBLIC "include/")
set_target_properties(libprotogen_static PROPERTIES
    OUTPUT_NAME "protogen_static"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    PREFIX "lib" )

add_executable(protogen
    "compiler/main.cc")
target_link_libraries(protogen libprotogen_static)
set_target_properties(protogen PROPERTIES
    OUTPUT_NAME "protogen"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )