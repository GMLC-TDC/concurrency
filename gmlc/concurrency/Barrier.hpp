/*
Copyright (c) 2017-2019,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once
#include <atomic>
#include <mutex>

namespace gmlc
{
namespace concurrency
{
/** namespace for the global variable in tripwire*/

class Barrier
{
  public:
    explicit Barrier(size_t count) : count_(count) {}
    void wait()
    {
        std::unique_lock<std::mutex> lck(mtx);
        if (--count_ == 0)
        {
            cv.notify_all();
        }
        else
        {
            cv.wait(lck, [this] { return count_ == 0; });
        }
    }

  private:
    std::mutex mtx;  //!< mutex for protecting count_
    std::condition_variable cv;  //!< associated condition variable
    size_t count_;  //!< items remaining
};
}  // namespace concurrency
}  // namespace gmlc
