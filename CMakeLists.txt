# ~~~
# Copyright (c) 2017-2023, Battelle Memorial Institute; Lawrence Livermore
# National Security, LLC; Alliance for Sustainable Energy, LLC.
# See the top-level NOTICE for additional details.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
# ~~~

cmake_minimum_required(VERSION 3.11...3.25)

project(GMLC_CONCURRENCY VERSION 0.4.0)

# -----------------------------------------------------------------------------
# GMLC CONCURRENCY library Version number
# -----------------------------------------------------------------------------
set(GMLC_CONCURRENCY_VERSION_BUILD)
set(GMLC_CONCURRENCY_DATE "2023-05-11")

set(GMLC_CONCURRENCY_VERSION_STRING
    "${GMLC_CONCURRENCY_VERSION} (${GMLC_CONCURRENCY_DATE})"
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # User settable
    set(GMLC_CONCURRENCY_CXX_STD "17" CACHE STRING "The CMake standard to require")

    # set(CUR_PROJ ON)
    set(CMAKE_CXX_STANDARD ${GMLC_CONCURRENCY_CXX_STD})
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
# -----------------------------------------------------------------------------
# set the module path and include some common macros
# -----------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/config/cmake/"
                      "${PROJECT_SOURCE_DIR}/ThirdParty/cmake/"
)
include(extraMacros)
include(CMakeDependentOption)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Prohibit in-source build
if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
    message(
        FATAL_ERROR
            "In-source build is not supported. Please, use an empty directory for building the project."
    )
endif()

add_library(concurrency_base INTERFACE)

include(compiler_flags)
target_link_libraries(concurrency_base INTERFACE compile_flags_target)

# -------------------------------------------------------------
# add threading support
# -------------------------------------------------------------
if(MSYS OR CYGWIN)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()
find_package(Threads QUIET)

if(TARGET Threads::Threads)
    target_link_libraries(concurrency_base INTERFACE Threads::Threads)
endif()

target_include_directories(concurrency_base INTERFACE ThirdParty)

cmake_dependent_option(
    GMLC_CONCURRENCY_CLANG_TIDY "Look for and use Clang-Tidy" OFF
    "CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME" OFF
)
set(GMLC_CONCURRENCY_CLANG_TIDY_OPTIONS ""
    CACHE STRING "Clang tidy options, such as -fix, semicolon separated"
)

mark_as_advanced(GMLC_CONCURRENCY_CLANG_TIDY_OPTIONS)
mark_as_advanced(GMLC_CONCURRENCY_CLANG_TIDY)

# Prepare Clang-Tidy
if(GMLC_CONCURRENCY_CLANG_TIDY)
    find_program(
        CLANG_TIDY_EXE NAMES "clang-tidy" DOC "Path to clang-tidy executable" REQUIRED
    )

    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" ${GMLC_CONCURRENCY_CLANG_TIDY_OPTIONS})
endif()

add_subdirectory(gmlc)

option(GMLC_CONCURRENCY_TEST "Enable tests for the concurrency library" ON)
# option(GMLC_CONCURRENCY_BENCHMARK "Enable benchmarks for the concurrency library" ON)

if(GMLC_CONCURRENCY_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()

if(GMLC_CONCURRENCY_BENCHMARK)
    add_subdirectory(benchmarks)
endif()

# -------------------------------------------------------------
# Get some configuration for C++17 as that becomes available
# -------------------------------------------------------------
# message(STATUS ${CMAKE_CXX_FLAGS})

option(GMLC_CONCURRENCY_GENERATE_DOXYGEN_DOC "Generate Doxygen doc target" OFF)

if(GMLC_CONCURRENCY_GENERATE_DOXYGEN_DOC)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)

        show_variable(
            DOXYGEN_OUTPUT_DIR PATH "location to put Doxygen docs"
            "${PROJECT_BINARY_DIR}/docs"
        )
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/config/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
        )
        add_custom_target(
            gmlc_concurrency_doc
            ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${DOXYGET_OUTPUT_DIR}
            COMMENT "Generating GMLC CONCURRENCY documentation with Doxygen"
            VERBATIM
        )
    endif(DOXYGEN_FOUND)
endif(GMLC_CONCURRENCY_GENERATE_DOXYGEN_DOC)
