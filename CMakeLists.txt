cmake_minimum_required(VERSION 3.20)

project(gconfig VERSION 0.1.0 LANGUAGES CXX)

option(GCONFIG_STRICT "Enable strict compiler warnings" ON)
option(GCONFIG_WARN_AS_ERROR "Treat warnings as errors" OFF)
option(GCONFIG_BUILD_EXAMPLES "Build example programs" ON)
option(GCONFIG_BUILD_TESTS "Build test programs" ON)
set(GCONFIG_GSEXP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../gsexp"
    CACHE PATH "Path to a gsexp source tree when gsexp::gsexp is not already available")

if(NOT TARGET gsexp::gsexp)
    if(EXISTS "${GCONFIG_GSEXP_SOURCE_DIR}/CMakeLists.txt")
        set(GSEXP_BUILD_EXAMPLES OFF CACHE BOOL "Build gsexp examples" FORCE)
        set(GSEXP_BUILD_TESTS OFF CACHE BOOL "Build gsexp tests" FORCE)
        set(GSEXP_BUILD_BENCHMARKS OFF CACHE BOOL "Build gsexp benchmarks" FORCE)
        add_subdirectory("${GCONFIG_GSEXP_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/gsexp")
    else()
        message(FATAL_ERROR
            "gsexp::gsexp target was not found and GCONFIG_GSEXP_SOURCE_DIR does not point "
            "to a gsexp source tree: ${GCONFIG_GSEXP_SOURCE_DIR}")
    endif()
endif()

add_library(gconfig
    src/document.cpp
    src/document_list.cpp
    src/io.cpp
    src/schema.cpp
    src/value.cpp
)

add_library(gconfig::gconfig ALIAS gconfig)

target_include_directories(gconfig
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(gconfig PUBLIC cxx_std_20)
target_link_libraries(gconfig PUBLIC gsexp::gsexp)

function(gconfig_apply_warnings target_name)
    if(NOT GCONFIG_STRICT)
        return()
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
        if(GCONFIG_WARN_AS_ERROR)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
        )
        if(GCONFIG_WARN_AS_ERROR)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

gconfig_apply_warnings(gconfig)

if(GCONFIG_BUILD_EXAMPLES)
    add_executable(gconfig_demo examples/gconfig_demo.cpp)
    target_link_libraries(gconfig_demo PRIVATE gconfig::gconfig)
    target_compile_features(gconfig_demo PRIVATE cxx_std_20)
    gconfig_apply_warnings(gconfig_demo)
endif()

if(GCONFIG_BUILD_TESTS)
    enable_testing()

    add_executable(gconfig_tests tests/gconfig_tests.cpp)
    target_link_libraries(gconfig_tests PRIVATE gconfig::gconfig)
    target_compile_features(gconfig_tests PRIVATE cxx_std_20)
    gconfig_apply_warnings(gconfig_tests)

    add_test(NAME gconfig_tests COMMAND gconfig_tests)
endif()

