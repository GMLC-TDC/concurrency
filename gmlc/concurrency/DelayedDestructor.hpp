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
        size_t destroyObjects(std::chrono::milliseconds wait) noexcept
        {
            std::size_t elementSize{static_cast<std::size_t>(-1)};
            try {
                std::unique_lock<std::timed_mutex> lock(destructionLock,std::defer_lock);
                if (!lock.try_lock_for(wait))
                {
                    return elementSize;
                }
                std::size_t elementSize=ElementsToBeDestroyed.size();
                if (elementSize>0) {
                    std::vector<std::shared_ptr<X>> ecall;
                    std::vector<std::string> ename;
                    for (auto& element : ElementsToBeDestroyed) {
                        if (element.use_count() == 1) {
                            ecall.push_back(element);
                            ename.push_back(element->getIdentifier());
                        }
                    }
                    if (!ename.empty()) {
                        // so apparently remove_if can actually call the
                        // destructor for shared_ptrs so the call function needs
                        // to be before this call
                        auto loc = std::remove_if(
                            ElementsToBeDestroyed.begin(),
                            ElementsToBeDestroyed.end(),
                            [&ename](const auto& element) {
                                return (
                                    (element.use_count() == 2) &&
                                    (std::find(
                                         ename.begin(),
                                         ename.end(),
                                         element->getIdentifier()) !=
                                     ename.end()));
                            });
                        ElementsToBeDestroyed.erase(
                            loc, ElementsToBeDestroyed.end());
                        elementSize=ElementsToBeDestroyed.size();
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
                        if (!lock.try_lock_for(wait))
                        {
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
            std::unique_lock<std::timed_mutex> lock(destructionLock,std::defer_lock);
            if (!lock.try_lock_for(delay))
            {
                return static_cast<size_t>(-1);
            }
            auto delayTime = (delay < 100ms) ? delay : 50ms;
            int delayCount =
                (delay < 100ms) ? 1 : static_cast<int>((delay.count() / 50));

            int cnt = 0;
            auto elementSize=ElementsToBeDestroyed.size();
            while (elementSize>0 && (cnt < delayCount)) {
                if (cnt > 0)  // don't sleep on the first loop
                {
                    if (delay > 4ms)
                    {
                        lock.unlock();
                        std::this_thread::sleep_for(delayTime);
                        if (!lock.try_lock_for(delayTime))
                        {
                            return elementSize;
                        }
                    }
                }
                ++cnt;
                elementSize=ElementsToBeDestroyed.size();
                if (elementSize>0) {
                    lock.unlock();
                    destroyObjects();
                    if (!lock.try_lock_for(delayTime))
                    {
                        return elementSize;
                    }
                }
            }
            return ElementsToBeDestroyed.size();
        }

        void addObjectsToBeDestroyed(std::shared_ptr<X> obj)
        {
            std::lock_guard<std::timed_mutex> lock(destructionLock);
            ElementsToBeDestroyed.push_back(std::move(obj));
        }
    };

}  // namespace concurrency
