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

#include "concurrency/Latch.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace gmlc::concurrency;

TEST(latch, basic)
{
    Latch b1(2);
    std::atomic<int> ct{0};
    auto f1 = std::async(std::launch::async, [&b1, &ct]() {
        b1.wait();
        ++ct;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(ct.load(), 0);
    auto f2 = std::async(std::launch::async, [&b1, &ct]() {
        b1.wait();
        ++ct;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(ct.load(), 0);
    b1.arrive();
    b1.arrive();
    f2.get();
    f1.get();
    EXPECT_EQ(ct.load(), 2);
}
