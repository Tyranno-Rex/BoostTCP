#pragma once
#include "log_console.hpp"
#include "globals.hpp"

#include <string>
#include <chrono>
#include <thread>
#include <iostream>

void log_process(HANDLE hPipe) {
    //while (true) {
        std::string log_message =
            "echo Total: " + std::to_string(total_send_cnt.load()) +
            " / Success: " + std::to_string(total_send_success_cnt.load()) +
            " / Fail: " + std::to_string(total_send_fail_cnt.load()) +
            " / Success Rate: " +
            std::to_string(
                total_send_cnt.load() > 0
                ? (double)total_send_success_cnt.load() / total_send_cnt.load() * 100
                : 0
            ) + "%\n";

        DWORD written = 0;
        WriteFile(
            hPipe,
            log_message.c_str(),
            static_cast<DWORD>(log_message.size() * sizeof(char)),
            &written,
            NULL
        );

        std::this_thread::sleep_for(std::chrono::seconds(5));
    //}
	CloseHandle(hPipe);
}
