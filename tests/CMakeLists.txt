if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/googletest/CMakeLists.txt")
    message(FATAL_ERROR "You have requested tests be built, but googletest is not downloaded. Please run:
    git submodule update --init")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/config/cmake")


set(GOOGLE_TEST_INDIVIDUAL OFF)
include(AddGoogletest)

set(CONCURRENCY_TESTS
    TriggerVariableTests
    )

# Only affects current directory, so safe
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

foreach(T ${CONCURRENCY_TESTS})

    add_executable(${T} ${T}.cpp )
    target_link_libraries(${T} PUBLIC CONCURRENCY)
    add_gtest(${T})
	set_target_properties(${T} PROPERTIES FOLDER tests)
   
endforeach()

if(CMAKE_BUILD_TYPE STREQUAL Coverage)
    include(CodeCoverage)
    setup_target_for_coverage(
        NAME CONCURRENCY_coverage
        EXECUTABLE ctest
        DEPENDENCIES
          ${CONCURRENCY_TESTS}
          )
endif()

