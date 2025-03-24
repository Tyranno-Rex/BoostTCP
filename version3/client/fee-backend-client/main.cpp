#include "main.hpp"
#include "socket.hpp"
#include "utils.hpp"
#include "globals.hpp"
#include "input.hpp"
#include "log_console.hpp"
#include "handle_messages.hpp"

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

    try {
        boost::asio::io_context io_context;
        static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
        plog::init(plog::verbose, &consoleAppender);
		
        // 콘솔 입력 스레드 시작
        std::thread inputConsoleThread(run_input_process_in_new_console);
        
        // 멀티스레드로 io_context 실행
        std::vector<std::thread> io_threads;
        size_t thread_count = std::thread::hardware_concurrency() * 0.5;
        
		// io_context 스레드 생성
        for (size_t i = 0; i < thread_count; ++i) {
            io_threads.emplace_back([&io_context]() {
                while (is_running) {
                    io_context.run();
                }
                });
        }

        write_messages(io_context, host, chat_port);

        if (inputConsoleThread.joinable()) {
            inputConsoleThread.join();
        }

        // 모든 io_context 스레드가 종료될 때까지 대기
        for (auto& thread : io_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        io_context.stop();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
	system("pause");
    return 0;
}
