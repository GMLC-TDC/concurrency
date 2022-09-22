if(NOT EXISTS "${GMLC_CONTAINERS_SOURCE_DIR}/extern/benchmark/CMakeLists.txt")
    message(
        FATAL_ERROR
            "You have requested tests be built, but googlebenchmark is not downloaded. Please run:
    git submodule update --init"
    )
endif()

list(APPEND CMAKE_MODULE_PATH "${GMLC_CONTAINERS_SOURCE_DIR}/config/cmake")

include(AddGooglebenchmark)

set(CONTAINERS_BENCHMARKS CircularBufferBenchmarks SimpleQueueBenchmarks)

# Only affects current directory, so safe
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

foreach(T ${CONTAINERS_BENCHMARKS})

    add_executable(${T} ${T}.cpp)
    target_link_libraries(${T} PUBLIC containers)
    add_benchmark(${T})
    set_target_properties(${T} PROPERTIES FOLDER benchmarks)

endforeach()

# Add boost to test boost::optional if available
find_package(Boost 1.58)
if(Boost_FOUND)
    # target_link_libraries(informational PUBLIC Boost::boost)
    # target_link_libraries(OptionalTest PUBLIC Boost::boost)

    # Enforce Boost::Optional even if __has_include is missing on your compiler
    # target_compile_definitions(informational PUBLIC CLI11_BOOST_OPTIONAL)
    # target_compile_definitions(OptionalTest PUBLIC CLI11_BOOST_OPTIONAL)
endif()

# Adding this printout to CTest file(WRITE "${PROJECT_BINARY_DIR}/CTestCustom.cmake"
# "set(CTEST_CUSTOM_PRE_TEST \"${CMAKE_BINARY_DIR}/informational\")" )

# if(CMAKE_BUILD_TYPE STREQUAL Coverage) include(CodeCoverage)
# setup_target_for_coverage( NAME CLI11_coverage EXECUTABLE ctest DEPENDENCIES
# ${CLI11_TESTS} ${CLI11_MULTIONLY_TESTS}) endif()
