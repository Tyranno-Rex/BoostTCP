#pragma once

#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <thread>
#include <cstdint>
#include <zlib.h>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <plog/Log.h>
#include <plog/init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/DebugOutputAppender.h>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

enum class PacketType : uint8_t {
    defEchoString = 100,
    JH = 101,
    YJ = 102,
    ES = 103,
};

#pragma pack(push, 1)
struct PacketHeader {
	PacketType type;            // 1byte
	char checkSum[16];          // 16byte
	uint32_t size;              // 4byte
};

struct PacketTail {
	uint8_t value; 		        // 1byte
};

struct Packet {
	PacketHeader header;         // 기본 : 21
	char payload[128];			// 기본 : 128	
    PacketTail tail;             // 기본 : 255
};
#pragma pack(pop)

std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data);
std::string printMessageWithTime(const std::string& message, bool isDebug);