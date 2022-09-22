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
#include "gtest/gtest.h"
#include <iostream>
#include <libguarded/rcu_guarded.hpp>
#include <libguarded/rcu_list.hpp>
#include <thread>

using namespace gmlc::libguarded;

TEST(rcu_guarded, rcu_guarded_1)
{
    rcu_guarded<rcu_list<int>> my_list;

    {
        auto h = my_list.lock_write();
        h->push_back(42);
    }

    {
        auto h = my_list.lock_read();

        int count = 0;
        for (auto it = h->begin(); it != h->end(); ++it) {
            auto item = *it;
            ++count;
            EXPECT_EQ(item, 42);
        }
        EXPECT_EQ(count, 1);
    }

    {
        auto rh = my_list.lock_read();
        auto wh = my_list.lock_write();

        auto iter = rh->begin();

        wh->erase(wh->begin());

        int count = 0;
        for (; iter != rh->end(); ++iter) {
            ++count;
            EXPECT_EQ(*iter, 42);
        }
        EXPECT_EQ(count, 1);
    }

    {
        auto h = my_list.lock_read();

        int count = 0;
        volatile int escape;
        for (auto it = h->begin(); it != h->end(); ++it) {
            escape = *it;
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
                    auto rh = my_list.lock_write();
                    volatile int escape;
                    for (auto it = rh->begin(); it != rh->end(); ++it) {
                        escape = *it;
                        (void)escape;
                    }
                }
            });

            threads.emplace_back([&]() {
                int count = 0;
                while (!t_writers_done.load() && count < 1000) {
                    auto wh = my_list.lock_write();
                    volatile int escape;
                    for (auto it = wh->begin(); it != wh->end(); ++it) {
                        escape = *it;
                        (void)escape;
                    }
                    for (int ii = 0; ii < 2; ++ii) {
                        wh->emplace_back(ii);
                        wh->emplace_front(ii - 1);
                        wh->push_back(ii + 4);
                        wh->push_front(ii - 7);
                    }
                    ++count;
                }
                ++t_writers_done;
            });
        };

        threads.emplace_back([&]() {
            while (t_writers_done.load() != num_writers) {
                auto wh = my_list.lock_write();
                for (auto iter = wh->begin(); iter != wh->end();) {
                    iter = wh->erase(iter);
                }
            }

            // Do one last time now that writers are finished
            auto wh = my_list.lock_write();
            for (auto iter = wh->begin(); iter != wh->end();) {
                iter = wh->erase(iter);
            }
        });

        for (auto& thread : threads) {
            thread.join();
        }
    }

    {
        auto h = my_list.lock_read();

        int count = 0;
        volatile int escape;
        for (auto it = h->begin(); it != h->end(); ++it) {
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
    bool allocated; // true for allocate(), false for deallocate()
};
using event_log = std::vector<event>;

template<typename T>
class mock_allocator {
    event_log* const log;

  public:
    using value_type = T;

    explicit mock_allocator(event_log* elog): log(elog) {}
    mock_allocator(const mock_allocator& other): log(other.log) {}

    // converting copy constructor (requires friend)
    template<typename>
    friend class mock_allocator;
    template<typename U>
    explicit mock_allocator(const mock_allocator<U>& other): log(other.log)
    {
    }

    T* allocate(size_t n, const void* hint = 0)
    {
        auto p = std::allocator<T>{}.allocate(n, hint);
        log->emplace_back(event{n * sizeof(T), true});
        return p;
    }
    void deallocate(T* p, size_t n)
    {
        if (p) {
            std::allocator<T>{}.deallocate(p, n);
            log->emplace_back(event{n * sizeof(T), false});
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

        auto h = my_list.lock_write(); // allocates zombie
        h->emplace_back(); // allocates node
        h->erase(h->begin()); // allocates zombie

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
