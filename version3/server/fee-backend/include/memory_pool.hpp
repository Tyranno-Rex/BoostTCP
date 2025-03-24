#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <zlib.h>
#include <mutex>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


class MemoryPool {
private:
	std::queue<std::array<char, 1540>> memory_blocks;
	std::mutex pool_mutex;
	std::condition_variable pool_cv;
	size_t pool_size;

public:
	MemoryPool() {}


	// 생성자에서 메모리 블록을 초기화합니다.
	MemoryPool(size_t PoolSize) : pool_size(PoolSize) {
		for (size_t i = 0; i < pool_size; i++) {
			memory_blocks.push(std::array<char, 1540>());
		}
	}

	// 메모리 블록을 초기화합니다.
	void init(size_t PoolSize) {
		pool_size = PoolSize;
		LOGI << "Memory pool init start";
		for (size_t i = 0; i < pool_size; i++) {
			memory_blocks.push(std::array<char, 1540>());
		}
		LOGI << "Memory pool init end";
	}

	// 메모리 블록을 할당하여 반환
	std::array<char, 1540> acquire() {
		std::unique_lock<std::mutex> lock(pool_mutex);
		while (memory_blocks.empty()) {
			pool_cv.wait(lock);  // 큐가 비었을 때 기다림
		}
		auto block = memory_blocks.front();
		memory_blocks.pop();
		return block;
	}

	// 메모리 블록을 반환
	void release(std::array<char, 1540>& block) {
		std::lock_guard<std::mutex> lock(pool_mutex);
		memset(block.data(), 0, block.size());
		memory_blocks.push(block);  // 반환된 메모리 블록을 큐에 다시 넣기
		pool_cv.notify_one();     // 대기 중인 스레드에게 알림
	}

	// 종료 시 메모리 블록을 모두 반환
	void clear() {
		std::lock_guard<std::mutex> lock(pool_mutex);
		while (!memory_blocks.empty()) {
			memory_blocks.pop();
		}
		std::queue<std::array<char, 1540>> empty_queue;
		std::swap(memory_blocks, empty_queue);
	}
};


template <typename T>
class MemoryPool_2 {
private:
    std::vector<std::shared_ptr<T>> pool_;
    std::mutex pool_mutex_;
    using FactoryFunc = std::function<std::shared_ptr<T>()>;
    FactoryFunc factory_;

public:
    MemoryPool_2(size_t count, FactoryFunc factory) : factory_(factory) {
        for (size_t i = 0; i < count; ++i) {
            pool_.emplace_back(factory_());
        }
    }

    ~MemoryPool_2() {
        close();
    }

	void init(size_t count) {
		for (size_t i = 0; i < count; ++i) {
			pool_.emplace_back(factory_());
		}
	}

    std::shared_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        if (pool_.empty()) {
            //return factory_();
			pool_.emplace_back(factory_());
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


