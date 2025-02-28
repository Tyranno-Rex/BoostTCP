#include "write_messages.hpp"
#include "globals.hpp"
#include "input.hpp"
#include "log.hpp"
#include "socket.hpp"
#include "utils.hpp"

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
    // ����� ��� �Է� ������ ����
    //std::thread inputThread(input_thread);

    // �α� ���μ���(���� �ܼ�) ����
	// security �Ӽ��� �������� ������ �ܼ��� �������� ����
    SECURITY_ATTRIBUTES security = {};
	// nLength�� ����ü�� ũ��, bInheritHandle�� �ڽ� ���μ����� ������� ����
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = TRUE;

	// ������ ����
    HANDLE hPipeIn_Read = nullptr, hPipeIn_Write = nullptr;
	// CreatePipe �Լ��� �������� �����ϰ� �ڵ��� ��ȯ -> �������� ����: ���μ��� �� ���
	// CreatePipe(�������ڵ�, �������ڵ�, ���ȼӼ�, ����ũ��)
    CreatePipe(&hPipeIn_Read, &hPipeIn_Write, &security, 0);

	// ���μ��� ���� ���� ����
    STARTUPINFO sinfo = {};
	// cb�� ����ü�� ũ��, hStdInput�� ǥ�� �Է� �ڵ�, dwFlags�� �÷���
    sinfo.cb = sizeof(STARTUPINFO);
    sinfo.hStdInput = hPipeIn_Read;
    sinfo.dwFlags = STARTF_USESTDHANDLES;

	// ���μ��� ���� ����
    PROCESS_INFORMATION pinfo = {};
	// lpCommandLine�� ������ ���α׷��� ���
    TCHAR lpCommandLine[] = TEXT("cmd.exe");


	// CreateProcess(��ƫ�̸�, ��ɶ���, ���ȼӼ�, ���ȼӼ�, ��ӿ���, 
    //              �÷���, ȯ�溯��, ������丮, ��������, ���μ�������)
    if (!CreateProcess( NULL, lpCommandLine, NULL, NULL, TRUE,
                        CREATE_NEW_CONSOLE, NULL, NULL, &sinfo, &pinfo ))
    {
        std::cerr << "Log process creation failed. Error: " << GetLastError() << std::endl;
    }
    else {
        // �α� ��� ������
        std::thread logThread(log_process, hPipeIn_Write);
        logThread.detach();

    }

    try {
        while (is_running) {
            std::unique_lock<std::mutex> lock(command_mutex);
            command_cv.wait(lock, [] { return new_message_ready; });

            std::string message = shared_message;
            new_message_ready = false;
            lock.unlock();
            
			trim(message);
			std::cout << "Received command: " << message << std::endl;

			//if (message == "/0") {
   //             // ������ �޽��� �� �� ����
   //             SocketPool socket_pool(io_context, host, port, 1);
   //             std::string msg = "simple message";
   //             boost::asio::thread_pool pool(1);
   //             boost::asio::post(pool, [&socket_pool, msg]() {
   //                 //handle_sockets(socket_pool, 1, msg, 0);
   //                 });
   //             pool.join();
   //         }
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
                std::thread([thread_cnt, connection_cnt, msg, &io_context, host, port]() {
                    while (is_running) {
                        if (is_stop) {
                            std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                        try {
                            while (!is_stop && is_running) {
		                        LOGE << "Starting debug mode with " << thread_cnt << " threads and " << connection_cnt << " connections.";
                                

                                //boost::asio::thread_pool pool(thread_cnt);
                                //int socket_cnt = 1000;
                                //SocketPool socket_pool(io_context, host, port, size_t(socket_cnt));
                                //for (int i = 0; i < thread_cnt; ++i) {
                                //    boost::asio::post(pool, [&socket_pool, connection_cnt, msg, i]() {
                                //        handle_sockets(socket_pool, connection_cnt, msg, i);
                                //        });
                                //}
                                //pool.join();
                                //socket_pool.close();


                                // shared_ptr�� ���� Ǯ ����
                                auto socket_pool_ptr = std::make_shared<SocketPool>(io_context, host, port, size_t(100));

                                // ������ ī���� �߰�
                                std::shared_ptr<std::atomic<int>> pending_ops = std::make_shared<std::atomic<int>>(0);

                                // ������ Ǯ ����
                                boost::asio::thread_pool pool(thread_cnt);

                                for (int i = 0; i < thread_cnt; ++i) {
                                    boost::asio::post(pool, [socket_pool_ptr, pending_ops, connection_cnt, msg, i]() {
                                        //handle_sockets(*socket_pool_ptr, connection_cnt, msg, i, pending_ops);
                                        });
                                }

                                // ��� �񵿱� �۾��� �Ϸ�� ������ ��� (0�� �� ������)
                                LOGE << "Thread pool joined. Waiting for " << *pending_ops << " async operations to complete...";
                                pool.join();


                                while (*pending_ops > 0) {
                                    LOGE << "Pending operations: " << *pending_ops;
                                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                }
                                

                                if (*pending_ops == 0) {
                                    // ������ Ǯ �۾� �Ϸ� ���
                                    LOGE << "All async operations completed. Closing socket pool.";
                                    socket_pool_ptr->close();
                                }

                                std::this_thread::sleep_for(std::chrono::seconds(1));
                                
                            }
                        }
                        catch (std::exception& e) {
                            std::cerr << "Exception in debug mode: " << e.what() << std::endl;
                        }
                    }
                    }).detach();  // detach() �Ǵ� join()�� ��Ȳ�� �°� ����
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
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }

    // ���μ��� ����
        
    if (pinfo.hProcess) {
        TerminateProcess(pinfo.hProcess, 0);
        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }
    CloseHandle(hPipeIn_Read);
    CloseHandle(hPipeIn_Write);
}
