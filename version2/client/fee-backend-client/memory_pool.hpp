#pragma once

#include <iostream>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <iostream>

template <typename T>
class MemoryPool {
private:
    std::vector<std::shared_ptr<T>> pool_;
    std::mutex pool_mutex_;
    using FactoryFunc = std::function<std::shared_ptr<T>()>;
    FactoryFunc factory_;

public:
    MemoryPool(size_t count, FactoryFunc factory) : factory_(factory) {
        for (size_t i = 0; i < count; ++i) {
            pool_.emplace_back(factory_()); 
        }
    }

    ~MemoryPool() {
        close();
    }

    std::shared_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        if (pool_.empty()) {
            return factory_();
        }
        auto obj = pool_.back();
        pool_.pop_back();
        return obj;
    }

    void release(std::shared_ptr<T> obj) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        pool_.push_back(std::move(obj)); // 반환된 객체 저장
    }

    void close() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        pool_.clear();
    }
};
