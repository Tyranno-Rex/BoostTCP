#include "main.hpp"
#include "socket.hpp"

//int total_connection = 0;
const char expected_hcv = 0x02;
const char expected_tcv = 0x03;
std::mutex cout_mutex;
std::mutex command_mutex;

std::atomic<bool> is_running = true;

int total_send_cnt = 0;
int total_send_success_cnt = 0;
int total_send_fail_cnt = 0;

void write_messages(boost::asio::io_context& io_context, const std::string& host, const std::string& port) {
    try {

        while (true) {
            std::string message;
            int thread_cnt, connection_cnt;
            int cnt = 0;


            LOGI << "Total send: " << 1000 * 10;
            LOGI << "Total send success: " << total_send_success_cnt;
            LOGI << "Total send fail: " << total_send_fail_cnt;

			total_send_cnt = 0;
			total_send_success_cnt = 0;
			total_send_fail_cnt = 0;
			std::cout << "Enter the message 1 or /debug [thread_cnt] [connection_cnt] or /clear or /exit (wrong input will exit the program)\n";
			std::getline(std::cin, message);
            if (message == "0") {
                SocketPool socket_pool(io_context, host, port, 1);
                message = "It is a .";
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
            if (message == "1") {
                SocketPool socket_pool(io_context, host, port, 100);
                message = "It is a truth universally acknowledged, that a single man in possession of a good fortune, must be in want of a wife.However lit...";
                thread_cnt = 10;
                connection_cnt = 10000;

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

#include <windows.h>
#include <string>

// Convert a narrow string to a wide string
std::wstring stringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

int main(int argc, char* argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    std::string host;
    std::string chat_port;
    std::string number;

    wchar_t cBuf[1024] = { 0 }; // 초기화
    memset(cBuf, 0, 1024);

    std::cout << "Enter the address:\n1. JH server\n2. YJ server\n3. ES server\n";
    std::getline(std::cin, number);

    std::wstring section = stringToWString("SERVER");
    std::wstring key;
    if (number == "1") {
        key = stringToWString("JH");
    }
    else if (number == "2") {
        key = stringToWString("YJ");
    }
    else if (number == "3") {
        key = stringToWString("ES");
    }
    else {
        key = stringToWString("Default");
        std::cout << "Invalid input. Using default(local) address.\n";
    }
    std::wstring file_path = stringToWString(".\\config.ini");
    std::wstring default_value = stringToWString(".");

    GetPrivateProfileString(section.c_str(), key.c_str(), default_value.c_str(), cBuf, 1024, file_path.c_str());
    std::wstring host_wstr(cBuf);
    host = std::string(host_wstr.begin(), host_wstr.end());

    memset(cBuf, 0, sizeof(cBuf));  // 버퍼 초기화
    key = L"PORT";

    GetPrivateProfileString(section.c_str(), key.c_str(), default_value.c_str(), cBuf, 1024, file_path.c_str());
    std::wstring port_wstr(cBuf);
    chat_port = std::string(port_wstr.begin(), port_wstr.end());

    std::cout << "Host: " << host << std::endl;
	std::cout << "Port: " << chat_port << std::endl;
    try {
        boost::asio::io_context io_context;
        static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
        plog::init(plog::verbose, &consoleAppender);

        write_messages(io_context, host, chat_port);

        while (is_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        io_context.stop();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
