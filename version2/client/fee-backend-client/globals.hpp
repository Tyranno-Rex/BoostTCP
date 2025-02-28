#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>

// 전역 변수 선언부
extern std::atomic<bool> is_running;
extern std::atomic<bool> is_stop;

extern std::mutex command_mutex;
extern std::mutex cout_mutex;
extern std::condition_variable command_cv;

extern bool new_message_ready;
extern std::string shared_message;

extern const char expected_hcv;
extern const char expected_tcv;

extern std::atomic<int> total_send_cnt;
extern std::atomic<int> total_send_success_cnt;
extern std::atomic<int> total_send_fail_cnt;

#endif // GLOBALS_HPP
#pragma once
