cmake_minimum_required(VERSION 3.16)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" IEUM_VERSION LIMIT_COUNT 1)
string(STRIP "${IEUM_VERSION}" IEUM_VERSION)
if(NOT IEUM_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain a semantic version such as 0.1.0")
endif()
set_property(DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
)

project(ieum
    VERSION ${IEUM_VERSION}
    DESCRIPTION "Structural dependency checker for Ieum source files"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(IEUM_HEADERS
    src/token.h
    src/ast.h
    src/lexer.h
    src/parser.h
    src/checker.h
    src/version.h
)

function(add_ieum_executable target source)
    add_executable(${target} ${source} ${IEUM_HEADERS})
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(${target} PRIVATE IEUM_VERSION="${PROJECT_VERSION}")

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
    endif()
endfunction()

add_ieum_executable(ieum src/main.cpp)
add_ieum_executable(testParser test/testParser.cpp)
add_ieum_executable(testPipeline test/testPipeline.cpp)
add_ieum_executable(testChecker test/testChecker.cpp)
add_ieum_executable(benchmarkChecker benchmark/benchmarkChecker.cpp)

if(MSVC)
    target_compile_options(benchmarkChecker PRIVATE /O2)
else()
    target_compile_options(benchmarkChecker PRIVATE -O2)
endif()

enable_testing()

add_test(NAME parser COMMAND testParser)
add_test(NAME pipeline COMMAND testPipeline)
add_test(NAME checker COMMAND testChecker)
add_test(NAME benchmark_smoke COMMAND benchmarkChecker 2 1)
set_tests_properties(benchmark_smoke PROPERTIES
    PASS_REGULAR_EXPRESSION "scenario=layered-valid-chain"
)

add_test(NAME cli_version COMMAND $<TARGET_FILE:ieum> --version)
set_tests_properties(cli_version PROPERTIES
    PASS_REGULAR_EXPRESSION "ieum ${PROJECT_VERSION}"
)

function(add_ieum_example_test name expected_exit expected_text)
    add_test(
        NAME example_${name}
        COMMAND ${CMAKE_COMMAND}
            "-DIEUM_EXECUTABLE=$<TARGET_FILE:ieum>"
            "-DIEUM_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/${name}.ieum"
            "-DEXPECTED_EXIT=${expected_exit}"
            "-DEXPECTED_TEXT=${expected_text}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyExample.cmake"
    )
endfunction()

add_ieum_example_test(valid 0 "")
add_ieum_example_test(module_body 0 "body_modules=2")
add_ieum_example_test(implicit_dependency 1 "notification")
add_ieum_example_test(cyclic_dependency 1 "order -> payment -> order")
add_ieum_example_test(layer_violation 1 "'data'")
add_ieum_example_test(transitive_layer_violation 1 "'data'")
add_ieum_example_test(invalid_declarations 1 "'missing'")
