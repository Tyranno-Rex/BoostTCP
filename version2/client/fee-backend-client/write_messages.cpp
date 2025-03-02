#include "write_messages.hpp"
#include "globals.hpp"
#include "input.hpp"
#include "log.hpp"
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
    // 사용자 명령 입력 쓰레드 시작
    //std::thread inputThread(input_thread);

    // 로그 프로세스(별도 콘솔) 생성
	// security 속성을 설정하지 않으면 콘솔이 생성되지 않음
    SECURITY_ATTRIBUTES security = {};
	// nLength는 구조체의 크기, bInheritHandle은 자식 프로세스에 상속할지 여부
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = TRUE;

	// 파이프 생성
    HANDLE hPipeIn_Read = nullptr, hPipeIn_Write = nullptr;
	// CreatePipe 함수는 파이프를 생성하고 핸들을 반환 -> 파이프의 역할: 프로세스 간 통신
	// CreatePipe(파이프핸들, 파이프핸들, 보안속성, 버퍼크기)
    CreatePipe(&hPipeIn_Read, &hPipeIn_Write, &security, 0);

	// 프로세스 시작 정보 설정
    STARTUPINFO sinfo = {};
	// cb는 구조체의 크기, hStdInput은 표준 입력 핸들, dwFlags는 플래그
    sinfo.cb = sizeof(STARTUPINFO);
    sinfo.hStdInput = hPipeIn_Read;
    sinfo.dwFlags = STARTF_USESTDHANDLES;

	// 프로세스 정보 설정
    PROCESS_INFORMATION pinfo = {};
	// lpCommandLine은 실행할 프로그램의 경로
    TCHAR lpCommandLine[] = TEXT("cmd.exe");


	// CreateProcess(모튤이름, 명령라인, 보안속성, 보안속성, 상속여부, 
    //              플래그, 환경변수, 현재디렉토리, 시작정보, 프로세스정보)
    if (!CreateProcess( NULL, lpCommandLine, NULL, NULL, TRUE,
                        CREATE_NEW_CONSOLE, NULL, NULL, &sinfo, &pinfo ))
    {
        std::cerr << "Log process creation failed. Error: " << GetLastError() << std::endl;
    }
    else {
        // 로그 출력 쓰레드
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
                            int socket_cnt = 1000;
                            
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
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }

    // 프로세스 정리
        
    if (pinfo.hProcess) {
        TerminateProcess(pinfo.hProcess, 0);
        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }
    CloseHandle(hPipeIn_Read);
    CloseHandle(hPipeIn_Write);
}
