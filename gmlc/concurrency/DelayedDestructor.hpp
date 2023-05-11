/*
Copyright (c) 2017-2022,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#ifdef ENABLE_TRIPWIRE
#include "TripWire.hpp"
#endif

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gmlc::concurrency {
/** helper class to destroy objects at a late time when it is convenient and
* there are no more possibilities of threading issues
@details this is essentially a delayed garbage collector based on shared_ptrs*/
template<class X>
class DelayedDestructor {
  private:
    std::timed_mutex destructionLock;
    std::vector<std::shared_ptr<X>> ElementsToBeDestroyed;
    std::function<void(std::shared_ptr<X>& ptr)> callBeforeDeleteFunction;
#ifdef ENABLE_TRIPWIRE
    TripWireDetector tripDetect;
#endif

  public:
    DelayedDestructor() = default;
    explicit DelayedDestructor(
        std::function<void(std::shared_ptr<X>& ptr)> callFirst) :
        callBeforeDeleteFunction(std::move(callFirst))
    {
    }
    ~DelayedDestructor()
    {
        try {
            int ii = 0;
            while (!ElementsToBeDestroyed.empty()) {
                ++ii;
                destroyObjects();
                if (!ElementsToBeDestroyed.empty()) {
#ifdef ENABLE_TRIPWIRE
                    // short circuit if the tripline was triggered
                    if (tripDetect.isTripped()) {
                        return;
                    }
#endif
                    if (ii > 4) {
                        destroyObjects();
                        break;
                    }
                    if (ii % 2 == 0) {
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(100));
                    } else {
                        std::this_thread::yield();
                    }
                }
            }
        }
        catch (...) {
        }
    }
    DelayedDestructor(DelayedDestructor&&) noexcept = delete;
    DelayedDestructor& operator=(DelayedDestructor&&) noexcept = delete;

    /** destroy objects that are no longer used*/
    size_t destroyObjects() noexcept
    {
        std::size_t elementSize{static_cast<std::size_t>(-1)};
        std::chrono::milliseconds wait(std::chrono::milliseconds(200));
        try {
            std::unique_lock<std::timed_mutex> lock(
                destructionLock, std::defer_lock);
            if (!lock.try_lock_for(wait)) {
                return elementSize;
            }
            elementSize = ElementsToBeDestroyed.size();
            if (elementSize > 0) {
                std::vector<std::shared_ptr<X>> ecall;
                std::vector<void*> epointers;
                for (auto& element : ElementsToBeDestroyed) {
                    if (element.use_count() == 1) {
                        ecall.push_back(element);
                        epointers.emplace_back(element.get());
                    }
                }
                if (!epointers.empty()) {
                    // so apparently remove_if can actually call the
                    // destructor for shared_ptrs so the call function needs
                    // to be before this call
                    auto loc = std::remove_if(
                        ElementsToBeDestroyed.begin(),
                        ElementsToBeDestroyed.end(),
                        [&epointers](const auto& element) {
                            return (
                                (element.use_count() == 2) &&
                                (std::find(
                                     epointers.begin(),
                                     epointers.end(),
                                     static_cast<void*>(element.get())) !=
                                 epointers.end()));
                        });
                    ElementsToBeDestroyed.erase(
                        loc, ElementsToBeDestroyed.end());
                    elementSize = ElementsToBeDestroyed.size();
                    auto deleteFunc = callBeforeDeleteFunction;
                    lock.unlock();
                    // this needs to be done after the lock, so a destructor
                    // can never called while under the lock
                    if (deleteFunc) {
                        for (auto& element : ecall) {
                            deleteFunc(element);
                        }
                    }
                    ecall.clear();  // make sure the destructors get called
                                    // before returning.
                    // reengage the lock so the size is correct
                    if (!lock.try_lock_for(wait)) {
                        return elementSize;
                    }
                }
            }
            return ElementsToBeDestroyed.size();
        }
        catch (...) {
        }
        return elementSize;
    }

    size_t destroyObjects(std::chrono::milliseconds delay)
    {
        using namespace std::literals::chrono_literals;
        std::unique_lock<std::timed_mutex> lock(
            destructionLock, std::defer_lock);
        auto minDelay = (delay > 200ms) ? delay : 200ms;
        if (!lock.try_lock_for(minDelay)) {
            return static_cast<size_t>(-1);
        }
        auto delayTime = (delay < 100ms) ? delay : 50ms;
        int delayCount =
            (delay < 100ms) ? 1 : static_cast<int>((delay.count() / 50));

        int cnt = 0;
        auto elementSize = ElementsToBeDestroyed.size();
        while (elementSize > 0 && (cnt < delayCount)) {
            if (cnt > 0)  // don't sleep on the first loop
            {
                if (delay > 4ms) {
                    lock.unlock();
                    std::this_thread::sleep_for(delayTime);
                    if (!lock.try_lock_for(delayTime)) {
                        return elementSize;
                    }
                }
            }
            ++cnt;
            elementSize = ElementsToBeDestroyed.size();
            if (elementSize > 0) {
                lock.unlock();
                destroyObjects();
                if (!lock.try_lock_for(delayTime)) {
                    return elementSize;
                }
            }
        }
        return ElementsToBeDestroyed.size();
    }
    /// @brief  get the number of elements waiting to be destroyed
    /// @return number of objects
    auto size()
    {
        std::lock_guard<std::timed_mutex> lock(destructionLock);
        return ElementsToBeDestroyed.size();
    }

    void addObjectsToBeDestroyed(std::shared_ptr<X> obj)
    {
        std::lock_guard<std::timed_mutex> lock(destructionLock);
        ElementsToBeDestroyed.push_back(std::move(obj));
    }
};

/// @brief  handle the delayed destructor as a single thread so no locks,
/// possible use with thread local structure
/// @tparam X the class of object to be destroyed
template<class X>
class DelayedDestructorSingleThread {
  private:
    std::vector<std::shared_ptr<X>> ElementsToBeDestroyed;
    std::function<void(std::shared_ptr<X>& ptr)> callBeforeDeleteFunction;
#ifdef ENABLE_TRIPWIRE
    TripWireDetector tripDetect;
#endif

  public:
    DelayedDestructorSingleThread() = default;
    explicit DelayedDestructorSingleThread(
        std::function<void(std::shared_ptr<X>& ptr)> callFirst) :
        callBeforeDeleteFunction(std::move(callFirst))
    {
    }
    ~DelayedDestructorSingleThread()
    {
        try {
            int ii = 0;
            while (!ElementsToBeDestroyed.empty()) {
                ++ii;
                destroyObjects();
                if (!ElementsToBeDestroyed.empty()) {
#ifdef ENABLE_TRIPWIRE
                    // short circuit if the tripline was triggered
                    if (tripDetect.isTripped()) {
                        return;
                    }
#endif
                    if (ii > 4) {
                        destroyObjects();
                        break;
                    }
                    if (ii % 2 == 0) {
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(100));
                    } else {
                        std::this_thread::yield();
                    }
                }
            }
        }
        catch (...) {
        }
    }
    DelayedDestructorSingleThread(DelayedDestructorSingleThread&&) noexcept =
        delete;
    DelayedDestructorSingleThread&
        operator=(DelayedDestructorSingleThread&&) noexcept = delete;

    /** destroy objects that are no longer used*/
    size_t destroyObjects() noexcept
    {
        std::size_t elementSize{static_cast<std::size_t>(-1)};
        std::chrono::milliseconds wait(std::chrono::milliseconds(50));
        try {
            elementSize = ElementsToBeDestroyed.size();
            if (elementSize > 0) {
                std::vector<std::shared_ptr<X>> ecall;
                std::vector<void*> epointers;
                for (auto& element : ElementsToBeDestroyed) {
                    if (element.use_count() == 1) {
                        ecall.push_back(element);
                        epointers.emplace_back(element.get());
                    }
                }
                if (!epointers.empty()) {
                    // so apparently remove_if can actually call the
                    // destructor for shared_ptrs so the call function needs
                    // to be before this call
                    auto loc = std::remove_if(
                        ElementsToBeDestroyed.begin(),
                        ElementsToBeDestroyed.end(),
                        [&epointers](const auto& element) {
                            return (
                                (element.use_count() == 2) &&
                                (std::find(
                                     epointers.begin(),
                                     epointers.end(),
                                     static_cast<void*>(element.get())) !=
                                 epointers.end()));
                        });
                    ElementsToBeDestroyed.erase(
                        loc, ElementsToBeDestroyed.end());
                    elementSize = ElementsToBeDestroyed.size();
                    auto deleteFunc = callBeforeDeleteFunction;

                    // this needs to be done after the lock, so a destructor
                    // can never called while under the lock
                    if (deleteFunc) {
                        for (auto& element : ecall) {
                            deleteFunc(element);
                        }
                    }
                    ecall.clear();  // make sure the destructors get called
                    // before returning.
                }
            }
            return ElementsToBeDestroyed.size();
        }
        catch (...) {
        }
        return elementSize;
    }

    size_t destroyObjects(std::chrono::milliseconds delay)
    {
        using namespace std::literals::chrono_literals;

        auto delayTime = (delay < 100ms) ? delay : 50ms;
        int delayCount =
            (delay < 100ms) ? 1 : static_cast<int>((delay.count() / 50));

        int cnt = 0;
        auto elementSize = ElementsToBeDestroyed.size();
        while (elementSize > 0 && (cnt < delayCount)) {
            if (cnt > 0)  // don't sleep on the first loop
            {
                if (delay > 4ms) {
                    std::this_thread::sleep_for(delayTime);
                }
            }
            ++cnt;
            elementSize = ElementsToBeDestroyed.size();
            if (elementSize > 0) {
                destroyObjects();
            }
        }
        return ElementsToBeDestroyed.size();
    }

    /// @brief  get the number of elements waiting to be destroyed
    /// @return number of objects
    auto size()
    {
        return ElementsToBeDestroyed.size();
    }

    void addObjectsToBeDestroyed(std::shared_ptr<X> obj)
    {
        ElementsToBeDestroyed.push_back(std::move(obj));
    }
};

}  // namespace gmlc::concurrency
