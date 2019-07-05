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

#include "concurrency/SearchableObjectHolder.hpp"

using namespace gmlc::concurrency;

/** test basic operations */
TEST(SOH_tests, basic_tests)
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
