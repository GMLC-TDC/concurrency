/***********************************************************************
 *
 * Copyright (c) 2015-2017 Ansel Sermersheim
 * All rights reserved.
 *
 * This file is part of libguarded
 *
 * libguarded is free software, released under the BSD 2-Clause license.
 * For license details refer to LICENSE provided with this project.
 *
 ***********************************************************************/

/*
Copyright © 2017-2019,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
/*
modified to use google test
*/
#include "gtest/gtest.h"

#include "libguarded/atomic_guarded.hpp"

#include <atomic>
#include <string>
#include <thread>

using namespace gmlc::libguarded;

TEST(atomic_guarded, atomci_guarded_1)
{
    atomic_guarded<std::string> data("the key string");

    std::string val(data);

    EXPECT_EQ(val, "the key string");

    data.store("another string");
    EXPECT_EQ(data.load(), "another string");

    data = "yet another string";
    EXPECT_EQ(data.load(), "yet another string");

    EXPECT_EQ(data.exchange("string b"), "yet another string");

    std::string val2{"string2"};
    EXPECT_FALSE(data.compare_exchange(val2, "string 8"));
    EXPECT_EQ(val2, "string b");
    EXPECT_TRUE(data.compare_exchange(val2, "string 8"));
}
