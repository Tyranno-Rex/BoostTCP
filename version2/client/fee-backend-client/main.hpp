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
    PacketType type;           // 기본 : 100
    char checkSum[16];
    uint32_t size;
};

struct PacketTail {
    uint8_t value;
};

struct Packet {
    PacketHeader header;
    char payload[128];
    PacketTail tail;              // 기본 : 255
};
#pragma pack(pop)

std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data);
std::string printMessageWithTime(const std::string& message, bool isDebug);