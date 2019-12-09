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

#include "concurrency/TriggerVariable.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace gmlc::concurrency;

/** test basic operations */
TEST(triggervariable_tests, basic_tests)
{
    TriggerVariable trigger;

    EXPECT_FALSE(trigger.isActive());
    EXPECT_FALSE(trigger.isTriggered());
    // trigger is not active so can't be triggered
    EXPECT_FALSE(trigger.trigger());
    EXPECT_TRUE(trigger.activate());
    // second activate should be false;
    EXPECT_FALSE(trigger.activate());
    EXPECT_TRUE(trigger.isActive());
    EXPECT_FALSE(trigger.isTriggered());

    EXPECT_TRUE(trigger.trigger());
    EXPECT_TRUE(trigger.isActive());
    EXPECT_TRUE(trigger.isTriggered());
}

/** test basic operations */
TEST(triggervariable_tests, waitActivation)
{
    TriggerVariable trigger;
    std::atomic<bool> started{false};
    std::atomic<bool> completed{false};
    auto fut = std::async(std::launch::async, [&]() {
        started = true;
        trigger.waitActivation();
        completed = true;
    });

    std::this_thread::yield();
    while (!started.load()) {
        std::this_thread::yield();
    }
    EXPECT_TRUE(started.load());
    EXPECT_FALSE(completed.load());
    trigger.activate();
    std::this_thread::yield();
    if (!completed.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_TRUE(completed.load());
}

/** test basic operations */
TEST(triggervariable_tests, waitTrigger)
{
    TriggerVariable trigger;
    trigger.activate();
    std::atomic<bool> started{false};
    std::atomic<bool> completed{false};
    auto fut = std::async(std::launch::async, [&]() {
        started = true;
        trigger.wait();
        completed = true;
    });

    std::this_thread::yield();
    while (!started.load()) {
        std::this_thread::yield();
    }
    EXPECT_TRUE(started.load());

    EXPECT_FALSE(completed.load());
    trigger.trigger();
    std::this_thread::yield();
    if (!completed.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_TRUE(completed.load());
}

TEST(triggervariable_tests, reset1)
{
    TriggerVariable trigger(true);

    EXPECT_TRUE(trigger.isActive());
    EXPECT_TRUE(trigger.trigger());

    trigger.reset();

    EXPECT_FALSE(trigger.isActive());
    EXPECT_TRUE(trigger.isTriggered());

    EXPECT_TRUE(trigger.activate());
    EXPECT_FALSE(trigger.isTriggered());
    EXPECT_TRUE(trigger.isActive());
}
