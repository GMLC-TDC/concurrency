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
#include <libguarded/deferred_guarded.hpp>
#include <thread>

#ifndef HAVE_CXX14
//#error This file requires the C++14 shared_mutex functionality
#endif

#include <shared_mutex>
using shared_mutex = std::shared_timed_mutex;

using namespace gmlc::libguarded;

TEST(deferred_guarded, deferred_guarded_1)
{
    deferred_guarded<int, shared_mutex> data(0);

    data.modify_detach([](int& x) { ++x; });

    {
        auto data_handle = data.lock_shared();

        EXPECT_TRUE(data_handle);
        EXPECT_EQ(*data_handle, 1);

        std::atomic<bool> th1_ok(true);
        std::atomic<bool> th2_ok(true);
        std::atomic<bool> th3_ok(true);

        std::thread th1([&data, &th1_ok]() {
            auto data_handle2 = data.try_lock_shared();
            if (!data_handle2) th1_ok = false;
            if (*data_handle2 != 1) th1_ok = false;
        });

        std::thread th2([&data, &th2_ok]() {
            auto data_handle2 =
                data.try_lock_shared_for(std::chrono::milliseconds(20));
            if (!data_handle2) th2_ok = false;
            if (*data_handle2 != 1) th2_ok = false;
        });

        std::thread th3([&data, &th3_ok]() {
            auto data_handle2 = data.try_lock_shared_until(
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(20));
            if (!data_handle2) th3_ok = false;
            if (*data_handle2 != 1) th3_ok = false;
        });

        th1.join();
        th2.join();
        th3.join();
        EXPECT_TRUE(th1_ok == true);
        EXPECT_TRUE(th2_ok == true);
        EXPECT_TRUE(th3_ok == true);
    }
}

TEST(deferred_guarded, deferred_guarded_2)
{
    deferred_guarded<int, shared_mutex> data(0);

    std::thread th1([&data]() {
        for (int i = 0; i < 100000; ++i) {
            data.modify_detach([](int& x) { ++x; });
        }
    });

    std::thread th2([&data]() {
        for (int i = 0; i < 100000; ++i) {
            auto fut = data.modify_async([](int& x) -> int { return ++x; });
            fut.wait();
        }
    });
    std::thread th3([&data]() {
        for (int i = 0; i < 100000; ++i) {
            auto fut = data.modify_async([](int& x) -> void { ++x; });
            fut.wait();
        }
    });

    std::thread th4([&data]() {
        int last_val = 0;
        while (last_val != 300000) {
            auto data_handle = data.lock_shared();
            EXPECT_TRUE(last_val <= *data_handle);
            last_val = *data_handle;
        }
    });

    th1.join();
    th2.join();
    th3.join();
    th4.join();

    auto data_handle = data.lock_shared();

    EXPECT_EQ(*data_handle, 300000);
}
