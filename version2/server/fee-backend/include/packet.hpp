#pragma once
#include <vector>
#include <optional>

// PacketType: ��Ŷ�� ������ ���� ����
enum class PacketType : uint8_t {
    defEchoString = 100,
    JH = 101,
    YJ = 102,
    ES = 103,
};

// pack(push, 1): 
#pragma pack(push, 1)

struct PacketHeader {
	PacketType type;           // ��Ŷ Ÿ��: 100
    char checkSum[16];
    uint32_t size;
};

struct PacketTail {
    uint8_t value;
};

struct Packet {
	PacketHeader header; // size = 21
	char payload[128];   // size = 128
	PacketTail tail;    // size = 1
};
#pragma pack(pop)