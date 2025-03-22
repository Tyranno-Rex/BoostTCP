#include "handle_messages.hpp"
#include "globals.hpp"
#include "input.hpp"
#include "log_console.hpp"
#include "socket.hpp"
#include "utils.hpp"
#include "memory_pool.hpp"

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <thread>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#include <tchar.h>
#endif


// trim from start (in place)
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}


void write_messages(boost::asio::io_context& io_context, const std::string& host, const std::string& port) {

    try {
        while (is_running) {
            std::unique_lock<std::mutex> lock(command_mutex);
            command_cv.wait(lock, [] { return new_message_ready; });

            std::string message = shared_message;
            new_message_ready = false;
            lock.unlock();
            
			trim(message);
			std::cout << "Received command: " << message << std::endl;

			// 특정 개수만큼만 메시지 전송
            if (message.rfind("/send", 0) == 0) {
                std::istringstream iss(message.substr(6));
                int send_cnt, thread_cnt;
				if (!(iss >> send_cnt >> thread_cnt) || send_cnt <= 0 || thread_cnt <= 0) {
                    continue;
                }

                std::string msg =
                    "You can't let your failures define you. "
                    "You have to let your failures teach you. "
                    "You have to let them show you what to do differently the next time.";

                int socket_cnt = 10000;

                MemoryPool<Socket> socket_pool(socket_cnt,
                    [&io_context, &host, &port]() {
                        return std::make_shared<Socket>(io_context, host, port);
                    }
                );
                boost::asio::thread_pool pool(thread_cnt);

                int send_cnt_per_thread = send_cnt / thread_cnt;

				LOGI << "send_cnt: " << send_cnt << " / thread_cnt: " << thread_cnt << " / send_cnt_per_thread: " << send_cnt_per_thread;
				LOGI << "thread_cnt * send_cnt_per_thread: " << thread_cnt * send_cnt_per_thread;

                for (int i = 0; i < thread_cnt; ++i) {
					LOGI << "Thread " << i << " started";
                    boost::asio::post(pool, [send_cnt_per_thread, msg, &socket_pool, i]() {
                        handle_sockets(socket_pool, send_cnt_per_thread, msg, i);
                        });
                }
            }
			

            if (message.rfind("/debug", 0) == 0) {
                std::istringstream iss(message.substr(7));
                int thread_cnt, connection_cnt;
                if (!(iss >> thread_cnt >> connection_cnt) || thread_cnt <= 0 || connection_cnt <= 0) {
                    continue;
                }
                std::string msg =
                    "You can't let your failures define you. "
                    "You have to let your failures teach you. "
                    "You have to let them show you what to do differently the next time.";
                std::thread([thread_cnt, connection_cnt, msg, &io_context, host, port]() 
                {
                    while (is_running) 
                    {
                        if (is_stop) {
                            std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                        try {
                            int socket_cnt = 10000;
                            
                            MemoryPool<Socket> socket_pool(socket_cnt,
                                [&io_context, &host, &port]() {
                                    return std::make_shared<Socket>(io_context, host, port);
                                }
                            );
                            while (!is_stop && is_running) {
                                boost::asio::thread_pool pool(thread_cnt);
                                for (int i = 0; i < thread_cnt; ++i) {
                                    boost::asio::post(pool, [connection_cnt, msg, &socket_pool, i]() {
                                        handle_sockets(socket_pool, connection_cnt, msg, i);
                                        });
                                }
                                pool.join();
                            }
                            socket_pool.close();
                        }
                        catch (std::exception& e) {
                            std::cerr << "Exception in debug mode: " << e.what() << std::endl;
                        }
                        }
                    }).detach();
                }            
            if (message == "/exit")
            {
                is_running = false;
            }
            else if (message == "/stop")
            {
                std::cout << "Stop command received.\n";
                is_stop = true;
            }
            else if (message == "/start")
            {
                is_stop = false;
            }
            else if (message == "/clear")
            {
#ifdef _WIN32
                system("cls");
#else
                system("clear");
#endif
            }
            else if (message == "/stats")
            {
                for (int i = 0; i < 5; ++i) {
                    LOGD << "Total: " << total_send_cnt.load() << " / Success: " << total_send_success_cnt.load()
                        << " / Fail: " << total_send_fail_cnt.load() << " / Success Rate: "
                        << (total_send_cnt.load() > 0
                            ? (double)total_send_success_cnt.load() / total_send_cnt.load() * 100
                            : 0) << "%";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }
}
