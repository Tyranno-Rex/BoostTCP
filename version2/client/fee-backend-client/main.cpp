#include "main.hpp"
#include "socket.hpp"
#include "utils.hpp"

//#include <windows.h>

std::atomic<bool> is_running = true;
std::mutex command_mutex;
std::mutex cout_mutex;
std::condition_variable command_cv;

bool new_message_ready = false;
std::string shared_message;
const char expected_hcv = 0x02;
const char expected_tcv = 0x03;

std::atomic<int> total_send_cnt = 0;
std::atomic<int> total_send_success_cnt = 0;
std::atomic<int> total_send_fail_cnt = 0;


void input_thread() {
    while (is_running) {
        std::string message;
        std::cout << "Enter command: ";
        std::getline(std::cin, message);

        {
            std::lock_guard<std::mutex> lock(command_mutex);
            shared_message = message;
            new_message_ready = true;
        }
        command_cv.notify_one();

        if (message == "/exit") {
            is_running = false;
            break;
        }
    }
}

void write_messages(boost::asio::io_context& io_context, const std::string& host, const std::string& port) {
    std::thread inputThread(input_thread);

    try {
        while (is_running) {
            std::unique_lock<std::mutex> lock(command_mutex);
            command_cv.wait(lock, [] { return new_message_ready; });

            std::string message = shared_message;
            new_message_ready = false;
            lock.unlock();

            if (message == "0") {
                SocketPool socket_pool(io_context, host, port, 1);
				std::string msg = "simple message";
                boost::asio::thread_pool pool(1);
                boost::asio::post(pool, [&socket_pool, msg]() {
                    handle_sockets(socket_pool, 1, msg, 0);
                    });
                pool.join();
            }
            else if (message == "1") {
                SocketPool socket_pool(io_context, host, port, 100);
				std::string msg = "You can't let your failures define you. You have to let your failures teach you. You have to let them show you what to do differently the next time.";
                boost::asio::thread_pool pool(10);
                boost::asio::post(pool, [&socket_pool, msg]() {
                    handle_sockets(socket_pool, 10000, msg, 0);
                    });
                pool.join();
            }
            else if (message.rfind("/debug", 0) == 0) {
                std::istringstream iss(message.substr(7));
                int thread_cnt, connection_cnt;
                if (!(iss >> thread_cnt >> connection_cnt) || thread_cnt <= 0 || connection_cnt <= 0) {
                    continue;
                }

				std::string msg = "You can't let your failures define you. You have to let your failures teach you. You have to let them show you what to do differently the next time.";
                while (is_running) {
                    try {
                        boost::asio::thread_pool pool(thread_cnt);
                        // int socket_cnt = rand() % 21 + 10;
                        int socket_cnt = 100;

                        SocketPool socket_pool(io_context, host, port, size_t(socket_cnt));

                        for (int i = 0; i < thread_cnt; ++i) {
                            boost::asio::post(pool, [&socket_pool, connection_cnt, msg, i]() {
                                handle_sockets(socket_pool, connection_cnt, msg, i);
                                });
                        }
                        pool.join();
                        socket_pool.close();
                        LOGI << total_send_cnt << " / " << total_send_success_cnt << " / " << total_send_fail_cnt
                            << " \t\tsuccess rate: " << (double)total_send_success_cnt / total_send_cnt * 100 << "\n";
					}
                    catch (std::exception& e) {
                        std::cerr << "Exception in debug mode: " << e.what() << std::endl;
                    }
                }
            }
            else if (message == "/clear") {
                system("cls");
            }
            else if (message == "/exit") {
                is_running = false;
                break;
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }

    inputThread.join();
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

    system("start cmd /k \"echo New Console Window & pause\"");
    std::cout << "Press any key to exit...\n";
    std::cin.get();

    try {
        boost::asio::io_context io_context;
        static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
        plog::init(plog::verbose, &consoleAppender);

        write_messages(io_context, host, chat_port);

        io_context.stop();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
