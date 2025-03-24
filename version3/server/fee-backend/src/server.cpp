#include "../include/server.hpp"
#include "../include/packet.hpp"
#include "../include/session.hpp"
#include "../include/memory_pool.hpp"
#include "../include/utils.hpp"
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <chrono>
#include <iomanip>

extern std::atomic<int> Session_Count;

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
<<<<<<< HEAD
    size_t thread_count = std::thread::hardware_concurrency();
    std::cout << "Thread count: " << thread_count << std::endl;
=======
    size_t thread_count = std::thread::hardware_concurrency() / 2;
	std::cout << "Thread count: " << thread_count << std::endl;
>>>>>>> parent of cfe8a2e (í´ë¼ ë©”ëª¨ë¦¬ ë¦­ ë° ì„œë²„ sequence ìˆœì„œ ì²˜ë¦¬ ì™„ë£Œ -> í´ë¼ì´ì–¸íŠ¸ ì„¸ì…˜ ë³„ ì²˜ë¦¬ë˜ëŠ” ìŠ¤ë ˆë“œë¥¼ ì§€ì •í•¨ìœ¼ë¡œì¨ ìˆœì„œë¥¼ ë³´ì¥í•¨.)

    for (size_t i = 0; i < thread_count; ++i) {
        worker_threads.emplace_back([this]() {
            LOGI << "Worker thread started";
            while (is_running) {
                PacketTask task;  
                if (packet_queue.pop(task)) {
					processPacketInWorker(task.session_id, task.data, task.size);
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
        // io_context¸¦ ¿©·¯ ½º·¹µå¿¡¼­ ½ÇÇà
        std::vector<std::thread> io_threads;
        size_t thread_count = std::thread::hardware_concurrency() / 2;
        for (size_t i = 0; i < thread_count; ++i) {
            io_threads.emplace_back([this]() {
                io_context.run();
                });
        }
        // ¸ŞÀÎ ½º·¹µå¿¡¼­µµ io_context ½ÇÇà
        io_context.run();
        // ¸ğµç io_context ½º·¹µå°¡ Á¾·áµÉ ¶§±îÁö ´ë±â
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
					Session_Count++;
					session.get()->setSessionID(Session_Count);
					//LOGI << "New client connected";
                    clients.push_back(session);
                }
                session->start();
            }
            doAccept(acceptor); // ´ÙÀ½ ¿¬°á ´ë±â
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
