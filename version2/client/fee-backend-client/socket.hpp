#pragma once
#include "main.hpp"
#include "socket.hpp"

extern std::mutex cout_mutex;
extern std::mutex command_mutex;

// socket 관리 pool
class SocketPool {
private:
    boost::asio::io_context& io_context_;
    std::string host_;
    std::string port_;
    std::queue<std::shared_ptr<tcp::socket>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
	std::atomic<bool> is_closing_{ false };// pool이 닫히고 있는지 여부

public:
    // 생성자에서 pool_size만큼 socket을 생성하여 pool에 넣는다.
    SocketPool(boost::asio::io_context& io_context, const std::string& host, const std::string& port, std::size_t pool_size)
        : io_context_(io_context), host_(host), port_(port) {
        for (std::size_t i = 0; i < pool_size; ++i) {
            // make_shared를 사용하여 shared_ptr 생성
            auto socket = std::make_shared<tcp::socket>(io_context_);
            // resolver를 통해 host, port로 endpoint를 얻어 connect
            tcp::resolver resolver(io_context_);
            // resolver.resolve(host, port)로 endpoint를 얻어 connect
            auto endpoints = resolver.resolve(host_, port_);
            // connect 동기로 수행
            boost::asio::connect(*socket, endpoints);
            // pool에 socket을 넣는다.
            pool_.push(socket);
        }
    }

    ~SocketPool() {
		std::cout << "SocketPool is destroyed" << std::endl;
        close();
    }

    // pool에서 socket을 가져온다.
    // 자료형은 shared_ptr<tcp::socket>로 tcp::socket을 가리키는 shared_ptr이다.
    std::shared_ptr<tcp::socket> acquire() {
        // mutex를 사용하여 pool에 대한 동시성 제어
        std::unique_lock<std::mutex> lock(mutex_);
        // pool이 비어있으면 대기
        while (pool_.empty()) {
            // 신호가 오게 되면 대기를 풀고 다시 확인
            cond_var_.wait(lock);
        }
        // pool에서 socket을 가져오는데, 이는 맨 앞에서 가져오고 pop한다.
        auto socket = pool_.front();
        pool_.pop();
        // socket을 반환
        return socket;
    }

    // pool에 socket을 반환한다.
    void release(std::shared_ptr<tcp::socket> socket) {
        // mutex를 사용하여 pool에 대한 동시성 제어
        std::unique_lock<std::mutex> lock(mutex_);
        // pool에 socket을 넣는다.
        pool_.push(socket);
        // 신호를 보내어 대기중인 스레드를 깨운다.
        cond_var_.notify_one();
    }


	// pool에 있는 모든 socket을 받고 반환한다.
    void close() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (is_closing_) return;  // 이미 종료 중이면 리턴

        is_closing_ = true;
        cond_var_.notify_all();  // 대기 중인 모든 스레드 깨우기

        while (!pool_.empty()) {
			std::cout << "SocketPool::close() - pool size: " << pool_.size() << std::endl;
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

        if (!io_context_.stopped()) {
            io_context_.stop();
        }
    }
};

void handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num);
