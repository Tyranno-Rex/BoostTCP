#pragma once
#include <array>
#include <vector>
#include <stdexcept>
#include <memory>
#include <openssl/evp.h>

#include "main.hpp"

extern std::queue<std::string> command_queue;
extern std::condition_variable command_cond_var;

std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data);
std::string printMessageWithTime(const std::string& message, bool isDebug);