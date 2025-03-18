#pragma once
#include <vector>
#include <optional>
#include <unordered_map>
#include <atomic>
#include <mutex>

class PacketChecker {
private:
    std::unordered_map<int, std::atomic<int>> last_seq;
    std::mutex mtx;  // delete_key를 위한 mutex

public:
    bool is_in_order(int key, int seq) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = last_seq.find(key);
        if (it == last_seq.end()) {
            last_seq[key] = seq;
            return true;
        }

        int expected = it->second.load();
        if (seq > expected) {
            it->second.store(seq);
            return true;
        }

        LOGE << "[SEQUENCE ERROR] [" << key << "] Expected: " << expected + 1 << ", but got : " << seq;
        return false;
    }


    void delete_key(int key) {
        std::lock_guard<std::mutex> lock(mtx);
        if (key % 1000 == 0 || key % 1000 == 500) {
            size_t start = key - 1000;
            size_t end = key - 500;
            for (size_t i = start; i < end; i++) {
				if (last_seq.find(i) != last_seq.end())
                    last_seq.erase(i);
            }
        }
    }
};

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
