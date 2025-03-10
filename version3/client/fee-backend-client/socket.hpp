#pragma once
#include "main.hpp"
#include "memory_pool.hpp" // MemoryPool ��� ���� ����

extern std::mutex cout_mutex;
extern std::mutex command_mutex;

using boost::asio::ip::tcp;

class Socket {
private:
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::string host_;
    std::string port_;
    tcp::resolver::results_type endpoints;


public:
    // �����ڿ��� io_context�� ȣ��Ʈ ������ �޾Ƽ� ���� �ʱ�ȭ
    Socket(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
        : io_context_(io_context), socket_(io_context), host_(host), port_(port) {
        tcp::resolver resolver(io_context_);
        endpoints = resolver.resolve(host_, port_);
    }

	void print() {
		std::cout << "Socket: " << host_ << ":" << port_ << std::endl;
	}

    // ���� ����
    void connect(int retry_count = 5, int retry_delay_ms = 1000) {
        //for (int i = 0; i < retry_count; ++i) {
        //    try {
        //        boost::asio::connect(socket_, endpoints); // connect �޼��忡���� ���Ḹ ����
        //        return; // ���� ���� �� �Լ� ����
        //    }
        //    catch (const std::exception& e) {
        //        std::cerr << "Socket connection error: " << e.what() << std::endl;
        //        std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms)); // ���� �ð� �� ��õ�
        //    }
        //}
        //throw std::runtime_error("Failed to connect after multiple attempts");

        try {
			boost::asio::connect(socket_, endpoints); // connect �޼��忡���� ���Ḹ ����
        }
		catch (const std::exception& e) {
			std::cerr << "Socket connection error: " << e.what() << std::endl;
		}
    
    }

	void disconnect() {
		if (socket_.is_open()) {
			boost::system::error_code ec;
			socket_.shutdown(tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		}
	}


    // ������ MemoryPool���� ������ �� �ֵ��� ����
    void reset() {
        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.shutdown(tcp::socket::shutdown_both, ec);
            socket_.close(ec);
        }
        connect(); // �翬��
    }

    std::shared_ptr<tcp::socket> get_shared_socket() {
        return std::shared_ptr<tcp::socket>(&socket_, [](tcp::socket*) {});
    }


};

void handle_sockets(MemoryPool<Socket>& socket_pool, int connection_cnt, const std::string& message, int thread_num);
