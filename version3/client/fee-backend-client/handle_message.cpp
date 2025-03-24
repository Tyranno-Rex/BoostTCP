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
<<<<<<< HEAD
=======
    // »ç¿ëÀÚ ¸í·É ÀÔ·Â ¾²·¹µå ½ÃÀÛ
    //std::thread inputThread(input_thread);

    // ·Î±× ÇÁ·Î¼¼½º(º°µµ ÄÜ¼Ö) »ı¼º
	// security ¼Ó¼ºÀ» ¼³Á¤ÇÏÁö ¾ÊÀ¸¸é ÄÜ¼ÖÀÌ »ı¼ºµÇÁö ¾ÊÀ½
    SECURITY_ATTRIBUTES security = {};
	// nLength´Â ±¸Á¶Ã¼ÀÇ Å©±â, bInheritHandleÀº ÀÚ½Ä ÇÁ·Î¼¼½º¿¡ »ó¼ÓÇÒÁö ¿©ºÎ
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = TRUE;

	// ÆÄÀÌÇÁ »ı¼º
    HANDLE hPipeIn_Read = nullptr, hPipeIn_Write = nullptr;
	// CreatePipe ÇÔ¼ö´Â ÆÄÀÌÇÁ¸¦ »ı¼ºÇÏ°í ÇÚµéÀ» ¹İÈ¯ -> ÆÄÀÌÇÁÀÇ ¿ªÇÒ: ÇÁ·Î¼¼½º °£ Åë½Å
	// CreatePipe(ÆÄÀÌÇÁÇÚµé, ÆÄÀÌÇÁÇÚµé, º¸¾È¼Ó¼º, ¹öÆÛÅ©±â)
    CreatePipe(&hPipeIn_Read, &hPipeIn_Write, &security, 0);

	// ÇÁ·Î¼¼½º ½ÃÀÛ Á¤º¸ ¼³Á¤
    STARTUPINFO sinfo = {};
	// cb´Â ±¸Á¶Ã¼ÀÇ Å©±â, hStdInputÀº Ç¥ÁØ ÀÔ·Â ÇÚµé, dwFlags´Â ÇÃ·¡±×
    sinfo.cb = sizeof(STARTUPINFO);
    sinfo.hStdInput = hPipeIn_Read;
    sinfo.dwFlags = STARTF_USESTDHANDLES;

	// ÇÁ·Î¼¼½º Á¤º¸ ¼³Á¤
    PROCESS_INFORMATION pinfo = {};
	// lpCommandLineÀº ½ÇÇàÇÒ ÇÁ·Î±×·¥ÀÇ °æ·Î
    TCHAR lpCommandLine[] = TEXT("cmd.exe");


	// CreateProcess(¸ğÆ«ÀÌ¸§, ¸í·É¶óÀÎ, º¸¾È¼Ó¼º, º¸¾È¼Ó¼º, »ó¼Ó¿©ºÎ, 
    //              ÇÃ·¡±×, È¯°æº¯¼ö, ÇöÀçµğ·ºÅä¸®, ½ÃÀÛÁ¤º¸, ÇÁ·Î¼¼½ºÁ¤º¸)
    if (!CreateProcess( NULL, lpCommandLine, NULL, NULL, TRUE,
                        CREATE_NEW_CONSOLE, NULL, NULL, &sinfo, &pinfo ))
    {
        std::cerr << "Log process creation failed. Error: " << GetLastError() << std::endl;
    }
    else {
        // ·Î±× Ãâ·Â ¾²·¹µå
        std::thread logThread(log_process, hPipeIn_Write);
        logThread.detach();

    }
>>>>>>> parent of cfe8a2e (í´ë¼ ë©”ëª¨ë¦¬ ë¦­ ë° ì„œë²„ sequence ìˆœì„œ ì²˜ë¦¬ ì™„ë£Œ -> í´ë¼ì´ì–¸íŠ¸ ì„¸ì…˜ ë³„ ì²˜ë¦¬ë˜ëŠ” ìŠ¤ë ˆë“œë¥¼ ì§€ì •í•¨ìœ¼ë¡œì¨ ìˆœì„œë¥¼ ë³´ì¥í•¨.)

    try {
        while (is_running) {
            std::unique_lock<std::mutex> lock(command_mutex);
            command_cv.wait(lock, [] { return new_message_ready; });

            std::string message = shared_message;
            new_message_ready = false;
            lock.unlock();
            
			trim(message);
			std::cout << "Received command: " << message << std::endl;

			// Æ¯Á¤ °³¼ö¸¸Å­¸¸ ¸Ş½ÃÁö Àü¼Û
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
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }
<<<<<<< HEAD
=======

    // ÇÁ·Î¼¼½º Á¤¸®
        
    if (pinfo.hProcess) {
        TerminateProcess(pinfo.hProcess, 0);
        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }
    CloseHandle(hPipeIn_Read);
    CloseHandle(hPipeIn_Write);
>>>>>>> parent of cfe8a2e (í´ë¼ ë©”ëª¨ë¦¬ ë¦­ ë° ì„œë²„ sequence ìˆœì„œ ì²˜ë¦¬ ì™„ë£Œ -> í´ë¼ì´ì–¸íŠ¸ ì„¸ì…˜ ë³„ ì²˜ë¦¬ë˜ëŠ” ìŠ¤ë ˆë“œë¥¼ ì§€ì •í•¨ìœ¼ë¡œì¨ ìˆœì„œë¥¼ ë³´ì¥í•¨.)
}
