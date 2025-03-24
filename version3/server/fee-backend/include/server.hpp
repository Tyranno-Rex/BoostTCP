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

// Session ¿¸πÊ º±æ
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

struct PacketTask {
    std::unique_ptr<std::vector<char>> data;
	uint32_t session_id;
	uint32_t seq_num;
    size_t size;

	PacketTask() = default;

    //PacketTask(std::unique_ptr<std::vector<char>>&& buffer, size_t s)
    //    : data(std::move(buffer))
    //    , size(s) {
    //}

	PacketTask(std::unique_ptr<std::vector<char>>&& buffer, size_t s, uint32_t session_id, uint32_t sequence)
		: data(std::move(buffer))
		, size(s)
		, session_id(session_id)
		, seq_num(sequence) {
	}

    //PacketTask(PacketTask&& other) noexcept
    //    : data(std::move(other.data))
    //    , size(other.size) {
    //}

    PacketTask(PacketTask&& other) noexcept
        : data(std::move(other.data)), size(other.size),
        session_id(other.session_id), seq_num(other.seq_num) {
    }

    PacketTask& operator=(PacketTask&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
            size = other.size;
            session_id = other.session_id;  // √ﬂ∞°
            seq_num = other.seq_num;        // √ﬂ∞°
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

    std::vector<std::thread> worker_threads;
    PacketQueue packet_queue;
<<<<<<< HEAD

    // ∞¢ worker∏∂¥Ÿ ¿¸øÎ task queue, mutex, condition_variable ª˝º∫
	std::vector<PacketQueue> worker_task_queues;
    std::vector<std::unique_ptr<std::mutex>> worker_task_mutexes;
    std::vector<std::unique_ptr<std::condition_variable>> worker_task_cvs;


=======
>>>>>>> parent of cfe8a2e (ÌÅ¥Îùº Î©îÎ™®Î¶¨ Î¶≠ Î∞è ÏÑúÎ≤Ñ sequence ÏàúÏÑú Ï≤òÎ¶¨ ÏôÑÎ£å -> ÌÅ¥ÎùºÏù¥Ïñ∏Ìä∏ ÏÑ∏ÏÖò Î≥Ñ Ï≤òÎ¶¨ÎêòÎäî Ïä§Î†àÎìúÎ•º ÏßÄÏ†ïÌï®ÏúºÎ°úÏç® ÏàúÏÑúÎ•º Î≥¥Ïû•Ìï®.)
    std::atomic<bool> is_running{ false };
public:
    Server(boost::asio::io_context& io_context, unsigned short port)
        : io_context(io_context), port(port) {
    }

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
    void removeClient(std::shared_ptr<Session> client);
    void consoleStop();
    void chatStop();
};