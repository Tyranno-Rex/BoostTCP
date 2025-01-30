#include "main.hpp"
#include "socket.hpp"

std::queue<std::string> command_queue;
std::condition_variable command_cond_var;
int total_connection = 0;
const char expected_hcv = 0x02;
const char expected_tcv = 0x03;
std::mutex cout_mutex;
std::mutex command_mutex;


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
            
            if (message == "1") {
                message = "It is a truth universally acknowledged, that a single man in possession of a good fortune, must be in want of a wife.However lit...";
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
            else if (message.rfind("/debug", 0) == 0) {
                std::istringstream iss(message.substr(7));
                if (!(iss >> thread_cnt >> connection_cnt)) {
                    continue;
                }
				// 전달된 thread_cnt와 connection_cnt이 숫자값으로 들어왔는 지 확인
				if (thread_cnt <= 0 || connection_cnt <= 0) {
					continue;
				}

				std::string message = "It is a truth universally acknowledged, that a single man in possession of a good fortune, must be in want of a wife.However lit...";

				if (message.size() > 128) {
					message = message.substr(0, 128);
				}

                while (true) {
                    boost::asio::thread_pool thread_pool_(thread_cnt);
					// 10 ~ 30 사이의 랜덤한 소켓 개수를 생성
					int socket_cnt = rand() % 21 + 10;
					SocketPool socket_pool_rand(io_context, host, port, size_t(socket_cnt));
                    for (int i = 0; i < thread_cnt; ++i) {
                        boost::asio::post(thread_pool_, [&socket_pool_rand, connection_cnt, message, i]() {
                            handle_sockets(socket_pool_rand, connection_cnt, message, i);
                            });
                    }
                    thread_pool_.join();
					socket_pool_rand.close();
					Sleep(10);
                }
            }
            else if (message == "/") {
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
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }
}

void consoleHandler() {
    while (true) {
		std::cout << "Enter command: 0~1 or /debug <thread_cnt> <connection_cnt> or /exit\n";
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

	std::cout << "Enter the address:\n1. JH server\n2. YJ server\n3. ES server\n";
    std::getline(std::cin, number);

    if (number == "1") {
        host = "192.168.20.241";
        chat_port = "7777";
    }
    else if (number == "2") {
        host = "192.168.20.158";
        chat_port = "7777";
    }
    else if (number == "3") {
        host = "192.168.21.96";
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