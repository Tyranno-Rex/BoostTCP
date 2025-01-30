#pragma once
#include <string>
#include <array>
#include <vector>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <ctime>
#include <sstream>


std::string printMessageWithTime(const std::string& message, bool isDebug);
std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data);