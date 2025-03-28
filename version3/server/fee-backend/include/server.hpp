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

#define CLIENT_CONNECTION_MAINTENANCE_INTERVAL 30

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
	std::thread client_check_thread;


    PacketQueue packet_queue;

 //   // 각 worker마다 전용 task queue, mutex, condition_variable 생성
	std::vector<PacketQueue> worker_task_queues;

 //   //std::vector<std::mutex> worker_task_mutexes;
 //   //std::vector<std::condition_variable> worker_task_cvs;
 //   std::vector<std::unique_ptr<std::mutex>> worker_task_mutexes;
 //   std::vector<std::unique_ptr<std::condition_variable>> worker_task_cvs;


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