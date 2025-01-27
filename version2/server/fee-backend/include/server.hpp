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
#include "packet.hpp"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#define GET (http::verb::get)
#define POST (http::verb::post)
#define PUT (http::verb::put)
#define PATCH (http::verb::patch)
#define DELETE (http::verb::delete_)

#define expected_hcv 0x02
#define expected_tcv 0x03
#define expected_checksum 0x04
#define expected_size 0x08


class MemoryPool {
private:
	std::queue<std::array<char, 150>> memory_blocks;
    std::mutex pool_mutex;
    std::condition_variable pool_cv;
    size_t pool_size;

public:
    // 생성자에서 메모리 블록을 초기화합니다.
    MemoryPool(size_t PoolSize) : pool_size(PoolSize) {
        for (size_t i = 0; i < pool_size; i++) {
			memory_blocks.push(std::array<char, 150>());
        }
    }

    // 메모리 블록을 할당하여 반환
	std::array<char, 150> acquire() {
		std::unique_lock<std::mutex> lock(pool_mutex);
		while (memory_blocks.empty()) {
			pool_cv.wait(lock);  // 큐가 비었을 때 기다림
		}
		auto block = memory_blocks.front();
		memory_blocks.pop();
		return block;
	}

	// 메모리 블록을 반환
	void release(std::array<char, 150>& block) {
		std::lock_guard<std::mutex> lock(pool_mutex);
		memset(block.data(), 0, block.size());
		memory_blocks.push(block);  // 반환된 메모리 블록을 큐에 다시 넣기
		pool_cv.notify_one();     // 대기 중인 스레드에게 알림
	}
};


class ClientSession;

class Server {
private:
    boost::asio::io_context& io_context;
    unsigned short port;
    std::vector<std::shared_ptr<ClientSession>> clients;
    std::mutex clients_mutex;

public:
    Server(boost::asio::io_context& io_context, unsigned short port)
        : io_context(io_context), port(port) {
    }

    void chatRun();
    void doAccept(tcp::acceptor& acceptor);
    void removeClient(std::shared_ptr<ClientSession> client);
};

// 클라이언트 세션을 관리하는 클래스
class ClientSession : public std::enable_shared_from_this<ClientSession> {
private:
    tcp::socket socket;
    Server& server;
    std::array<char, 150> current_buffer;   // 현재 읽기용 버퍼
    std::array<char, 150> packet_buffer;    // 패킷 조립용 버퍼
    size_t packet_buffer_offset = 0;        // 패킷 버퍼의 현재 위치

public:
    //ClientSession(tcp::socket socket_, Server& server_)
    //    : socket(std::move(socket_)), server(server_) {
    //}

	ClientSession(tcp::socket socket_, Server& server_)
        : socket(std::move(socket_))
		, server(server_) {
    }

    ~ClientSession() {
    }

    void start() {
        doRead();
    }
private:
    void doRead();
	bool handlePacket(size_t bytes_transferred);
};

