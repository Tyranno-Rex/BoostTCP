#include "globals.hpp"

// 전역 변수 정의부
std::atomic<bool> is_running = true;
std::atomic<bool> is_stop = false;

std::mutex command_mutex;
std::mutex cout_mutex;
std::condition_variable command_cv;

bool new_message_ready = false;
std::string shared_message;
const char expected_hcv = 0x02;
const char expected_tcv = 0x03;

std::atomic<int> socket_pool_size = 0;
std::atomic<int> total_send_cnt = 0;
std::atomic<int> total_send_success_cnt = 0;
std::atomic<int> total_send_fail_cnt = 0;