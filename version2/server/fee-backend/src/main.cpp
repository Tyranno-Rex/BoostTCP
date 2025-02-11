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

std::atomic<int> JH_recv_packet_total_cnt = 0;
std::atomic<int> JY_recv_packet_success_cnt = 0;
std::atomic<int> JY_recv_packet_fail_cnt = 0;

std::atomic<int> YJ_recv_packet_total_cnt = 0;
std::atomic<int> YJ_recv_packet_success_cnt = 0;
std::atomic<int> YJ_recv_packet_fail_cnt = 0;

std::atomic<int> ES_recv_packet_total_cnt = 0;
std::atomic<int> ES_recv_packet_success_cnt = 0;
std::atomic<int> ES_recv_packet_fail_cnt = 0;

void consoleInputHandler() {
    std::string command;
    while (running) {
        std::getline(std::cin, command);
        if (command == "exit") {
            running = false;  
            return;  
        }
		if (command == "clear") {
			system("cls");
		}
        else {
			LOGE << "Unknown command";
        }
    }
}

void monitorManager() {
	while (running) {   
		std::this_thread::sleep_for(std::chrono::seconds(5));
		LOGI << "JH: " << JH_recv_packet_total_cnt << " / " << JY_recv_packet_success_cnt << " / " << JY_recv_packet_fail_cnt << " success rate: " << (double)JY_recv_packet_success_cnt / JH_recv_packet_total_cnt * 100 << "%";
		LOGI << "YJ: " << YJ_recv_packet_total_cnt << " / " << YJ_recv_packet_success_cnt << " / " << YJ_recv_packet_fail_cnt << " success rate: " << (double)YJ_recv_packet_success_cnt / YJ_recv_packet_total_cnt * 100 << "%";
		LOGI << "ES: " << ES_recv_packet_total_cnt << " / " << ES_recv_packet_success_cnt << " / " << ES_recv_packet_fail_cnt << " success rate: " << (double)ES_recv_packet_success_cnt / ES_recv_packet_total_cnt * 100 << "%\n\n";
    }
}

int main(void) {
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
#endif
    try {
		LOGE << "Server start";
        boost::asio::io_context io_context;
        Server chatServer(io_context, 7777);
        Server consoleServer(io_context, 7778);

        // Memory pool 초기화
        g_memory_pool.init(100000);

		//plog 초기화
        //static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
		static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
		plog::init(plog::verbose, &consoleAppender);

        std::thread consoleThread(consoleInputHandler);
		std::thread monitorThread(monitorManager);
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
		if (monitorThread.joinable()) monitorThread.join();

		LOGI << "Server shutting down";
        io_context.stop();  // Boost.Asio 종료
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0; 
}
