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
    // 파이프 보안을 위한 속성
    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // 자식 프로세스(cmd.exe)의 표준출력을 받을 파이프
    HANDLE hPipeOut_Read = NULL;
    HANDLE hPipeOut_Write = NULL;

    if (!CreatePipe(&hPipeOut_Read, &hPipeOut_Write, &saAttr, 0))
    {
        std::cerr << "CreatePipe failed. Error: " << GetLastError() << std::endl;
        return;
    }
    // 부모(현재) 프로세스에서 Read 핸들은 상속되지 않도록 설정
    SetHandleInformation(hPipeOut_Read, HANDLE_FLAG_INHERIT, 0);

    // 자식 프로세스 정보 구조체
    PROCESS_INFORMATION piProcInfo{};
    STARTUPINFO siStartInfo{};
    siStartInfo.cb = sizeof(STARTUPINFO);

    // 자식 프로세스의 stdout/stderr를 우리가 만든 파이프로 리다이렉트
    siStartInfo.hStdOutput = hPipeOut_Write;
    siStartInfo.hStdError = hPipeOut_Write;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // 새 콘솔로 cmd.exe 실행
    TCHAR szCmdline[] = TEXT("cmd.exe");
    if (!CreateProcess(
        NULL,
        szCmdline,
        NULL,
        NULL,
        TRUE,                // 핸들 상속
        CREATE_NEW_CONSOLE,  // 새 콘솔 열기
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

    // 부모 프로세스에서 더 이상 Write 핸들은 쓸 일이 없으므로 닫음
    CloseHandle(hPipeOut_Write);

    // 이제 hPipeOut_Read를 통해 자식 프로세스(cmd.exe)의 출력(stdout)을 읽어온다
    char buffer[4096];
    DWORD dwRead = 0;

    bool local_running = true;
    while (local_running && is_running)
    {
        // cmd.exe가 출력한 내용 읽기
        if (!ReadFile(hPipeOut_Read, buffer, sizeof(buffer) - 1, &dwRead, NULL) || dwRead == 0)
        {
            // ERROR_BROKEN_PIPE == 파이프가 닫혔거나 프로세스 종료
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

        // cmd.exe 출력 내용을 한 줄씩 분석
        std::string output(buffer);
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line))
        {
            // 예: 사용자가 새 콘솔에서 "echo /stop"을 치면
            //     cmd.exe가 "/stop" 라인을 출력할 수 있음
            //     해당 라인이 '/'로 시작하면 명령으로 인식
            if (!line.empty() && line[0] == '/')
            {
                // 기존 input_thread 로직과 동일하게 처리
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

    // 자식 프로세스 종료
    TerminateProcess(piProcInfo.hProcess, 0);
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    CloseHandle(hPipeOut_Read);

#else
    // Windows가 아닌 경우, 이 방식은 작동하지 않음
    std::cerr << "[Error] Windows 전용 예제입니다.\n";
#endif
}
