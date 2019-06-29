/*
Copyright © 2017-2019,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include <future>
#include <memory>
#include <string>
#include <thread>
#include <utility>
/** these test cases test data_block and data_view objects
 */

#include "gtest/gtest.h"
#include <iostream>

#include "AirLock.hpp"

using namespace gmlc::containers;

/** test basic operations */
TEST (airlock_tests, basic_tests)
{
    AirLock<int> alock;

    EXPECT_TRUE (alock.try_load (45));
    EXPECT_TRUE (!alock.try_load (54));

    EXPECT_TRUE (alock.isLoaded ());
    auto res = alock.try_unload ();
    ASSERT_TRUE (res);

    EXPECT_EQ (*res, 45);
    EXPECT_TRUE (!alock.isLoaded ());
    EXPECT_TRUE (alock.try_load (54));
}

/** test with a move only element*/
TEST (airlock_tests, move_only_tests)
{
    AirLock<std::unique_ptr<double>> alock;

    alock.try_load (std::make_unique<double> (4534.23));

    EXPECT_TRUE (alock.isLoaded ());

    auto b = alock.try_unload ();
    EXPECT_EQ (**b, 4534.23);

    b = alock.try_unload ();
    EXPECT_TRUE (!(b));
    EXPECT_TRUE (!alock.isLoaded ());
}

/** multi-thread test*/
TEST (airlock_tests, move_mthread_tests)
{
    AirLock<std::string> alock;

    alock.try_load ("load 1");

    EXPECT_TRUE (alock.isLoaded ());

    auto fut =
      std::async (std::launch::async, [&alock]() { alock.load ("load 2"); });
    auto fut2 =
      std::async (std::launch::async, [&alock]() { alock.load ("load 2"); });
    std::this_thread::yield ();
    auto b = alock.try_unload ();
    EXPECT_EQ (*b, "load 1");
    int chk = 0;
    while (!alock.isLoaded ())
    {
        std::this_thread::sleep_for (std::chrono::milliseconds (100));
        if (chk++ > 10)
        {
            break;
        }
    }
    b = alock.try_unload ();
    ASSERT_TRUE (b);
    EXPECT_EQ (*b, "load 2");
    fut.get ();
    fut2.get ();
    EXPECT_TRUE (alock.isLoaded ());
}
