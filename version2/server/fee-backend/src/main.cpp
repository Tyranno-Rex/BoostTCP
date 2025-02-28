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


std::atomic<int> ReceiveConnectionCnt = 0;
std::atomic<int> Total_Packet_Cnt = 0;
std::atomic<int> Total_Packet_Cnt2 = 0;
std::atomic<int> Total_Packet_Cnt3 = 0;


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

// ����͸� ����� ���� ������ �ڵ�
HANDLE g_hMonitorPipe = nullptr;

// ����͸� ���μ����� �����͸� �����ϴ� �Լ�
void sendToMonitorProcess(const std::string& message) {
    if (g_hMonitorPipe != nullptr) {
        DWORD bytesWritten;
        std::string fullMessage = "echo " + message + "\r\n";
        WriteFile(g_hMonitorPipe, fullMessage.c_str(), static_cast<DWORD>(fullMessage.size()), &bytesWritten, NULL);
    }
}

// ����͸� �ܼ� ���μ����� ���� ��� ������ �Լ�
void monitor_process_thread(HANDLE hPipe) {
    g_hMonitorPipe = hPipe;

    // ����͸� ���μ����� ����� ������ ���
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ���μ��� ���� �� ������ �ڵ� �ݱ�
    if (g_hMonitorPipe != nullptr) {
        CloseHandle(g_hMonitorPipe);
        g_hMonitorPipe = nullptr;
    }
}

// ������ ����͸� ������ �Լ�
void monitorManager() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::stringstream ss;

		ss << "ReceiveConnectionCnt: " << ReceiveConnectionCnt << " Total_Packet_Cnt: " << Total_Packet_Cnt << " Total_Packet_Cnt2: " << Total_Packet_Cnt2 << " Total_Packet_Cnt3: " << Total_Packet_Cnt3;
		sendToMonitorProcess(ss.str());
		ss.str("");

        ss << "JH: " << JH_recv_packet_total_cnt << " / " << JY_recv_packet_success_cnt
            << " / " << JY_recv_packet_fail_cnt << " success rate: "
            << (double)JY_recv_packet_success_cnt / JH_recv_packet_total_cnt * 100 << "%";
        sendToMonitorProcess(ss.str());
        ss.str("");
        
		ss << "YJ: " << YJ_recv_packet_total_cnt << " / " << YJ_recv_packet_success_cnt
			<< " / " << YJ_recv_packet_fail_cnt << " success rate: "
			<< (double)YJ_recv_packet_success_cnt / YJ_recv_packet_total_cnt * 100 << "%";

        //ss << "YJ: " << YJ_recv_packet_total_cnt << " / " << YJ_recv_packet_success_cnt
        //    << " / " << YJ_recv_packet_fail_cnt << " success rate: "
        //    << (double)YJ_recv_packet_success_cnt / YJ_recv_packet_total_cnt * 100 << "%";
        sendToMonitorProcess(ss.str());
        ss.str("");

        ss << "ES: " << ES_recv_packet_total_cnt << " / " << ES_recv_packet_success_cnt
            << " / " << ES_recv_packet_fail_cnt << " success rate: "
            << (double)ES_recv_packet_success_cnt / ES_recv_packet_total_cnt * 100 << "%";
        sendToMonitorProcess(ss.str());
        sendToMonitorProcess(""); // �� �� �߰�
    }
}

// ����͸� �ܼ� ���� �Լ�
bool createMonitorConsole() {
    // ���� �Ӽ� ����
    SECURITY_ATTRIBUTES security = {};
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = TRUE;

    // ������ ����
    HANDLE hPipeRead = nullptr, hPipeWrite = nullptr;
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &security, 0)) {
        std::cerr << "Monitor pipe creation failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    // ���μ��� ���� ���� ����
    STARTUPINFO sinfo = {};
    sinfo.cb = sizeof(STARTUPINFO);
    sinfo.hStdInput = hPipeRead;
    sinfo.dwFlags = STARTF_USESTDHANDLES;

    // ���μ��� ���� ����
    PROCESS_INFORMATION pinfo = {};
    TCHAR lpCommandLine[] = TEXT("cmd.exe");

    // ����͸� �ܼ� ���μ��� ����
    if (!CreateProcess(NULL, lpCommandLine, NULL, NULL, TRUE,
        CREATE_NEW_CONSOLE, NULL, NULL, &sinfo, &pinfo)) {
        std::cerr << "Monitor process creation failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hPipeRead);
        CloseHandle(hPipeWrite);
        return false;
    }

    // ���μ��� �ڵ� �ݱ� (���μ����� ��� �����)
    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);

    // ����͸� ������ ����
    std::thread monitorPipeThread(monitor_process_thread, hPipeWrite);
    monitorPipeThread.detach();

    return true;
}

// ������ main �Լ�
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
        // Memory pool �ʱ�ȭ
        g_memory_pool.init(10000);
        //plog �ʱ�ȭ
        static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
        plog::init(plog::verbose, &consoleAppender);

        // ����͸� �ܼ� ����
        if (!createMonitorConsole()) {
            LOGE << "Failed to create monitoring console";
        }

        std::thread consoleThread(consoleInputHandler);
        std::thread monitorThread(monitorManager);
        std::thread chatThread([&chatServer]() {
            chatServer.chatRun();
            });

        // ���� �����忡�� ���� ��ȣ ����
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // ���� ���� ó��
        consoleServer.consoleStop();
        chatServer.chatStop();

        // ������ ����
        chatServer.stopThreadPool();
        if (consoleThread.joinable()) consoleThread.join();
        if (chatThread.joinable()) chatThread.join();
        if (monitorThread.joinable()) monitorThread.join();

        LOGI << "Server shutting down";
        io_context.stop();  // Boost.Asio ����
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}

//void monitorManager() {
//	while (running) {   
//		std::this_thread::sleep_for(std::chrono::seconds(5));
//		LOGI << "JH: " << JH_recv_packet_total_cnt << " / " << JY_recv_packet_success_cnt << " / " << JY_recv_packet_fail_cnt << " success rate: " << (double)JY_recv_packet_success_cnt / JH_recv_packet_total_cnt * 100 << "%";
//		LOGI << "YJ: " << YJ_recv_packet_total_cnt << " / " << YJ_recv_packet_success_cnt << " / " << YJ_recv_packet_fail_cnt << " success rate: " << (double)YJ_recv_packet_success_cnt / YJ_recv_packet_total_cnt * 100 << "%";
//		LOGI << "ES: " << ES_recv_packet_total_cnt << " / " << ES_recv_packet_success_cnt << " / " << ES_recv_packet_fail_cnt << " success rate: " << (double)ES_recv_packet_success_cnt / ES_recv_packet_total_cnt * 100 << "%\n\n";
//    }
//}
//
//int main(void) {
//#ifdef _DEBUG
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
//    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
//#endif
//    try {
//		LOGE << "Server start";
//        boost::asio::io_context io_context;
//        Server chatServer(io_context, 7777);
//        Server consoleServer(io_context, 7778);
//
//        // Memory pool �ʱ�ȭ
//        g_memory_pool.init(100000);
//
//		//plog �ʱ�ȭ
//        //static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
//		static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
//		plog::init(plog::verbose, &consoleAppender);
//
//        std::thread consoleThread(consoleInputHandler);
//		std::thread monitorThread(monitorManager);
//        std::thread chatThread([&chatServer]() {
//            chatServer.chatRun();
//            });
//
//        // ���� �����忡�� ���� ��ȣ ����
//        while (running) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(100));
//        }
//
//        // ���� ���� ó��
//        consoleServer.consoleStop();
//        chatServer.chatStop();
//
//        // ������ ����
//        chatServer.stopThreadPool();
//        if (consoleThread.joinable()) consoleThread.join();
//        if (chatThread.joinable()) chatThread.join();
//		if (monitorThread.joinable()) monitorThread.join();
//
//		LOGI << "Server shutting down";
//        io_context.stop();  // Boost.Asio ����
//    }
//    catch (std::exception& e) {
//        std::cerr << "Error: " << e.what() << std::endl;
//    }
//
//    return 0; 
//}
