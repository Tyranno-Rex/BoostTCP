#include "../include/server.hpp"
#include "../include/packet.hpp"
#include "../include/session.hpp"
#include "../include/memory_pool.hpp"
#include "../include/utils.hpp"
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <chrono>
#include <iomanip>

void Server::chatRun() {
    try {
        tcp::acceptor acceptor(io_context,
            tcp::endpoint(tcp::v4(), port));
        doAccept(acceptor);
        io_context.run();
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
					std::cout << "New client connected" << std::endl;
                    clients.push_back(session);
                }
                session->start();
            }
            doAccept(acceptor); // 다음 연결 대기
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
