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
	// �����ڿ��� �޸� ����� �ʱ�ȭ�մϴ�.
	MemoryPool(size_t PoolSize) : pool_size(PoolSize) {
		for (size_t i = 0; i < pool_size; i++) {
			memory_blocks.push(std::array<char, 1540>());
		}
	}

	// �޸� ����� �ʱ�ȭ�մϴ�.
	void init(size_t PoolSize) {
		pool_size = PoolSize;
		LOGI << "Memory pool init start";
		for (size_t i = 0; i < pool_size; i++) {
			memory_blocks.push(std::array<char, 1540>());
		}
		LOGI << "Memory pool init end";
	}

	// �޸� ����� �Ҵ��Ͽ� ��ȯ
	std::array<char, 1540> acquire() {
		std::unique_lock<std::mutex> lock(pool_mutex);
		while (memory_blocks.empty()) {
			pool_cv.wait(lock);  // ť�� ����� �� ��ٸ�
		}
		auto block = memory_blocks.front();
		memory_blocks.pop();
		return block;
	}

	// �޸� ����� ��ȯ
	void release(std::array<char, 1540>& block) {
		std::lock_guard<std::mutex> lock(pool_mutex);
		memset(block.data(), 0, block.size());
		memory_blocks.push(block);  // ��ȯ�� �޸� ����� ť�� �ٽ� �ֱ�
		pool_cv.notify_one();     // ��� ���� �����忡�� �˸�
	}

	// ���� �� �޸� ����� ��� ��ȯ
	void clear() {
		std::lock_guard<std::mutex> lock(pool_mutex);
		while (!memory_blocks.empty()) {
			memory_blocks.pop();
		}
		std::queue<std::array<char, 1540>> empty_queue;
		std::swap(memory_blocks, empty_queue);
	}
};