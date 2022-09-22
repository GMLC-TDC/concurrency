# ~~~
# Copyright (c) 2017-2019, Battelle Memorial Institute; Lawrence Livermore
# National Security, LLC; Alliance for Sustainable Energy, LLC.
# See the top-level NOTICE for additional details.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
# ~~~

set(LIBGUARDED_TEST_SOURCES
    cow_guardedTests.cpp
	deferred_guardedTests.cpp
	guardedTests.cpp
	guarded_optTests.cpp
	lr_guardedTests.cpp
	ordered_guardedTests.cpp
	rcu_guardedTests.cpp
	shared_guardedTests.cpp
	shared_guarded_optTests.cpp
	atomic_guardedTests.cpp
    )


add_executable(libguarded_test ${LIBGUARDED_TEST_SOURCES} )
target_link_libraries(libguarded_test PUBLIC concurrency)
add_gtest(libguarded_test )
set_target_properties(libguarded_test PROPERTIES FOLDER tests)




