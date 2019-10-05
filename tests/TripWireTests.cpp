/*
Copyright (c) 2017-2019,
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

#include "gtest/gtest.h"
#include <iostream>

#include "concurrency/TripWire.hpp"

using namespace gmlc::concurrency;

TEST(tripwire, basic)
{
    auto line = make_tripline();
    auto trig = std::make_shared<TripWireTrigger>(line);
    TripWireDetector detect(line);

    EXPECT_FALSE(detect.isTripped());
    trig = nullptr;
    EXPECT_TRUE(detect.isTripped());
}

DECLARE_INDEXED_TRIPLINES(10)

TEST(tripwire, indexed)
{
    TripWireDetector detect(5);

    EXPECT_FALSE(detect.isTripped());

    EXPECT_THROW(TripWireDetector(12), std::out_of_range);
    // smaller scope for trigger
    {
        TripWireTrigger trig(5);
        EXPECT_FALSE(detect.isTripped());
    }
    EXPECT_TRUE(detect.isTripped());
}

DECLARE_TRIPLINE()

TEST(tripwire, declared)
{
    TripWireDetector detect;

    EXPECT_FALSE(detect.isTripped());

    {
        TripWireTrigger trig{};
        EXPECT_FALSE(detect.isTripped());
    }
    EXPECT_TRUE(detect.isTripped());
}
