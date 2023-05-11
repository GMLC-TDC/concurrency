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
#include <utility>
/** these test cases test data_block and data_view objects
 */

#include "concurrency/DelayedDestructor.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace gmlc::concurrency;

/** test basic operations */
TEST(DelayedDestr, basic)
{
    DelayedDestructor<std::string> DD1;
    DD1.addObjectsToBeDestroyed(std::make_shared<std::string>("test_1"));

    EXPECT_EQ(DD1.size(),1U);

    DD1.destroyObjects();
    EXPECT_EQ(DD1.size(),0U);
}


TEST(DelayedDestrSS, basic)
{
    DelayedDestructorSingleThread<std::string> DD1;
    DD1.addObjectsToBeDestroyed(std::make_shared<std::string>("test_1"));

    EXPECT_EQ(DD1.size(),1U);

    DD1.destroyObjects();
    EXPECT_EQ(DD1.size(),0U);
}
