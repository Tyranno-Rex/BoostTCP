#pragma once
#include "main.hpp"
#include "memory_pool.hpp" // MemoryPool 헤더 파일 포함

extern std::mutex cout_mutex;
extern std::mutex command_mutex;

using boost::asio::ip::tcp;

class Socket {
private:
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::string host_;
    std::string port_;

public:
    // 생성자에서 io_context와 호스트 정보를 받아서 소켓 초기화
    Socket(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
        : io_context_(io_context), socket_(io_context), host_(host), port_(port) {
    }

	void print() {
		std::cout << "Socket: " << host_ << ":" << port_ << std::endl;
	}

    // 소켓 연결
    void connect() {
        try {
            tcp::resolver resolver(io_context_);
            auto endpoints = resolver.resolve(host_, port_);
            boost::asio::connect(socket_, endpoints);
        }
        catch (const std::exception& e) {
            std::cerr << "Socket connection error: " << e.what() << std::endl;
        }
    }

    // 소켓을 MemoryPool에서 재사용할 수 있도록 리셋
    void reset() {
        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.shutdown(tcp::socket::shutdown_both, ec);
            socket_.close(ec);
        }
        connect(); // 재연결
    }

    /*std::shared_ptr<tcp::socket> get_shared_socket() {
        return std::make_shared<tcp::socket>(std::move(socket_));
    }*/

    std::shared_ptr<tcp::socket> get_shared_socket() {
        return std::shared_ptr<tcp::socket>(&socket_, [](tcp::socket*) {});
    }


};

void handle_sockets(MemoryPool<Socket>& socket_pool, int connection_cnt, const std::string& message, int thread_num);
