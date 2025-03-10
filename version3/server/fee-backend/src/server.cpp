#include "../include/server.hpp"
#include "../include/packet.hpp"
#include "../include/session.hpp"
#include "../include/memory_pool.hpp"
#include "../include/utils.hpp"
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <chrono>
#include <iomanip>

extern std::atomic<int> JH_recv_packet_total_cnt;
extern std::atomic<int> JY_recv_packet_success_cnt;
extern std::atomic<int> JY_recv_packet_fail_cnt;

extern std::atomic<int> YJ_recv_packet_total_cnt;
extern std::atomic<int> YJ_recv_packet_success_cnt;
extern std::atomic<int> YJ_recv_packet_fail_cnt;

extern std::atomic<int> ES_recv_packet_total_cnt;
extern std::atomic<int> ES_recv_packet_success_cnt;
extern std::atomic<int> ES_recv_packet_fail_cnt;

void Server::initializeThreadPool() {
    is_running = true;
    size_t thread_count = std::thread::hardware_concurrency() / 2;
	std::cout << "Thread count: " << thread_count << std::endl;

    for (size_t i = 0; i < thread_count; ++i) {
        worker_threads.emplace_back([this]() {
            LOGI << "Worker thread started";
            while (is_running) {
                PacketTask task;  
                if (packet_queue.pop(task)) {
					processPacketInWorker(task.data, task.size);
                }
            }
            });
    }
}

void Server::chatRun() {
    try {
        initializeThreadPool();
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
        doAccept(acceptor);
        // io_context�� ���� �����忡�� ����
        std::vector<std::thread> io_threads;
        size_t thread_count = std::thread::hardware_concurrency() / 2;
        for (size_t i = 0; i < thread_count; ++i) {
            io_threads.emplace_back([this]() {
                io_context.run();
                });
        }
        // ���� �����忡���� io_context ����
        io_context.run();
        // ��� io_context �����尡 ����� ������ ���
        for (auto& thread : io_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error in chatRun: " << e.what() << std::endl;
    }
}

void Server::doAccept(tcp::acceptor& acceptor) {
    acceptor.async_accept(
        [this, &acceptor](const boost::system::error_code& error, tcp::socket socket) {
            if (!error) {
				auto session = std::make_shared<Session>(std::move(socket), *this);
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
					LOGI << "New client connected";
                    clients.push_back(session);
                }
                session->start();
            }
            doAccept(acceptor); // ���� ���� ���
        });
}

void Server::removeClient(std::shared_ptr<Session> client) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(
        std::remove(clients.begin(), clients.end(), client),
        clients.end());
}

void Server::consoleStop() {
	io_context.stop();
}

void Server::chatStop() {
	for (auto& client : clients) {
		client->stop();
	}
    
	io_context.stop();
}
