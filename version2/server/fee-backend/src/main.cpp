#include <iostream>
#include <thread>
#include <pqxx/pqxx>
#include "../include/server.hpp"

std::atomic<bool> running(true);

void consoleInputHandler() {
    std::string command;
    while (running) {
        std::getline(std::cin, command);
        if (command == "exit") {
            running = false;
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
        Server ConsoleServer(7778);
        Server chatServer(7777);

        std::thread consoleThread(consoleInputHandler);
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
