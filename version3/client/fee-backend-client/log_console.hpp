#pragma once
#ifndef LOG_PROCESS_HPP
#define LOG_PROCESS_HPP

#ifdef _WIN32
#include <Windows.h>
#endif

// ���� �ܼ�(������)�� �α׸� �ֱ������� ����ϴ� �Լ�
void log_process(HANDLE hPipe);

#endif // LOG_PROCESS_HPP
