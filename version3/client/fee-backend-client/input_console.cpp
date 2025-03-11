#include "input.hpp"
#include "globals.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef _WIN32
#include <tchar.h>
#endif

void run_input_process_in_new_console()
{
#ifdef _WIN32
    // ������ ������ ���� �Ӽ�
    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // �ڽ� ���μ���(cmd.exe)�� ǥ������� ���� ������
    HANDLE hPipeOut_Read = NULL;
    HANDLE hPipeOut_Write = NULL;

    if (!CreatePipe(&hPipeOut_Read, &hPipeOut_Write, &saAttr, 0))
    {
        std::cerr << "CreatePipe failed. Error: " << GetLastError() << std::endl;
        return;
    }
    // �θ�(����) ���μ������� Read �ڵ��� ��ӵ��� �ʵ��� ����
    SetHandleInformation(hPipeOut_Read, HANDLE_FLAG_INHERIT, 0);

    // �ڽ� ���μ��� ���� ����ü
    PROCESS_INFORMATION piProcInfo{};
    STARTUPINFO siStartInfo{};
    siStartInfo.cb = sizeof(STARTUPINFO);

    // �ڽ� ���μ����� stdout/stderr�� �츮�� ���� �������� �����̷�Ʈ
    siStartInfo.hStdOutput = hPipeOut_Write;
    siStartInfo.hStdError = hPipeOut_Write;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // �� �ַܼ� cmd.exe ����
    TCHAR szCmdline[] = TEXT("cmd.exe");
    if (!CreateProcess(
        NULL,
        szCmdline,
        NULL,
        NULL,
        TRUE,                // �ڵ� ���
        CREATE_NEW_CONSOLE,  // �� �ܼ� ����
        NULL,
        NULL,
        &siStartInfo,
        &piProcInfo))
    {
        std::cerr << "CreateProcess(cmd.exe) failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hPipeOut_Read);
        CloseHandle(hPipeOut_Write);
        return;
    }

    // �θ� ���μ������� �� �̻� Write �ڵ��� �� ���� �����Ƿ� ����
    CloseHandle(hPipeOut_Write);

    // ���� hPipeOut_Read�� ���� �ڽ� ���μ���(cmd.exe)�� ���(stdout)�� �о�´�
    char buffer[4096];
    DWORD dwRead = 0;

    bool local_running = true;
    while (local_running && is_running)
    {
        // cmd.exe�� ����� ���� �б�
        if (!ReadFile(hPipeOut_Read, buffer, sizeof(buffer) - 1, &dwRead, NULL) || dwRead == 0)
        {
            // ERROR_BROKEN_PIPE == �������� �����ų� ���μ��� ����
            if (GetLastError() == ERROR_BROKEN_PIPE)
            {
                break;
            }
            else
            {
                std::cerr << "ReadFile error: " << GetLastError() << std::endl;
                break;
            }
        }
        buffer[dwRead] = '\0';

        // cmd.exe ��� ������ �� �پ� �м�
        std::string output(buffer);
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line))
        {
            // ��: ����ڰ� �� �ֿܼ��� "echo /stop"�� ġ��
            //     cmd.exe�� "/stop" ������ ����� �� ����
            //     �ش� ������ '/'�� �����ϸ� ������� �ν�
            if (!line.empty() && line[0] == '/')
            {
                // ���� input_thread ������ �����ϰ� ó��
                {
                    std::lock_guard<std::mutex> lock(command_mutex);
                    shared_message = line;
                    new_message_ready = true;
                }
                command_cv.notify_one();
            }
			if (line == "/exit")
			{
				local_running = false;
				break;
			}
			line.clear();
        }

		output.clear();
		iss.clear();
    }

    // �ڽ� ���μ��� ����
    TerminateProcess(piProcInfo.hProcess, 0);
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    CloseHandle(hPipeOut_Read);

#else
    // Windows�� �ƴ� ���, �� ����� �۵����� ����
    std::cerr << "[Error] Windows ���� �����Դϴ�.\n";
#endif
}
