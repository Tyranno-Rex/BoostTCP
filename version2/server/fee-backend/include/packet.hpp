#pragma once
#include <vector>
#include <optional>

// PacketType: 패킷을 구분을 위한 변수
enum class PacketType : uint8_t {
    defEchoString = 100,
    JH = 101,
    YJ = 102,
    ES = 103,
};

// pack(push, 1): 
#pragma pack(push, 1)
struct PacketHeader { 		// header size = 25	
	uint32_t seqNum;			// size = 4
	PacketType type;		// size = 1   
	char checkSum[16];		// size = 16	
	uint32_t size;			// size = 4
};

struct PacketTail {			// tail size = 1
	uint8_t value;          // size = 1
};

struct Packet {				// packet size = 154
	PacketHeader header;    // size = 25
	char payload[128];      // size = 128
	PacketTail tail;        // size = 1
};
#pragma pack(pop)
