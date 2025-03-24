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
//MemoryPool<std::array<char, 1540>> g_memory_pool(10000);
MemoryPool<std::array<char, 1540>> g_memory_pool(10000, []() { return std::make_shared<std::array<char, 1540>>(); });

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
        //Server consoleServer(io_context, 7778);
        
        // Memory pool �ʱ�ȭ
		//g_memory_pool.init(10000);

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
        //consoleServer.consoleStop();
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