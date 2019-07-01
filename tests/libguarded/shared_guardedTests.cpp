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
for Sustainable Energy, LLC All rights reserved. See LICENSE file and DISCLAIMER
for more details.
*/
/*
modified to use google test
*/
#include "gtest/gtest.h"

#include <libguarded/shared_guarded.hpp>

#include <atomic>
#include <thread>

#ifndef HAVE_CXX14
//#error This file requires the C++14 shared_mutex functionality
#endif

#include <shared_mutex>
using shared_mutex = std::shared_timed_mutex;
namespace chrono = std::chrono;

using namespace gmlc::libguarded;

TEST(shared_guarded, shared_guarded_1)
{
    shared_guarded<int, shared_mutex> data(0);

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
            if (data_handle2)
                th1_ok = false;
        });

        std::thread th2([&]() {
            auto data_handle2 =
              data.try_lock_for(std::chrono::milliseconds(20));
            if (data_handle2)
                th2_ok = false;
        });

        std::thread th3([&]() {
            auto data_handle2 = data.try_lock_until(
              std::chrono::steady_clock::now() + std::chrono::milliseconds(20));
            if (data_handle2)
                th3_ok = false;
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
            if (data_handle2)
                th1_ok = false;
        });

        std::thread th2([&]() {
            auto data_handle2 =
              data.try_lock_shared_for(std::chrono::milliseconds(20));
            if (data_handle2)
                th2_ok = false;
        });

        std::thread th3([&]() {
            auto data_handle2 = data.try_lock_shared_until(
              std::chrono::steady_clock::now() + std::chrono::milliseconds(20));
            if (data_handle2)
                th3_ok = false;
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
            if (data_handle2)
                th1_ok = false;
            if (*data_handle2 != 1)
                th1_ok = false;
        });

        std::thread th2([&]() {
            auto data_handle2 =
              data.try_lock_shared_for(std::chrono::milliseconds(20));
            if (data_handle2)
                th2_ok = false;
            if (*data_handle2 != 1)
                th2_ok = false;
        });

        std::thread th3([&]() {
            auto data_handle2 = data.try_lock_shared_until(
              std::chrono::steady_clock::now() + std::chrono::milliseconds(20));
            if (data_handle2)
                th3_ok = false;
            if (*data_handle2 != 1)
                th3_ok = false;
        });

        th1.join();
        th2.join();
        th3.join();

        EXPECT_TRUE(th1_ok == true);
        EXPECT_TRUE(th2_ok == true);
        EXPECT_TRUE(th3_ok == true);
    }
}

TEST(shared_guarded, shared_guarded_2)
{
    shared_guarded<int, shared_mutex> data(0);

    std::thread th1([&data]() {
        for (int i = 0; i < 100000; ++i)
        {
            auto data_handle = data.lock();
            ++(*data_handle);
        }
    });

    std::thread th2([&data]() {
        for (int i = 0; i < 100000; ++i)
        {
            auto data_handle = data.lock();
            ++(*data_handle);
        }
    });

    std::thread th3([&data]() {
        int last_val = 0;
        while (last_val != 200000)
        {
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
