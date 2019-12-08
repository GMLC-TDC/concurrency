/*
Copyright (c) 2017-2019,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance
for Sustainable Energy, LLC.  See the top-level NOTICE for additional details.
All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#ifdef ENABLE_TRIPWIRE
#    include "TripWire.hpp"
#endif
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace gmlc {
namespace concurrency {
    /** helper class to contain a list of objects that need to be referencable at
 * some level the objects are stored through shared_ptrs*/
    template<class X>
    class SearchableObjectHolder {
      private:
        std::mutex mapLock;
        std::map<std::string, std::shared_ptr<X>> objectMap;
#ifdef ENABLE_TRIPWIRE
        TripWireDetector trippedDetect;
#endif
      public:
        SearchableObjectHolder() = default;
        // class is not movable
        SearchableObjectHolder(SearchableObjectHolder&&) noexcept = delete;
        SearchableObjectHolder& operator=(SearchableObjectHolder&&) noexcept = delete;
        ~SearchableObjectHolder()
        {
#ifdef ENABLE_TRIPWIRE
            // this is a short circuit used to detect shutdown
            if (trippedDetect.isTripped()) {
                return;
            }
#endif
            std::unique_lock<std::mutex> lock(mapLock);
            int cntr = 0;
            while (!objectMap.empty()) {
                ++cntr;
                lock.unlock();
                // don't leave things locked while sleeping or yielding
                if (cntr % 2 != 0) {
                    std::this_thread::yield();
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                lock.lock();
                if (cntr > 6) {
                    break;
                }
            }
        }
        /** add and object to container*/
        bool addObject(const std::string& name, std::shared_ptr<X> obj)
        {
            std::lock_guard<std::mutex> lock(mapLock);
            auto res = objectMap.emplace(name, std::move(obj));
            return res.second;
        }
        /** check if the container is empty
    @details this is really only useful if there is only one thread adding
    object otherwise the results are not totally reliable upon return
    */
        bool empty()
        {
            std::lock_guard<std::mutex> lock(mapLock);
            return objectMap.empty();
        }

        /** get a vector of all the contained objects*/
        std::vector<std::shared_ptr<X>> getObjects()
        {
            std::vector<std::shared_ptr<X>> objs;
            std::lock_guard<std::mutex> lock(mapLock);
            for (auto& obj : objectMap) {
                objs.push_back(obj.second);
            }
            return objs;
        }

        /** remove an object from the object holder by name*/
        bool removeObject(const std::string& name)
        {
            std::lock_guard<std::mutex> lock(mapLock);
            auto fnd = objectMap.find(name);
            if (fnd != objectMap.end()) {
                objectMap.erase(fnd);
                return true;
            }
            return false;
        }

        /** remove an object if it matches a certain criteria as given by the
     * function operator*/
        bool removeObject(std::function<bool(const std::shared_ptr<X>&)> operand)
        {
            std::lock_guard<std::mutex> lock(mapLock);
            for (auto obj = objectMap.begin(); obj != objectMap.end(); ++obj) {
                if (operand(obj->second)) {
                    objectMap.erase(obj);
                    return true;
                }
            }
            return false;
        }

        bool copyObject(const std::string& copyFromName, const std::string& copyToName)
        {
            std::lock_guard<std::mutex> lock(mapLock);
            auto fnd = objectMap.find(copyFromName);
            if (fnd != objectMap.end()) {
                auto newObjectPtr = fnd->second;
                auto ret = objectMap.emplace(copyToName, std::move(newObjectPtr));
                return ret.second;
            }
            return false;
        }

        std::shared_ptr<X> findObject(const std::string& name)
        {
#ifdef ENABLE_TRIPWIRE
            if (trippedDetect.isTripped()) {
                return nullptr;
            }
#endif
            std::lock_guard<std::mutex> lock(mapLock);
            auto fnd = objectMap.find(name);
            if (fnd != objectMap.end()) {
                return fnd->second;
            }
            return nullptr;
        }

        std::shared_ptr<X> findObject(std::function<bool(const std::shared_ptr<X>&)> operand)
        {
            std::lock_guard<std::mutex> lock(mapLock);
            auto obj = std::find_if(objectMap.begin(), objectMap.end(), [&operand](auto& val) {
                return operand(val.second);
            });
            if (obj != objectMap.end()) {
                return obj->second;
            }
            return nullptr;
        }
    };

} // namespace concurrency
} // namespace gmlc
