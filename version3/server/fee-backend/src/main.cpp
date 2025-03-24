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
//MemoryPool g_memory_pool;
MemoryPool<char[1540]> g_memory_pool;

PacketChecker g_packet_checker;

std::atomic<int> Session_Count = 0;

std::atomic<int> JH_recv_packet_total_cnt = 0;
std::atomic<int> JH_recv_packet_success_cnt = 0;
std::atomic<int> JH_recv_packet_fail_cnt = 0;

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

// ¸ğ´ÏÅÍ¸µ Ãâ·ÂÀ» À§ÇÑ ÆÄÀÌÇÁ ÇÚµé
HANDLE g_hMonitorPipe = nullptr;

// ¸ğ´ÏÅÍ¸µ ÇÁ·Î¼¼½º¿¡ µ¥ÀÌÅÍ¸¦ Àü¼ÛÇÏ´Â ÇÔ¼ö
void sendToMonitorProcess(const std::string& message) {
    if (g_hMonitorPipe != nullptr) {
        DWORD bytesWritten;
        std::string fullMessage = "echo " + message + "\r\n";
        WriteFile(g_hMonitorPipe, fullMessage.c_str(), static_cast<DWORD>(fullMessage.size()), &bytesWritten, NULL);
    }
}

// ¸ğ´ÏÅÍ¸µ ÄÜ¼Ö ÇÁ·Î¼¼½º¸¦ À§ÇÑ Ãâ·Â ¾²·¹µå ÇÔ¼ö
void monitor_process_thread(HANDLE hPipe) {
    g_hMonitorPipe = hPipe;

    // ¸ğ´ÏÅÍ¸µ ÇÁ·Î¼¼½º°¡ Á¾·áµÉ ¶§±îÁö ´ë±â
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ÇÁ·Î¼¼½º Á¾·á ½Ã ÆÄÀÌÇÁ ÇÚµé ´İ±â
    if (g_hMonitorPipe != nullptr) {
        CloseHandle(g_hMonitorPipe);
        g_hMonitorPipe = nullptr;
    }
}

// ¼öÁ¤µÈ ¸ğ´ÏÅÍ¸µ °ü¸®ÀÚ ÇÔ¼ö
void monitorManager() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::stringstream ss;

        ss << "JH: " << JH_recv_packet_total_cnt << " / " << JH_recv_packet_success_cnt
            << " / " << JH_recv_packet_fail_cnt << " success rate: "
            << (double)JH_recv_packet_success_cnt / JH_recv_packet_total_cnt * 100 << "%";
        sendToMonitorProcess(ss.str());
        ss.str("");
        
		ss << "YJ: " << YJ_recv_packet_total_cnt << " / " << YJ_recv_packet_success_cnt
			<< " / " << YJ_recv_packet_fail_cnt << " success rate: "
			<< (double)YJ_recv_packet_success_cnt / YJ_recv_packet_total_cnt * 100 << "%";
        sendToMonitorProcess(ss.str());
        ss.str("");

        ss << "ES: " << ES_recv_packet_total_cnt << " / " << ES_recv_packet_success_cnt
            << " / " << ES_recv_packet_fail_cnt << " success rate: "
            << (double)ES_recv_packet_success_cnt / ES_recv_packet_total_cnt * 100 << "%";
        sendToMonitorProcess(ss.str());
        sendToMonitorProcess(""); // ºó ÁÙ Ãß°¡
    }
}

// ¸ğ´ÏÅÍ¸µ ÄÜ¼Ö »ı¼º ÇÔ¼ö
bool createMonitorConsole() {
    // º¸¾È ¼Ó¼º ¼³Á¤
    SECURITY_ATTRIBUTES security = {};
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = TRUE;

    // ÆÄÀÌÇÁ »ı¼º
    HANDLE hPipeRead = nullptr, hPipeWrite = nullptr;
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &security, 0)) {
        std::cerr << "Monitor pipe creation failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    // ÇÁ·Î¼¼½º ½ÃÀÛ Á¤º¸ ¼³Á¤
    STARTUPINFO sinfo = {};
    sinfo.cb = sizeof(STARTUPINFO);
    sinfo.hStdInput = hPipeRead;
    sinfo.dwFlags = STARTF_USESTDHANDLES;

    // ÇÁ·Î¼¼½º Á¤º¸ ¼³Á¤
    PROCESS_INFORMATION pinfo = {};
    TCHAR lpCommandLine[] = TEXT("cmd.exe");

    // ¸ğ´ÏÅÍ¸µ ÄÜ¼Ö ÇÁ·Î¼¼½º »ı¼º
    if (!CreateProcess(NULL, lpCommandLine, NULL, NULL, TRUE,
        CREATE_NEW_CONSOLE, NULL, NULL, &sinfo, &pinfo)) {
        std::cerr << "Monitor process creation failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hPipeRead);
        CloseHandle(hPipeWrite);
        return false;
    }

    // ÇÁ·Î¼¼½º ÇÚµé ´İ±â (ÇÁ·Î¼¼½º´Â °è¼Ó ½ÇÇàµÊ)
    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);

    // ¸ğ´ÏÅÍ¸µ ¾²·¹µå »ı¼º
    std::thread monitorPipeThread(monitor_process_thread, hPipeWrite);
    monitorPipeThread.detach();

    return true;
}

// ¼öÁ¤µÈ main ÇÔ¼ö
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
<<<<<<< HEAD
        //Server consoleServer(io_context, 7778);
        
=======
        Server consoleServer(io_context, 7778);
>>>>>>> parent of cfe8a2e (í´ë¼ ë©”ëª¨ë¦¬ ë¦­ ë° ì„œë²„ sequence ìˆœì„œ ì²˜ë¦¬ ì™„ë£Œ -> í´ë¼ì´ì–¸íŠ¸ ì„¸ì…˜ ë³„ ì²˜ë¦¬ë˜ëŠ” ìŠ¤ë ˆë“œë¥¼ ì§€ì •í•¨ìœ¼ë¡œì¨ ìˆœì„œë¥¼ ë³´ì¥í•¨.)
        // Memory pool ÃÊ±âÈ­
		g_memory_pool.init(10000);

        //plog ÃÊ±âÈ­
        static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
        plog::init(plog::verbose, &consoleAppender);

        // ¸ğ´ÏÅÍ¸µ ÄÜ¼Ö »ı¼º
        if (!createMonitorConsole()) {
            LOGE << "Failed to create monitoring console";
        }

        std::thread consoleThread(consoleInputHandler);
        std::thread monitorThread(monitorManager);
        std::thread chatThread([&chatServer]() {
            chatServer.chatRun();
            });

        // ¸ŞÀÎ ½º·¹µå¿¡¼­ Á¾·á ½ÅÈ£ °¨Áö
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // ¼­¹ö Á¾·á Ã³¸®
        consoleServer.consoleStop();
        chatServer.chatStop();

        // ½º·¹µå Á¤¸®
        chatServer.stopThreadPool();
        if (consoleThread.joinable()) consoleThread.join();
        if (chatThread.joinable()) chatThread.join();
        if (monitorThread.joinable()) monitorThread.join();

        LOGI << "Server shutting down";
        io_context.stop();  // Boost.Asio Á¾·á
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}