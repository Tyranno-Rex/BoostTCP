//#pragma once
//
//#ifndef INPUT_THREAD_HPP
//#define INPUT_THREAD_HPP
//
//// ����� ����� �޴� ������ �Լ�
//void input_thread();
//
//#endif // INPUT_THREAD_HPP


#ifndef INPUT_PROCESS_HPP
#define INPUT_PROCESS_HPP

#ifdef _WIN32
#include <Windows.h>
#endif

// �� �ܼ�(cmd.exe)�� ����� ����� �Է��� �ް�, 
// �ش� �ܼ��� stdout�� �������� �о� ����� ó���ϴ� �Լ�
void run_input_process_in_new_console();

#endif // INPUT_PROCESS_HPP
