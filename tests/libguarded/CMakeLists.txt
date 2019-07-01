set(LIBGUARDED_TESTS
    cow_guarded
	deferred_guarded
	guarded
	lr_guarded
	ordered_guarded
	rcu_guarded
	shared_guarded
    )

# Only affects current directory, so safe
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

foreach(T ${LIBGUARDED_TESTS})

    add_executable(${T} ${T}Tests.cpp )
    target_link_libraries(${T} PUBLIC concurrency)
    add_gtest(${T})
	set_target_properties(${T} PROPERTIES FOLDER tests)
   
endforeach()


