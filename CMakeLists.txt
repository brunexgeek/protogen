project(protogen)

cmake_minimum_required(VERSION 3.18)

# This variables affect everything in the compiler and generated code
set(PROTOGEN_MAJOR 3)
set(PROTOGEN_MINOR 0)
set(PROTOGEN_PATCH 0)

configure_file("source/cmake.hh.in" "__include/cmake.hh")

if (UNIX)
    set(ENABLE_SANITIZER OFF CACHE BOOL "")
    if (ENABLE_SANITIZER)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fsanitize=address")
    endif()

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
    "source/proto3.cc"
    "source/printer.cc"
    "source/exception.cc"
    "source/cpp/cppgen.cc" )
target_include_directories(libprotogen_static
    PUBLIC "include/"
    PRIVATE "${CMAKE_BINARY_DIR}/__include/")
add_dependencies(libprotogen_static process_template)
set_target_properties(libprotogen_static PROPERTIES
    OUTPUT_NAME "protogen_static"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    PREFIX "lib" )

add_executable(protogen
    "source/main.cc")
target_include_directories(protogen
    PRIVATE "${CMAKE_BINARY_DIR}/__include/")
target_link_libraries(protogen libprotogen_static)
set_target_properties(protogen PROPERTIES
    OUTPUT_NAME "protogen"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/__include/auto-code.hh"
    DEPENDS template "${CMAKE_CURRENT_LIST_DIR}/source/cpp/code.txt"
    COMMAND "${CMAKE_BINARY_DIR}/template" "${CMAKE_CURRENT_LIST_DIR}/source/cpp/code.txt" "${CMAKE_BINARY_DIR}/__include/auto-code.hh"
)

add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/__include/auto-protogen.hh"
    DEPENDS template "${CMAKE_CURRENT_LIST_DIR}/source/cpp/protogen.hh"
    COMMAND "${CMAKE_BINARY_DIR}/template" "${CMAKE_CURRENT_LIST_DIR}/source/cpp/protogen.hh" "${CMAKE_BINARY_DIR}/__include/auto-protogen.hh"
)

add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/__include/auto-json.hh"
    DEPENDS template "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json.hh"
    COMMAND "${CMAKE_BINARY_DIR}/template" "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json.hh" "${CMAKE_BINARY_DIR}/__include/auto-json.hh"
)
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/__include/auto-json-array.hh"
    DEPENDS template "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json-array.hh"
    COMMAND "${CMAKE_BINARY_DIR}/template" "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json-array.hh" "${CMAKE_BINARY_DIR}/__include/auto-json-array.hh"
)
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/__include/auto-json-base64.hh"
    DEPENDS template "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json-base64.hh"
    COMMAND "${CMAKE_BINARY_DIR}/template" "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json-base64.hh" "${CMAKE_BINARY_DIR}/__include/auto-json-base64.hh"
)
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/__include/auto-json-number.hh"
    DEPENDS template "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json-number.hh"
    COMMAND "${CMAKE_BINARY_DIR}/template" "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json-number.hh" "${CMAKE_BINARY_DIR}/__include/auto-json-number.hh"
)
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/__include/auto-json-string.hh"
    DEPENDS template "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json-string.hh"
    COMMAND "${CMAKE_BINARY_DIR}/template" "${CMAKE_CURRENT_LIST_DIR}/source/cpp/json-string.hh" "${CMAKE_BINARY_DIR}/__include/auto-json-string.hh"
)

add_custom_target(process_template
    DEPENDS
        "${CMAKE_BINARY_DIR}/__include/auto-code.hh"
        "${CMAKE_BINARY_DIR}/__include/auto-protogen.hh"
        "${CMAKE_BINARY_DIR}/__include/auto-json.hh"
        "${CMAKE_BINARY_DIR}/__include/auto-json-array.hh"
        "${CMAKE_BINARY_DIR}/__include/auto-json-base64.hh"
        "${CMAKE_BINARY_DIR}/__include/auto-json-number.hh"
        "${CMAKE_BINARY_DIR}/__include/auto-json-string.hh"
)

add_executable(template "source/template.cc")
target_include_directories(template
    PRIVATE "${CMAKE_BINARY_DIR}/__include/")
set_target_properties(template PROPERTIES
    OUTPUT_NAME "template"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

set(PROTOGEN_EXEC "${CMAKE_BINARY_DIR}/protogen")

add_custom_target(generate_test1
    "${PROTOGEN_EXEC}" "${CMAKE_CURRENT_LIST_DIR}/tests/test1.proto" "${CMAKE_BINARY_DIR}/__include/test1.pg.hh"
    DEPENDS protogen process_template)

add_custom_target(generate_test2
    "${PROTOGEN_EXEC}" "${CMAKE_CURRENT_LIST_DIR}/tests/test2.proto" "${CMAKE_BINARY_DIR}/__include/test2.pg.hh"
    DEPENDS protogen process_template)

add_custom_target(generate_test3
    "${PROTOGEN_EXEC}" "${CMAKE_CURRENT_LIST_DIR}/tests/test3.proto" "${CMAKE_BINARY_DIR}/__include/test3.pg.hh"
    DEPENDS protogen process_template)

add_custom_target(generate_test7
    "${PROTOGEN_EXEC}" "${CMAKE_CURRENT_LIST_DIR}/tests/test7.proto" "${CMAKE_BINARY_DIR}/__include/test7.pg.hh"
    DEPENDS protogen process_template)


add_executable(tests "tests/tests.cc")
target_include_directories(tests
    PUBLIC "include/"
    PRIVATE "${CMAKE_BINARY_DIR}/__include/")
add_dependencies(tests generate_test1 generate_test7 generate_test3)
set_target_properties(tests PROPERTIES
    OUTPUT_NAME "run-tests"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

install(TARGETS protogen DESTINATION bin)
