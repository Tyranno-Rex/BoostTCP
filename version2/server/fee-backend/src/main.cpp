#include <iostream>
#include <thread>
#include "../include/server.hpp"
#include "../include/memory_pool.hpp"

std::atomic<bool> running(true);
MemoryPool g_memory_pool(1000);

void consoleInputHandler(Server& consoleServer, Server& chatServer) {
    std::string command;
    while (running) {
        std::getline(std::cin, command);
        if (command == "exit") {
            running = false;
			consoleServer.consoleStop();
			chatServer.chatStop();
			g_memory_pool.clear();
            exit(0);
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
		boost::asio::io_context io_context;
        Server chatServer(io_context, 7777);
		Server consoleServer(io_context, 7778);

		std::thread consoleThread(consoleInputHandler, std::ref(consoleServer), std::ref(chatServer));
        std::thread chatThread([&chatServer]() {
            chatServer.chatRun();
            });

        chatThread.join();
        consoleThread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
