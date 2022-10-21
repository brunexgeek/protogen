project(protogen)

cmake_minimum_required(VERSION 3.18)

set(PROTOGEN_MAJOR 2)
set(PROTOGEN_MINOR 1)
set(PROTOGEN_PATCH 0)

add_definitions(-DPROTOGEN_MAJOR=${PROTOGEN_MAJOR})
add_definitions(-DPROTOGEN_MINOR=${PROTOGEN_MINOR})
add_definitions(-DPROTOGEN_PATCH=${PROTOGEN_PATCH})
add_definitions(-DPROTOGEN_VERSION="${PROTOGEN_MAJOR}.${PROTOGEN_MINOR}.${PROTOGEN_PATCH}")

if (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors -pedantic -std=c++11 -Wl,--no-undefined -fPIC -Wall -Wextra -Wconversion -Wmaybe-uninitialized -Werror=return-type")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

if (MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DBUILD_DEBUG)
else()
    set(CMAKE_BUILD_TYPE "Release")
    add_definitions(-DBUILD_RELEASE)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

add_library(libprotogen_static STATIC
    "compiler/proto3.cc"
    "compiler/printer.cc"
    "compiler/exception.cc"
    "compiler/cpp/cppgen.cc" )
target_include_directories(libprotogen_static
    PUBLIC "include/"
    PRIVATE "${CMAKE_BINARY_DIR}/__include/")
add_dependencies(libprotogen_static process_template)
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

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/__include/")
add_custom_target(process_template
    "${CMAKE_BINARY_DIR}/template" "${CMAKE_CURRENT_LIST_DIR}/compiler/cpp/code.txt" "${CMAKE_BINARY_DIR}/__include/auto-code.hh"
    DEPENDS template)

add_executable(template "compiler/template.cc")
set_target_properties(template PROPERTIES
    OUTPUT_NAME "template"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

set(PROTOGEN_EXEC "${CMAKE_BINARY_DIR}/protogen")

add_custom_target(generate_test1
    "${PROTOGEN_EXEC}" "${CMAKE_CURRENT_LIST_DIR}/tests/test1.proto" "${CMAKE_BINARY_DIR}/__include/test1.pg.hh"
    DEPENDS protogen)

add_custom_target(generate_test2
    "${PROTOGEN_EXEC}" "${CMAKE_CURRENT_LIST_DIR}/tests/test2.proto" "${CMAKE_BINARY_DIR}/__include/test2.pg.hh"
    DEPENDS protogen)

add_custom_target(generate_test3
    "${PROTOGEN_EXEC}" "${CMAKE_CURRENT_LIST_DIR}/tests/test3.proto" "${CMAKE_BINARY_DIR}/__include/test3.pg.hh"
    DEPENDS protogen)

add_custom_target(generate_test7
    "${PROTOGEN_EXEC}" "${CMAKE_CURRENT_LIST_DIR}/tests/test7.proto" "${CMAKE_BINARY_DIR}/__include/test7.pg.hh"
    DEPENDS protogen)


add_executable(tests "tests/tests.cc")
target_include_directories(tests
    PUBLIC "include/"
    PRIVATE "${CMAKE_BINARY_DIR}/__include/")
add_dependencies(tests generate_test1 generate_test7 generate_test3)
set_target_properties(tests PROPERTIES
    OUTPUT_NAME "run-tests"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

install(TARGETS protogen DESTINATION bin)
