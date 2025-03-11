#pragma once
#ifndef LOG_PROCESS_HPP
#define LOG_PROCESS_HPP

#ifdef _WIN32
#include <Windows.h>
#endif

// 별도 콘솔(파이프)에 로그를 주기적으로 출력하는 함수
void log_process(HANDLE hPipe);

#endif // LOG_PROCESS_HPP
