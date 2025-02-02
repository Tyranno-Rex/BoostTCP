#include "main.hpp"
#include "socket.hpp"

int total_connection = 0;
const char expected_hcv = 0x02;
const char expected_tcv = 0x03;
std::mutex cout_mutex;
std::mutex command_mutex;

std::atomic<bool> is_running = true;

void write_messages(boost::asio::io_context& io_context, const std::string& host, const std::string& port) {
    try {

        while (true) {
            std::string message;
            int thread_cnt, connection_cnt;
            int cnt = 0;
            
			std::cout << "Enter the message 1 or /debug [thread_cnt] [connection_cnt] or /clear or /exit\n";
			std::getline(std::cin, message);
            if (message == "1") {
                SocketPool socket_pool(io_context, host, port, 100);
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
					Sleep(1);
                }
            }
            else if (message == "/clear") {
				system("cls");
            }
            else if (message == "/exit") {
				is_running = false;
                break;
            }
            else {
				is_running = false;
				break;
            }        
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    //_CrtSetBreakAlloc(2836);
    atexit([] { _CrtDumpMemoryLeaks(); });

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
        std::thread writeThread(write_messages, std::ref(io_context), host, chat_port);

		while (is_running) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

        writeThread.join();
		io_context.stop();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}