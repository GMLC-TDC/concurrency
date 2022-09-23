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
Copyright (c) 2017-2022,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
/*
modified to use google test
*/
#include "gtest/gtest.h"
#include <atomic>
#include <libguarded/ordered_guarded.hpp>
#include <shared_mutex>
#include <thread>

using shared_mutex = std::shared_timed_mutex;

using namespace gmlc::libguarded;

TEST(ordered_guarded, ordered_guarded_1)
{
    ordered_guarded<int, shared_mutex> data(0);

    data.modify([](int& value) { ++value; });

    {
        auto data_handle = data.lock_shared();

        std::atomic<bool> th1_ok(true);
        std::atomic<bool> th2_ok(true);
        std::atomic<bool> th3_ok(true);

        EXPECT_TRUE(data_handle);
        EXPECT_EQ(*data_handle, 1);

        std::thread th1([&data, &th1_ok]() {
            auto data_handle2 = data.try_lock_shared();
            if (!data_handle2) { th1_ok = false; }
            if (*data_handle2 != 1) { th1_ok = false; }
        });

        std::thread th2([&data, &th2_ok]() {
            auto data_handle2 =
                data.try_lock_shared_for(std::chrono::milliseconds(20));
            if (!data_handle2) { th2_ok = false; }
            if (*data_handle2 != 1) { th2_ok = false; }
        });

        std::thread th3([&data, &th3_ok]() {
            auto data_handle2 = data.try_lock_shared_until(
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(20));
            if (!data_handle2) { th3_ok = false; }
            if (*data_handle2 != 1) { th3_ok = false; }
        });

        th1.join();
        th2.join();
        th3.join();
        EXPECT_TRUE(th1_ok == true);
        EXPECT_TRUE(th2_ok == true);
        EXPECT_TRUE(th3_ok == true);
    }
}

TEST(ordered_guarded, ordered_guarded_2)
{
    ordered_guarded<int, shared_mutex> data(0);

    std::atomic<bool> th1_ok(true);
    std::atomic<bool> th2_ok(true);
    std::atomic<bool> th3_ok(true);
    std::atomic<bool> th4_ok(true);

    std::thread th1([&data]() {
        for (int i = 0; i < 100000; ++i) {
            data.modify([](int& x) { ++x; });
        }
    });

    std::thread th2([&data, &th2_ok]() {
        for (int i = 0; i < 100000; ++i) {
            int check_i = data.modify([i](int& x) {
                ++x;
                return i;
            });
            if (check_i != i) {
                th2_ok = false;
            }
        }
    });

    std::thread th3([&data, &th3_ok]() {
        int last_val = 0;
        while (last_val != 200000) {
            auto data_handle = data.lock_shared();
            if (last_val > *data_handle) {
                th3_ok = false;
            }
            last_val = *data_handle;
        }
    });

    std::thread th4([&data, &th4_ok]() {
        int last_val = 0;
        while (last_val != 200000) {
            int new_data = data.read([](const int& value) { return value; });
            if (last_val > new_data) {
                th4_ok = false;
            }
            last_val = new_data;
        }
    });

    th1.join();
    th2.join();

    {
        auto data_handle = data.lock_shared();

        EXPECT_EQ(*data_handle, 200000);
    }

    th3.join();
    th4.join();

    EXPECT_TRUE(th1_ok == true);
    EXPECT_TRUE(th2_ok == true);
    EXPECT_TRUE(th3_ok == true);
    EXPECT_TRUE(th4_ok == true);

    EXPECT_EQ(data.modify([](const int& value) { return value; }), 200000);
}
