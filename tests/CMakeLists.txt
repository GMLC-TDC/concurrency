if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/googletest/CMakeLists.txt")
    message(FATAL_ERROR "You have requested tests be built, but googletest is not downloaded. Please run:
    git submodule update --init")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/config/cmake")


set(GOOGLE_TEST_INDIVIDUAL OFF)
include(AddGoogletest)

set(CONCURRENCY_TEST_SOURCES
    TriggerVariableTests.cpp
	SearchableObjectHolderTests.cpp
	TripWireTests.cpp
    )

add_executable(concurrencyTests ${CONCURRENCY_TEST_SOURCES})
target_link_libraries(concurrencyTests PUBLIC concurrency)
add_gtest(concurrencyTests)
set_target_properties(concurrencyTests PROPERTIES FOLDER tests)
   

add_subdirectory(libguarded)

if(CMAKE_BUILD_TYPE STREQUAL Coverage)
    include(CodeCoverage)
    setup_target_for_coverage(
        NAME CONCURRENCY_coverage
        EXECUTABLE ctest
        DEPENDENCIES
          ${CONCURRENCY_TESTS}
          )
endif()

