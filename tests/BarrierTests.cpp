/*
Copyright (c) 2017-2022,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include <future>
#include <memory>
#include <string>
#include <thread>
/** these test cases test tripwire
 */

#include "concurrency/Barrier.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace gmlc::concurrency;

TEST(barrier, basic)
{
    Barrier barrier1(2);
    std::atomic<int> count{0};
    auto fut1 = std::async(std::launch::async, [&barrier1, &count]() {
        barrier1.wait();
        ++count;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(count.load(), 0);
    auto fut2 = std::async(std::launch::async, [&barrier1, &count]() {
        barrier1.wait();
        ++count;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(count.load(), 2);
    fut2.get();
    fut1.get();
}
