#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <thread>
#include <cstdint>
#include <zlib.h>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

const char expected_hcv = 0x02;
const char expected_tcv = 0x03;
std::mutex cout_mutex;
std::mutex command_mutex;
std::queue<std::string> command_queue;
std::condition_variable command_cond_var;
int total_connection = 0;

enum class PacketType : uint8_t {
    defEchoString = 100,
	JH = 101,
	YJ = 102,
	ES = 103,
};

#pragma pack(push, 1)
struct PacketHeader {
    PacketType type;           // 기본 : 100
    char checkSum[16];
    uint32_t size;
};

struct PacketTail {
    uint8_t value;
};

struct Packet {
    PacketHeader header;
    char payload[128];
    PacketTail tail;              // 기본 : 255
};
#pragma pack(pop)

std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data) {
    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(mdctx, EVP_md5(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize digest");
    }

    if (EVP_DigestUpdate(mdctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update digest");
    }

    unsigned int length = 0;
    if (EVP_DigestFinal_ex(mdctx, checksum.data(), &length) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(mdctx);
    return checksum;
}

// socket 관리 pool
class SocketPool {
private:
    boost::asio::io_context& io_context_;
    std::string host_;
    std::string port_;
    std::queue<std::shared_ptr<tcp::socket>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_var_;

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

};

std::string printMessageWithTime(const std::string& message, bool isDebug = false) {
    if (isDebug) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << message << std::endl;
		total_connection++;
        return message;
    }

	// 현재 시간을 구한다.
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;

	// 현재 시간을 tm 구조체로 변환
    std::time_t time_t_now = seconds.count();
    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);

	// 시간을 문자열로 변환
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%H:%M:%S");
    // 밀리초까지 추가함
    oss << "." << std::setfill('0') << std::setw(3) << milliseconds.count();

    std::string str_retrun = "[" + oss.str() + "] " + message;
    return str_retrun;
}

void handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num) {
    try {
        for (int i = 0; i < connection_cnt; ++i) {
            try {
				// 패킷 생성
                Packet packet;
                packet.header.type = PacketType::ES;
                packet.tail.value = 255;

				// 메시지를 payload에 복사
                std::string msg = std::to_string(thread_num) + ":" + std::to_string(i) + " " + message;
                msg = printMessageWithTime(msg);
				// 메시지를 최대 128자로 제한
                msg.resize(std::min(msg.size(), (size_t)128), ' ');

				// memset을 통해서 payload를 0으로 초기화
                std::memset(packet.payload, 0, sizeof(packet.payload));
				// memcpy를 통해서 payload에 메시지를 복사
                std::memcpy(packet.payload, msg.c_str(), std::min(msg.size(), (size_t)128));
				// header.size에 메시지의 크기를 저장
                packet.header.size = static_cast<uint32_t>(msg.size());

				// checksum을 계산하여 header.checkSum에 저장
                auto checksum = calculate_checksum(std::vector<char>(msg.begin(), msg.end()));
                std::memcpy(packet.header.checkSum, checksum.data(), MD5_DIGEST_LENGTH);

				// socket을 pool에서 가져온다.
                auto socket = socket_pool.acquire();
				// socket을 통해 패킷을 전송
                boost::asio::write(*socket, boost::asio::buffer(&packet, sizeof(packet)));
                // socket을 반환한다.
                socket_pool.release(socket);

                printMessageWithTime(msg, true);

            }
            catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "Error handling socket " << i << ": " << e.what() << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Error handling sockets: " << e.what() << std::endl;
    }
}

void write_messages(boost::asio::io_context& io_context, const std::string& host, const std::string& port) {
    try {
        SocketPool socket_pool(io_context, host, port, 10);

        while (true) {
            std::string message;
            int thread_cnt, connection_cnt;
			int cnt = 0;

            {
                std::unique_lock<std::mutex> lock(command_mutex);
                command_cond_var.wait(lock, [] { return !command_queue.empty(); });
                message = command_queue.front();
                command_queue.pop();
            }

            if (message == "1") {
                message = "It is a truth universally acknowledged, that a single man in possession of a good fortune, must be in want of a wife.However lit...";
                thread_cnt = 10;
                connection_cnt = 10;

                if (message.size() > 128) {
                    message = message.substr(0, 128);
                }

                boost::asio::thread_pool pool(thread_cnt);
                for (int i = 0; i < thread_cnt; ++i) {
                    boost::asio::post(pool, [&socket_pool, connection_cnt, message, i]() {
                        handle_sockets(socket_pool, connection_cnt, message, i);
                        });
                }
                pool.join();
            }
            else if (message == "2") {
                message = "In a quiet corner of the sprawling forest, where sunlight barely reached the ground, a small fox watched as shadows danced on the leaves.";
                thread_cnt = 10;
                connection_cnt = 50;

                if (message.size() > 128) {
                    message = message.substr(0, 128);
                }

                boost::asio::thread_pool pool(thread_cnt);
                for (int i = 0; i < thread_cnt; ++i) {
                    boost::asio::post(pool, [&socket_pool, connection_cnt, message, i]() {
                        handle_sockets(socket_pool, connection_cnt, message, i);
                        });
                }
                pool.join();
            }
            else if (message == "3") {
                message = "The sun was setting, casting a warm glow over the city as the last of the day's light faded away. the streets were quiet, the only sound the distant hum of traffic.";
                thread_cnt = 10;
                connection_cnt = 1000;

                if (message.size() > 128) {
                    message = message.substr(0, 128);
                }

                boost::asio::thread_pool pool(thread_cnt);
                for (int i = 0; i < thread_cnt; ++i) {
                    boost::asio::post(pool, [&socket_pool, connection_cnt, message, i]() {
                        handle_sockets(socket_pool, connection_cnt, message, i);
                        });
                }
                pool.join();
            }
			// /debug <thread count> <connection count> <message>
            else if (message.rfind("/debug", 0) == 0) {
                std::istringstream iss(message.substr(7));
                if (!(iss >> thread_cnt >> connection_cnt)) {
                    thread_cnt = 10;
                    connection_cnt = 10;
                }
				// 전달된 thread_cnt와 connection_cnt이 숫자값으로 들어왔는 지 확인
				if (thread_cnt <= 0 || connection_cnt <= 0) {
					thread_cnt = 10;
					connection_cnt = 10;
				}
                std::string debug_message;
                std::getline(iss, debug_message);
                debug_message = debug_message.empty() ? "default message" : debug_message;

                if (debug_message.size() > 128) {
                    debug_message = debug_message.substr(0, 128);
                }

                //while (true) {
                for (int i = 0; i < thread_cnt; ++i) {
                    boost::asio::thread_pool thread_pool_(thread_cnt);
                    for (int i = 0; i < thread_cnt; ++i) {
                        boost::asio::post(thread_pool_, [&socket_pool, connection_cnt, debug_message, i]() {
                            handle_sockets(socket_pool, connection_cnt, debug_message, i);
                            });
                    }
                    thread_pool_.join();
                }
                //}
            }
            else if (message == "/exit") {
                for (int i = 0; i < 10; ++i) {
                    auto socket = socket_pool.acquire();
                    socket->close();
                    socket_pool.release(socket);
                }
                break;
            }
            else {
                std::cout << "Invalid command.\n";
            }
        
            std::cout << "Total connection: " << total_connection << std::endl;
            total_connection = 0;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }
}

void consoleHandler() {
    while (true) {
		std::cout << "Enter command: 1 ~ 3 or /debug <message>\n";
        std::string message;
        std::getline(std::cin, message);
        {
            std::lock_guard<std::mutex> lock(command_mutex);
            command_queue.push(message);
        }
        command_cond_var.notify_one();
    }
}

int main(int argc, char* argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    std::string host;
    std::string chat_port;
    std::string number;

    std::cout << "Enter the address:\n0. remote ES server \n1. local ES Server\n2. remote YJ server\n3. local YJ Server\n4. remote JH server\n5. local JH server\n";
    std::getline(std::cin, number);

    if (number == "0") {
        host = "192.168.21.96";
        chat_port = "3571";
    }
    if (number == "1") {
        host = "127.0.0.1";
        chat_port = "3572";
    }
    else if (number == "2") {
        host = "192.168.20.158";
        chat_port = "27931";
    }
    else if (number == "3") {
        host = "127.0.0.1";
        chat_port = "27931";
    }
    else if (number == "4") {
        host = "192.168.";
        chat_port = "";
    }
    else if (number == "5") {
        host = "127.0.0.1";
        chat_port = "";
    }
    else {
        host = "127.0.0.1";
        chat_port = "3572";
        std::cout << "Invalid input. Using default(local) address.\n";
    }

    try {
        boost::asio::io_context io_context;
        std::thread consoleThread(consoleHandler);
        std::thread writeThread(write_messages, std::ref(io_context), host, chat_port);
        consoleThread.join();
        writeThread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}