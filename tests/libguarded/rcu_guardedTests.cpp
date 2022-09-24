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
#include <algorithm>
#include <iostream>
#include <libguarded/rcu_guarded.hpp>
#include <libguarded/rcu_list.hpp>
#include <thread>

using namespace gmlc::libguarded;

TEST(rcu_guarded, rcu_guarded_1)
{
    rcu_guarded<rcu_list<int>> my_list;

    {
        auto handle = my_list.lock_write();
        handle->push_back(42);
    }

    {
        auto handle = my_list.lock_read();

        int count = 0;
        for (auto item : *handle) {
            ++count;
            EXPECT_EQ(item, 42);
        }
        EXPECT_EQ(count, 1);
    }

    {
        auto readHandle = my_list.lock_read();
        auto writeHandle = my_list.lock_write();

        auto iter = readHandle->begin();

        writeHandle->erase(writeHandle->begin());

        int count = 0;
        for (; iter != readHandle->end(); ++iter) {
            ++count;
            EXPECT_EQ(*iter, 42);
        }
        EXPECT_EQ(count, 1);
    }

    {
        auto handle = my_list.lock_read();

        int count = 0;
        volatile int escape;
        for (int it : *handle) {
            escape = it;
            (void)escape;
            ++count;
        }
        EXPECT_EQ(count, 0);
    }

    {
        constexpr const int num_writers = 8;
        std::atomic<int> t_writers_done{0};

        std::vector<std::thread> threads;
        for (int i = 0; i < num_writers; ++i) {
            threads.emplace_back([&]() {
                while (!t_writers_done.load()) {
                    auto rHandle = my_list.lock_write();
                    volatile int escape;
                    // NOLINTNEXTLINE
                    for (auto it = rHandle->begin(); it != rHandle->end();
                         ++it) {
                        escape = *it;
                        (void)escape;
                    }
                }
            });

            threads.emplace_back([&]() {
                int count = 0;
                while (t_writers_done.load() == 0 && count < 1000) {
                    auto writeHandle = my_list.lock_write();
                    volatile int escape;
                    // NOLINTNEXTLINE
                    for (auto it = writeHandle->begin();
                         it != writeHandle->end();
                         ++it) {
                        escape = *it;
                        (void)escape;
                    }
                    for (int ii = 0; ii < 2; ++ii) {
                        writeHandle->emplace_back(ii);
                        writeHandle->emplace_front(ii - 1);
                        writeHandle->push_back(ii + 4);
                        writeHandle->push_front(ii - 7);
                    }
                    ++count;
                }
                ++t_writers_done;
            });
        }

        threads.emplace_back([&]() {
            while (t_writers_done.load() != num_writers) {
                auto writeHandle = my_list.lock_write();
                for (auto iter = writeHandle->begin();
                     iter != writeHandle->end();) {
                    iter = writeHandle->erase(iter);
                }
            }

            // Do one last time now that writers are finished
            auto writeHandle = my_list.lock_write();
            for (auto iter = writeHandle->begin();
                 iter != writeHandle->end();) {
                iter = writeHandle->erase(iter);
            }
        });

        for (auto& thread : threads) {
            thread.join();
        }
    }

    {
        auto handle = my_list.lock_read();

        int count = 0;
        volatile int escape;
        // NOLINTNEXTLINE
        for (auto it = handle->begin(); it != handle->end(); ++it) {
            escape = *it;
            (void)escape;
            ++count;
        }
        EXPECT_EQ(count, 0);
    }
}

// allocation events recorded by mock_allocator
struct event {
    size_t size;
    bool allocated;  // true for allocate(), false for deallocate()
};
using event_log = std::vector<event>;

template<typename T>
class mock_allocator {
    event_log* const log;

  public:
    using value_type = T;

    explicit mock_allocator(event_log* elog) : log(elog) {}
    mock_allocator(const mock_allocator& other) : log(other.log) {}

    // converting copy constructor (requires friend)
    template<typename>
    friend class mock_allocator;
    template<typename U>
    explicit mock_allocator(const mock_allocator<U>& other) : log(other.log)
    {
    }

    T* allocate(size_t size)
    {
        auto memory = std::allocator<T>{}.allocate(size);
        log->emplace_back(event{size * sizeof(T), true});
        return memory;
    }
    void deallocate(T* memory, size_t size)
    {
        if (memory) {
            std::allocator<T>{}.deallocate(memory, size);
            log->emplace_back(event{size * sizeof(T), false});
        }
    }
};

TEST(rcu_guarded, rcu_guarded_allocator)
{
    // large value type makes it easy to distinguish nodes from zombies
    // (this avoids any dependency on the private rcu_list node types)
    constexpr size_t value_size = 256;
    auto is_zombie = [=](const event& e) { return e.size < value_size; };
    auto is_alloc = [](const event& e) { return e.allocated; };

    using T = std::aligned_storage<value_size>::type;

    event_log log;
    {
        mock_allocator<T> alloc{&log};
        rcu_guarded<rcu_list<T, std::mutex, mock_allocator<T>>> my_list(alloc);

        auto handle = my_list.lock_write();  // allocates zombie
        handle->emplace_back();  // allocates node
        handle->erase(handle->begin());  // allocates zombie

        // expect 3 allocations, two of which are zombies. just count events,
        // don't make assumptions about ordering
        EXPECT_EQ(3, log.size());
        EXPECT_EQ(3, std::count_if(log.begin(), log.end(), is_alloc));
        EXPECT_EQ(2, std::count_if(log.begin(), log.end(), is_zombie));
    }

    // expects 3 new deallocations, two of which are zombies
    EXPECT_EQ(6, log.size());
    EXPECT_EQ(3, std::count_if(log.begin(), log.end(), is_alloc));
    EXPECT_EQ(4, std::count_if(log.begin(), log.end(), is_zombie));
}
