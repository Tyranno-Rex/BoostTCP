//#pragma once
//
//#ifndef INPUT_THREAD_HPP
//#define INPUT_THREAD_HPP
//
//// 사용자 명령을 받는 쓰레드 함수
//void input_thread();
//
//#endif // INPUT_THREAD_HPP


#ifndef INPUT_PROCESS_HPP
#define INPUT_PROCESS_HPP

#ifdef _WIN32
#include <Windows.h>
#endif

// 새 콘솔(cmd.exe)을 띄워서 사용자 입력을 받고, 
// 해당 콘솔의 stdout을 파이프로 읽어 명령을 처리하는 함수
void run_input_process_in_new_console();

#endif // INPUT_PROCESS_HPP
