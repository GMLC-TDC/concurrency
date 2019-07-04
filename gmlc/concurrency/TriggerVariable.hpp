/*
Copyright © 2017-2019,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <atomic>

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace gmlc
{
namespace concurrency
{
class TriggerVariable
{
  public:
    explicit TriggerVariable(bool active = false)
        : triggered(false), activated(active){};
    /** activate the trigger to the ready state
    @return true if the Trigger was activated false if it was already active
    */
    bool activate()
    {
        std::lock_guard<std::mutex> lock(stateLock);
        if (activated)
        {
            // we are already activated so this function did nothing so return
            // false
            return false;
        }
        activated = true;
        cv_active.notify_all();
        return true;
    }
    /** trigger the variable
    @return true if the trigger was successful, false if the trigger has not
    been activated yet*/
    bool trigger()
    {
        std::lock_guard<std::mutex> lock(stateLock);
        if (activated)
        {
            triggered.store(true, std::memory_order_release);
            cv_trigger.notify_all();
            return true;
        }
        return false;
    }
    /** check if the variable has been triggered*/
    bool isTriggered() const
    {
        return triggered.load(std::memory_order_acquire);
    }
    /** wait for the variable to trigger*/
    void wait() const
    {
        std::unique_lock<std::mutex> lk(stateLock);
        if (activated && (!triggered.load(std::memory_order_acquire)))
        {
            cv_trigger.wait(lk, [this] {
                return triggered.load(std::memory_order_acquire);
            });
        }
    }
    /** wait for a period of time for the value to trigger*/
    bool wait_for(const std::chrono::milliseconds &duration) const
    {
        std::unique_lock<std::mutex> lk(stateLock);
        if (activated && (!triggered.load(std::memory_order_acquire)))
        {
            return cv_trigger.wait_for(lk, duration, [this] {
                return triggered.load(std::memory_order_acquire);
            });
        }
        return true;
    }
    /** wait on the Trigger becoming active*/
    void waitActivation() const
    {
        std::unique_lock<std::mutex> lk(stateLock);
        if (!activated)
        {
            cv_active.wait(lk, [this] { return activated; });
        }
    }
    /** wait for a period of time for the value to trigger*/
    bool wait_forActivation(const std::chrono::milliseconds &duration) const
    {
        std::unique_lock<std::mutex> lk(stateLock);
        if (!activated)
        {
            return cv_active.wait_for(lk, duration,
                                      [this] { return activated; });
        }
        return true;
    }
    /** reset the trigger Variable to the inactive state*/
    void reset()
    {
        std::unique_lock<std::mutex> lk(stateLock);
        if (activated)
        {
            while (!triggered.load(std::memory_order_acquire))
            {
                lk.unlock();
                trigger();
                lk.lock();
            }
        }
        activated = false;
    }
    /** check if the variable is active*/
    bool isActive() const
    {
        std::lock_guard<std::mutex> lock(stateLock);
        return activated;
    }

  private:
    std::atomic<bool> triggered;  //!< the state of the trigger
    bool activated;  //!< variable controlling if the trigger has been activated
    mutable std::mutex stateLock;  //!< mutex protecting the trigger
    mutable std::condition_variable cv_trigger;  //!< semaphore for the trigger
    mutable std::condition_variable
      cv_active;  //!< semaphore for the activation
};

}  // namespace concurrency
}  // namespace gmlc
