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

#include <plog/Log.h>
#include <plog/init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/DebugOutputAppender.h>

#include "packet.hpp"
#include "memory_pool.hpp"
#include "session.hpp"

// Session 전방 선언
class Session;

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

//void processPacketInWorker(PacketTask task);

struct PacketTask {
    std::unique_ptr<std::vector<char>> data;
	Session* session;
    uint32_t session_id;
	//uint32_t seq_num;
    size_t size;

	PacketTask() = default;

	//PacketTask(std::unique_ptr<std::vector<char>>&& buffer, size_t s, uint32_t session_id, uint32_t sequence)
	//	: data(std::move(buffer))
	//	, size(s)
	//	, session_id(session_id)
	//	, seq_num(sequence) {
	//}
    
    //PacketTask(PacketTask&& other) noexcept
    //    : data(std::move(other.data)), size(other.size),
    //    session_id(other.session_id), seq_num(other.seq_num) {
    //}

    //PacketTask& operator=(PacketTask&& other) noexcept {
    //    if (this != &other) {
    //        data = std::move(other.data);
    //        size = other.size;
    //        session_id = other.session_id;  // 추가
    //        seq_num = other.seq_num;        // 추가
    //    }
    //    return *this;
    //}

	PacketTask(std::unique_ptr<std::vector<char>>&& buffer, size_t s, uint32_t session_id, Session* session)
		: data(std::move(buffer))
		, size(s)
		, session_id(session_id)
		, session(session) {
	}

	PacketTask(PacketTask&& other) noexcept
		: data(std::move(other.data)), size(other.size),
		session_id(other.session_id), session(other.session) {
	}

	PacketTask& operator=(PacketTask&& other) noexcept {
		if (this != &other) {
			data = std::move(other.data);
			size = other.size;
			session_id = other.session_id;  // 추가
			session = other.session;        // 추가
		}
		return *this;
	}

    PacketTask(const PacketTask&) = delete;
    PacketTask& operator=(const PacketTask&) = delete;
};

class PacketQueue {
private:
    std::queue<PacketTask> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    bool stopping = false;
    size_t max_queue_size = 1000000;

public:
    PacketQueue() = default;

    // 복사 생성자/대입 연산은 삭제
    PacketQueue(const PacketQueue&) = delete;
    PacketQueue& operator=(const PacketQueue&) = delete;

    // move 생성자: 내부의 tasks와 상태만 이동, mutex와 condition은 새로 기본 생성됨
    PacketQueue(PacketQueue&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex);
        tasks = std::move(other.tasks);
        stopping = other.stopping;
        max_queue_size = other.max_queue_size;
        // mutex와 condition은 기본 상태로 남음.
    }

    PacketQueue& operator=(PacketQueue&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock_this(mutex);
            std::lock_guard<std::mutex> lock_other(other.mutex);
            tasks = std::move(other.tasks);
            stopping = other.stopping;
            max_queue_size = other.max_queue_size;
            // mutex와 condition은 그대로.
        }
        return *this;
    }


    bool push(PacketTask&& task) {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this] { return tasks.size() < max_queue_size || stopping; });
        if (stopping) {
			LOGE << "PUSH PacketQueue is stopping";
            return false;
        }
        tasks.push(std::move(task));
        condition.notify_one();
        return true;
    }

    bool pop(PacketTask& task) {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this] { return !tasks.empty() || stopping; });
        if (stopping && tasks.empty()) {
			LOGE << "POP PacketQueue is stopping";
            return false;
        }

        task = std::move(tasks.front());
        tasks.pop();
        condition.notify_one();
        return true;
    }

    void stop() {
        std::unique_lock<std::mutex> lock(mutex);
        stopping = true;
        condition.notify_all();
    }

    void clear() {
        std::unique_lock<std::mutex> lock(mutex);
        std::queue<PacketTask> empty;
        tasks.swap(empty);
    }
};

class Server {
private:
    boost::asio::io_context& io_context;
    unsigned short port;
    std::vector<std::shared_ptr<Session>> clients;
    std::mutex clients_mutex;
    MemoryPool_2<Session> session_pool;
	std::unordered_map<uint32_t, std::shared_ptr<Session>> session_map;

    std::vector<std::thread> worker_threads;
	std::thread pop_thread;


    PacketQueue packet_queue;

    // 각 worker마다 전용 task queue, mutex, condition_variable 생성
	std::vector<PacketQueue> worker_task_queues;
    //std::vector<std::mutex> worker_task_mutexes;
    //std::vector<std::condition_variable> worker_task_cvs;
    std::vector<std::unique_ptr<std::mutex>> worker_task_mutexes;
    std::vector<std::unique_ptr<std::condition_variable>> worker_task_cvs;


    std::atomic<bool> is_running{ false };

public:
    Server(boost::asio::io_context& io_context, unsigned short port)
        : io_context(io_context), port(port),
        session_pool(1000, [this, &io_context]() { return std::make_shared<Session>(io_context, *this); }) 
    {
    }

    // 복사 생성자와 복사 대입 연산자를 삭제해 non-copyable로 만듦
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void initializeThreadPool();

    void stopThreadPool() {
        is_running = false;
        packet_queue.stop();

        for (auto& thread : worker_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        worker_threads.clear();
    }

    PacketQueue& getPacketQueue() {
        return packet_queue;
    }

    void chatRun();
    void doAccept(tcp::acceptor& acceptor);
	//void doAccept(tcp::acceptor& acceptor, MemoryPool_2<Session>& session_pool);
    void removeClient(std::shared_ptr<Session> client);
    void consoleStop();
    void chatStop();
};