#include <iostream>
#include <thread>
#include "../include/server.hpp"
#include "../include/memory_pool.hpp"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define DBG_NEW new
#endif

std::atomic<bool> running(true);
MemoryPool g_memory_pool;


void consoleInputHandler() {
    std::string command;
    while (running) {
        std::getline(std::cin, command);
        if (command == "exit") {
            running = false;  
            return;  
        }
        else {
            std::cout << "Unknown command" << std::endl;
        }
    }
}

int main(void) {
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
#endif
    try {
        std::cout << "Server start" << std::endl;
        boost::asio::io_context io_context;
        Server chatServer(io_context, 7777);
        Server consoleServer(io_context, 7778);

        // Memory pool 초기화
        g_memory_pool.init(1000);

        std::thread consoleThread(consoleInputHandler);
        std::thread chatThread([&chatServer]() {
            chatServer.chatRun();
            });

        // 메인 스레드에서 종료 신호 감지
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 서버 종료 처리
        consoleServer.consoleStop();
        chatServer.chatStop();  

        // 스레드 정리
		chatServer.stopThreadPool();
        if (consoleThread.joinable()) consoleThread.join();
        if (chatThread.joinable()) chatThread.join();


        std::cout << "Server shutting down" << std::endl;
        io_context.stop();  // Boost.Asio 종료
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;  // exit(0) 대신 자연스러운 종료
}
