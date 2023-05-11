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
Copyright (c) 2017-2023,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
/*
modified to use google test
*/

#include "gtest/gtest.h"
#include <atomic>
#include <libguarded/shared_guarded_opt.hpp>
#include <shared_mutex>
#include <thread>

using shared_mutex = std::shared_timed_mutex;

using namespace gmlc::libguarded;

TEST(shared_guarded_opt, shared_guarded_1)
{
    shared_guarded_opt<int, shared_mutex> data(true, 0);

    {
        auto data_handle = data.lock();

        ++(*data_handle);
    }

    {
        auto data_handle = data.try_lock();

        EXPECT_TRUE(data_handle);
        EXPECT_EQ(*data_handle, 1);

        /* These tests must be done from another thread, because on
           glibc std::mutex is actually a recursive mutex. */

        std::atomic<bool> th1_ok(true);
        std::atomic<bool> th2_ok(true);
        std::atomic<bool> th3_ok(true);

        std::thread th1([&]() {
            auto data_handle2 = data.try_lock();
            if (data_handle2) {
                th1_ok = false;
            }
        });

        std::thread th2([&]() {
            auto data_handle2 =
                data.try_lock_for(std::chrono::milliseconds(20));
            if (data_handle2) {
                th2_ok = false;
            }
        });

        std::thread th3([&]() {
            auto data_handle2 = data.try_lock_until(
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(20));
            if (data_handle2) {
                th3_ok = false;
            }
        });

        th1.join();
        th2.join();
        th3.join();

        EXPECT_TRUE(th1_ok == true);
        EXPECT_TRUE(th2_ok == true);
        EXPECT_TRUE(th3_ok == true);
    }

    {
        auto data_handle = data.try_lock();

        EXPECT_TRUE(data_handle);
        EXPECT_EQ(*data_handle, 1);

        std::atomic<bool> th1_ok(true);
        std::atomic<bool> th2_ok(true);
        std::atomic<bool> th3_ok(true);

        std::thread th1([&]() {
            auto data_handle2 = data.try_lock_shared();
            if (data_handle2) {
                th1_ok = false;
            }
        });

        std::thread th2([&]() {
            auto data_handle2 =
                data.try_lock_shared_for(std::chrono::milliseconds(20));
            if (data_handle2) {
                th2_ok = false;
            }
        });

        std::thread th3([&]() {
            auto data_handle2 = data.try_lock_shared_until(
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(20));
            if (data_handle2) {
                th3_ok = false;
            }
        });

        th1.join();
        th2.join();
        th3.join();

        EXPECT_TRUE(th1_ok == true);
        EXPECT_TRUE(th2_ok == true);
        EXPECT_TRUE(th3_ok == true);
    }

    {
        auto data_handle = data.lock_shared();

        EXPECT_TRUE(data_handle);
        EXPECT_EQ(*data_handle, 1);

        std::atomic<bool> th1_ok(true);
        std::atomic<bool> th2_ok(true);
        std::atomic<bool> th3_ok(true);

        std::thread th1([&]() {
            auto data_handle2 = data.try_lock_shared();
            if (!data_handle2) {
                th1_ok = false;
            }
            if (*data_handle2 != 1) {
                th1_ok = false;
            }
        });

        std::thread th2([&]() {
            auto data_handle2 =
                data.try_lock_shared_for(std::chrono::milliseconds(20));
            if (!data_handle2) {
                th2_ok = false;
            }
            if (*data_handle2 != 1) {
                th2_ok = false;
            }
        });

        std::thread th3([&]() {
            auto data_handle2 = data.try_lock_shared_until(
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(20));
            if (!data_handle2) {
                th3_ok = false;
            }
            if (*data_handle2 != 1) {
                th3_ok = false;
            }
        });

        th1.join();
        th2.join();
        th3.join();

        EXPECT_TRUE(th1_ok == true);
        EXPECT_TRUE(th2_ok == true);
        EXPECT_TRUE(th3_ok == true);
    }
}

TEST(shared_guarded_opt, shared_guarded_2)
{
    shared_guarded_opt<int, shared_mutex> data(true, 0);

    std::thread th1([&data]() {
        for (int i = 0; i < 100000; ++i) {
            auto data_handle = data.lock();
            ++(*data_handle);
        }
    });

    std::thread th2([&data]() {
        for (int i = 0; i < 100000; ++i) {
            auto data_handle = data.lock();
            ++(*data_handle);
        }
    });

    std::thread th3([&data]() {
        int last_val = 0;
        while (last_val != 200000) {
            auto data_handle = data.lock_shared();
            EXPECT_TRUE(last_val <= *data_handle);
            last_val = *data_handle;
        }
    });

    th1.join();
    th2.join();
    th3.join();

    auto data_handle = data.lock();

    EXPECT_EQ(*data_handle, 200000);
}

TEST(shared_guarded_opt, shared_guarded_false)
{
    shared_guarded_opt<int, shared_mutex> data(false, 0);

    {
        auto data_handle = data.lock();

        ++(*data_handle);
    }

    {
        auto data_handle = data.try_lock();

        EXPECT_TRUE(data_handle);
        EXPECT_EQ(*data_handle, 1);

        /* These tests must be done from another thread, because on
        glibc std::mutex is actually a recursive mutex. */

        std::atomic<bool> th1_ok(true);
        std::atomic<bool> th2_ok(true);
        std::atomic<bool> th3_ok(true);

        std::thread th1([&]() {
            auto data_handle2 = data.try_lock();
            if (data_handle2) {
                th1_ok = false;
            }
        });

        std::thread th2([&]() {
            auto data_handle2 =
                data.try_lock_for(std::chrono::milliseconds(20));
            if (data_handle2) {
                th2_ok = false;
            }
        });

        std::thread th3([&]() {
            auto data_handle2 = data.try_lock_until(
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(20));
            if (data_handle2) {
                th3_ok = false;
            }
        });

        th1.join();
        th2.join();
        th3.join();

        EXPECT_FALSE(th1_ok);
        EXPECT_FALSE(th2_ok);
        EXPECT_FALSE(th3_ok);
    }

    {
        auto data_handle = data.try_lock();

        EXPECT_TRUE(data_handle);
        EXPECT_EQ(*data_handle, 1);

        std::atomic<bool> th1_ok(true);
        std::atomic<bool> th2_ok(true);
        std::atomic<bool> th3_ok(true);

        std::thread th1([&]() {
            auto data_handle2 = data.try_lock_shared();
            if (data_handle2) {
                th1_ok = false;
            }
        });

        std::thread th2([&]() {
            auto data_handle2 =
                data.try_lock_shared_for(std::chrono::milliseconds(20));
            if (data_handle2) {
                th2_ok = false;
            }
        });

        std::thread th3([&]() {
            auto data_handle2 = data.try_lock_shared_until(
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(20));
            if (data_handle2) {
                th3_ok = false;
            }
        });

        th1.join();
        th2.join();
        th3.join();

        EXPECT_FALSE(th1_ok);
        EXPECT_FALSE(th2_ok);
        EXPECT_FALSE(th3_ok);
    }

    {
        auto data_handle = data.lock_shared();

        EXPECT_TRUE(data_handle);
        EXPECT_EQ(*data_handle, 1);

        std::atomic<bool> th1_ok(true);
        std::atomic<bool> th2_ok(true);
        std::atomic<bool> th3_ok(true);

        std::thread th1([&]() {
            auto data_handle2 = data.try_lock_shared();
            if (!data_handle2) {
                th1_ok = false;
            }
            if (*data_handle2 != 1) {
                th1_ok = false;
            }
        });

        std::thread th2([&]() {
            auto data_handle2 =
                data.try_lock_shared_for(std::chrono::milliseconds(20));
            if (!data_handle2) {
                th2_ok = false;
            }
            if (*data_handle2 != 1) {
                th2_ok = false;
            }
        });

        std::thread th3([&]() {
            auto data_handle2 = data.try_lock_shared_until(
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(20));
            if (!data_handle2) {
                th3_ok = false;
            }
            if (*data_handle2 != 1) {
                th3_ok = false;
            }
        });

        th1.join();
        th2.join();
        th3.join();

        EXPECT_TRUE(th1_ok);
        EXPECT_TRUE(th2_ok);
        EXPECT_TRUE(th3_ok);
    }
}
