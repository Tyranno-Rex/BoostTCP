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


#include <memory>
#include <functional>
#include <concurrent_queue.h>
#include <concurrent_vector.h>

template <typename T>
class MemoryPool_2 {
private:
	//concurrency::concurrent_queue<std::shared_ptr<T>> pool_;
	concurrency::concurrent_vector<std::shared_ptr<T>> pool_;
	using FactoryFunc = std::function<std::shared_ptr<T>()>;
	FactoryFunc factory_;

public:
	MemoryPool_2(size_t count, FactoryFunc factory) : factory_(factory) {
		for (size_t i = 0; i < count; ++i) {
			//pool_.push(factory_());
			pool_.push_back(factory_());
		}
	}

	~MemoryPool_2() {
		close();
	}

	void init(size_t count) {
		for (size_t i = 0; i < count; ++i) {
			//pool_.push(factory_());
			pool_.push_back(factory_());
		}
	}

	//std::shared_ptr<T> acquire() {
	//	std::shared_ptr<T> obj;
	//	/*if (!pool_.try_pop(obj)) {
	//		obj = factory_();
	//		pool_.push(obj);
	//	}*/
	//	if (pool_.empty()) {
	//		obj = factory_();
	//		pool_.push_back(obj);
	//	}
	//	else {
	//		obj = pool_.back();
	//		pool_.pop_back();
	//	}
	//	return obj;
	//}

	std::shared_ptr<T> acquire() {
		std::shared_ptr<T> obj;

		if (pool_.empty()) {
			obj = factory_();
			pool_.push_back(obj);
		}
		else {
			// 동기화 필요
			auto lastIndex = pool_.size() - 1;
			obj = pool_[lastIndex];

			// concurrent_vector는 pop_back이 없으므로, erase 사용 (주의: 느릴 수 있음)
			//pool_.erase(pool_.begin() + lastIndex);
			pool_.push_back(obj);
		}

		return obj;
	}


	void release(std::shared_ptr<T> obj) {
		//pool_.push(std::move(obj));
		pool_.push_back(std::move(obj));
	}

	void close() {
		std::shared_ptr<T> obj;
		

		//while (pool_.try_pop(obj)) {
		// 	obj.reset(); // 명시적으로 해제
		//}

		for (auto& obj : pool_) {
			obj.reset();
		}
		pool_.clear();
	}

	//concurrency::concurrent_queue<std::shared_ptr<T>>& getPool() {
	//	return pool_;
	//}

	concurrency::concurrent_vector<std::shared_ptr<T>>& getPool() {
		return pool_;
	}
};