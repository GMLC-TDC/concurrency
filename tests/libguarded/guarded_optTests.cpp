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
Copyright (c) 2017-2019,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
/*
modified to use google test
*/
#include "libguarded/guarded_opt.hpp"

#include "gtest/gtest.h"
#include <atomic>
#include <thread>

using namespace gmlc::libguarded;

TEST(guarded_opt, guarded_1)
{
    guarded_opt<int, std::timed_mutex> data(true, 0);

    {
        auto data_handle = data.lock();

        ++(*data_handle);
    }

    {
        auto data_handle = data.try_lock();

        std::atomic<bool> th1_ok(true);
        std::atomic<bool> th2_ok(true);
        std::atomic<bool> th3_ok(true);

        EXPECT_TRUE(data_handle);
        EXPECT_EQ(*data_handle, 1);

        /* These tests must be done from another thread, because on
           glibc std::mutex is actually a recursive mutex. */

        std::thread th1([&data, &th1_ok]() {
            auto data_handle2 = data.try_lock();
            if (data_handle2) th1_ok = false;
        });

        std::thread th2([&data, &th2_ok]() {
            auto data_handle2 = data.try_lock_for(std::chrono::milliseconds(20));
            if (data_handle2) th2_ok = false;
        });

        std::thread th3([&data, &th3_ok]() {
            auto data_handle2 = data.try_lock_until(
                std::chrono::steady_clock::now() + std::chrono::milliseconds(20));
            if (data_handle2) th3_ok = false;
        });

        th1.join();
        th2.join();
        th3.join();
        EXPECT_TRUE(th1_ok == true);
        EXPECT_TRUE(th2_ok == true);
        EXPECT_TRUE(th3_ok == true);
    }
}

TEST(guarded_opt, guarded_2)
{
    guarded_opt<int> data(true, 0);

    std::thread th1([&data]() {
        for (int i = 0; i < 10000; ++i) {
            auto data_handle = data.lock();
            ++(*data_handle);
        }
    });

    std::thread th2([&data]() {
        for (int i = 0; i < 10000; ++i) {
            auto data_handle = data.lock();
            ++(*data_handle);
        }
    });

    th1.join();
    th2.join();

    auto data_handle = data.lock();

    EXPECT_EQ(*data_handle, 20000);
}
