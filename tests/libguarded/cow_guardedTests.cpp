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
 Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See the top-level NOTICE for
 additional details. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause
 */
/*
modified to use google test
*/
#include "gtest/gtest.h"
#include <libguarded/cow_guarded.hpp>
#include <thread>

using namespace gmlc::libguarded;

TEST(cow_guarded, cow_guarded_1)
{
    cow_guarded<int, std::timed_mutex> data(0);

    {
        auto data_handle = data.lock();

        ++(*data_handle);
    }

    {
        auto data_handle = data.lock_shared();

        EXPECT_TRUE(data_handle != nullptr);
        EXPECT_EQ(*data_handle, 1);

        std::thread th1([&data]() {
            auto data_handle2 = data.try_lock_shared();
            EXPECT_TRUE(data_handle2 != nullptr);
            EXPECT_EQ(*data_handle2, 1);
        });

        std::thread th2([&data]() {
            auto data_handle2 = data.try_lock_shared_for(std::chrono::milliseconds(20));
            EXPECT_TRUE(data_handle2 != nullptr);
            EXPECT_EQ(*data_handle2, 1);
        });

        std::thread th3([&data]() {
            auto data_handle2 = data.try_lock_shared_until(
                std::chrono::steady_clock::now() + std::chrono::milliseconds(20));
            EXPECT_TRUE(data_handle2 != nullptr);
            EXPECT_EQ(*data_handle2, 1);
        });

        th1.join();

        th2.join();
        th3.join();
    }

    {
        auto data_handle = data.lock();

        auto data_handle2 = data.lock_shared();

        ++(*data_handle);
        EXPECT_EQ(*data_handle, 2);

        EXPECT_TRUE(data_handle2 != nullptr);
        EXPECT_EQ(*data_handle2, 1);

        data_handle.cancel();
        EXPECT_TRUE(data_handle == nullptr);

        EXPECT_TRUE(data_handle2 != nullptr);
        EXPECT_EQ(*data_handle2, 1);
    }

    {
        auto data_handle = data.lock_shared();

        EXPECT_TRUE(data_handle != nullptr);
        EXPECT_EQ(*data_handle, 1);
    }
}

TEST(cow_guarded, cow_guarded_2)
{
    cow_guarded<int, std::timed_mutex> data(0);

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
        for (int i = 0; i < 100000; ++i) {
            while (last_val != 200000) {
                auto data_handle = data.lock_shared();
                EXPECT_TRUE(last_val <= *data_handle);
                last_val = *data_handle;
            }
        }
    });

    th1.join();
    th2.join();
    th3.join();

    auto data_handle = data.lock_shared();

    EXPECT_EQ(*data_handle, 200000);
}
