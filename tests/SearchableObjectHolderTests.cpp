/*
Copyright (c) 2017-2023,
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

#include "concurrency/SearchableObjectHolder.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace gmlc::concurrency;

/** test basic operations */
TEST(SOH, basic)
{
    SearchableObjectHolder<std::string> SOH1;
    SOH1.addObject("test1", std::make_shared<std::string>("test_1"));

    auto res = SOH1.findObject("test1");
    ASSERT_TRUE(res);
    EXPECT_EQ(*res, "test_1");

    SOH1.removeObject("test1");
    auto res2 = SOH1.findObject("test1");
    EXPECT_FALSE(res2);
}

TEST(SOH, contained)
{
    SearchableObjectHolder<std::string> SOH1;
    EXPECT_TRUE(SOH1.empty());
    SOH1.addObject("test1", std::make_shared<std::string>("test_1"));
    SOH1.addObject("test2", std::make_shared<std::string>("test_2"));
    SOH1.addObject("test3", std::make_shared<std::string>("test_3"));

    auto objects = SOH1.getObjects();
    ASSERT_EQ(objects.size(), 3U);
    EXPECT_EQ(*(objects[0]), "test_1");
    EXPECT_EQ(*(objects[1]), "test_2");
    EXPECT_EQ(*(objects[2]), "test_3");
    objects.clear();
}

TEST(SOH, containedType)
{
    SearchableObjectHolder<std::string, char> SOH1;
    EXPECT_TRUE(SOH1.empty());
    SOH1.addObject("test1", std::make_shared<std::string>("test_1"), '1');
    SOH1.addObject("test2", std::make_shared<std::string>("test_2"), '2');
    SOH1.addObject("test3", std::make_shared<std::string>("test_3"), '3');

    auto objects = SOH1.getObjects();
    ASSERT_EQ(objects.size(), 3U);
    EXPECT_EQ(*(objects[0]), "test_1");
    EXPECT_EQ(*(objects[1]), "test_2");
    EXPECT_EQ(*(objects[2]), "test_3");

    EXPECT_TRUE(SOH1.checkObjectType("test1", '1'));

    objects.clear();
}
