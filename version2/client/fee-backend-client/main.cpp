#include "main.hpp"
#include "socket.hpp"

std::queue<std::string> command_queue;
std::condition_variable command_cond_var;
int total_connection = 0;
const char expected_hcv = 0x02;
const char expected_tcv = 0x03;
std::mutex cout_mutex;
std::mutex command_mutex;

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

std::string printMessageWithTime(const std::string& message, bool isDebug) {
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

void write_messages(boost::asio::io_context& io_context, const std::string& host, const std::string& port) {
    try {
        SocketPool socket_pool(io_context, host, port, 100);

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
            if (message == "0") {
				message = "Hello, world!";
				thread_cnt = 1;
				connection_cnt = 1;

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
            else if (message == "1") {
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

                while (true) {
                    boost::asio::thread_pool thread_pool_(thread_cnt);
                    for (int i = 0; i < thread_cnt; ++i) {
                        boost::asio::post(thread_pool_, [&socket_pool, connection_cnt, debug_message, i]() {
                            handle_sockets(socket_pool, connection_cnt, debug_message, i);
                            });
                    }
                    thread_pool_.join();
                }
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
        chat_port = "7777";
    }
    if (number == "1") {
        host = "127.0.0.1";
        chat_port = "7777";
    }
    else if (number == "2") {
        host = "192.168.20.158";
        chat_port = "7777";
    }
    else if (number == "3") {
        host = "127.0.0.1";
        chat_port = "7777";
    }
    else if (number == "4") {
        host = "192.168.";
        chat_port = "7777";
    }
    else if (number == "5") {
        host = "127.0.0.1";
        chat_port = "7777";
    }
    else {
        host = "127.0.0.1";
        chat_port = "7777";
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