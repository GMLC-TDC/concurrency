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
#include <utility>
/** these test cases test TriggerVariables
 */

#include "concurrency/DelayedObjects.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace gmlc::concurrency;

/** test basic operations */
TEST(DelayedObjects_tests, basic_tests)
{
    DelayedObjects<std::string> objs;

    auto fut1 = objs.getFuture("string1");
    auto fut2 = objs.getFuture(45);

    objs.setDelayedValue("string1", "string num1");

    auto str1 = fut1.get();
    EXPECT_EQ(str1, "string num1");

    objs.setDelayedValue(45, "string2");
    auto str2 = fut2.get();
    EXPECT_EQ(str2, "string2");

    objs.finishedWithValue("string1");
}

/** test basic operations */
TEST(DelayedObjects_tests, all_fulfill)
{
    DelayedObjects<int> objs;
    auto fut1 = objs.getFuture("t1");
    auto fut2 = objs.getFuture("t2");

    auto fut3 = objs.getFuture(45);
    auto fut4 = objs.getFuture(55);

    objs.fulfillAllPromises(19);
    EXPECT_EQ(fut1.get(), 19);
    EXPECT_EQ(fut2.get(), 19);
    EXPECT_EQ(fut3.get(), 19);
    EXPECT_EQ(fut4.get(), 19);
}
