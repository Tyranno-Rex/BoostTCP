#pragma once
#include "main.hpp"
#include "socket.hpp"

extern std::mutex cout_mutex;
extern std::mutex command_mutex;

// socket ���� pool
class SocketPool {
private:
    boost::asio::io_context& io_context_;
    std::string host_;
    std::string port_;
    std::queue<std::shared_ptr<tcp::socket>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
	std::atomic<bool> is_closing_{ false };// pool�� ������ �ִ��� ����

public:
    SocketPool();

    // �����ڿ��� pool_size��ŭ socket�� �����Ͽ� pool�� �ִ´�.
    SocketPool(boost::asio::io_context& io_context, const std::string& host, const std::string& port, std::size_t pool_size)
        : io_context_(io_context), host_(host), port_(port) {
        for (std::size_t i = 0; i < pool_size; ++i) {
            try {
                // make_shared�� ����Ͽ� shared_ptr ����
                auto socket = std::make_shared<tcp::socket>(io_context_);
                // resolver�� ���� host, port�� endpoint�� ��� connect
                tcp::resolver resolver(io_context_);
                // resolver.resolve(host, port)�� endpoint�� ��� connect
                auto endpoints = resolver.resolve(host_, port_);
                // connect ����� ����
                boost::asio::connect(*socket, endpoints);
                // pool�� socket�� �ִ´�.
                pool_.push(socket);
            }
			catch (const std::exception& e) {
				LOGE << "Error creating socket: " << e.what();
				close();
            }
        }
    }

    ~SocketPool() {
		std::cout << "SocketPool is destroyed" << std::endl;
        close();
    }

    // pool���� socket�� �����´�.
    // �ڷ����� shared_ptr<tcp::socket>�� tcp::socket�� ����Ű�� shared_ptr�̴�.
    std::shared_ptr<tcp::socket> acquire() {
        // mutex�� ����Ͽ� pool�� ���� ���ü� ����
        std::unique_lock<std::mutex> lock(mutex_);
        // pool�� ��������� ���
        while (pool_.empty()) {
            // ��ȣ�� ���� �Ǹ� ��⸦ Ǯ�� �ٽ� Ȯ��
            cond_var_.wait(lock);
        }
        // pool���� socket�� �������µ�, �̴� �� �տ��� �������� pop�Ѵ�.
        auto socket = pool_.front();
        pool_.pop();
        // socket�� ��ȯ
        return socket;
    }

    // pool�� socket�� ��ȯ�Ѵ�.
    void release(std::shared_ptr<tcp::socket> socket) {
        // mutex�� ����Ͽ� pool�� ���� ���ü� ����
        std::unique_lock<std::mutex> lock(mutex_);
        // pool�� socket�� �ִ´�.
        pool_.push(socket);
        // ��ȣ�� ������ ������� �����带 �����.
        cond_var_.notify_one();
    }


	// pool�� �ִ� ��� socket�� �ް� ��ȯ�Ѵ�.
    void close() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (is_closing_) return;  // �̹� ���� ���̸� ����

        is_closing_ = true;
        cond_var_.notify_all();  // ��� ���� ��� ������ �����

        while (!pool_.empty()) {
            auto socket = pool_.front();
            pool_.pop();
            try {
                if (socket && socket->is_open()) {
                    socket->close();
                }
            }
            catch (...) {

            }
        }

		if (pool_.empty()) {
			std::cout << "All sockets are closed" << std::endl;
		}

        if (!io_context_.stopped()) {
            io_context_.stop();
        }
    }

	// �����ִ� ������ ������ ��ȯ
    std::size_t open_socket_count() {
        std::unique_lock<std::mutex> lock(mutex_);
        std::size_t count = 0;
        std::queue<std::shared_ptr<tcp::socket>> temp_pool = pool_;
        while (!temp_pool.empty()) {
            auto socket = temp_pool.front();
            temp_pool.pop();
            if (socket && socket->is_open()) {
                count++;
            }
        }
        return count;
    }
};

//void handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num);
//void handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num, std::shared_ptr<std::atomic<int>> pending_ops);


class SocketHandler : public std::enable_shared_from_this<SocketHandler> {
public:
    SocketHandler(SocketPool& socket_pool, int connection_cnt, const std::string& message, int thread_num, std::shared_ptr<std::atomic<int>> pending_ops)
        : socket_pool_(socket_pool), connection_cnt_(connection_cnt), message_(message), thread_num_(thread_num), pending_ops_(pending_ops) {
    }

    void handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num, std::shared_ptr<std::atomic<int>> pending_ops);

private:
    SocketPool& socket_pool_;
    int connection_cnt_;
    std::string message_;
    int thread_num_;
    std::shared_ptr<std::atomic<int>> pending_ops_;
};