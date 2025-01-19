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

struct PacketHeader {
	PacketType type;           // 패킷 타입: 100
    char checkSum[16];
    uint32_t size;
};

struct PacketTail {
    uint8_t value;
};

struct Packet {
    PacketHeader header;
    char payload[128];
    PacketTail tail;              // 총 
};
#pragma pack(pop)


class PacketBuffer {
private:
    std::vector<char> buffer;
    size_t size_payload;

public:
	PacketBuffer() : size_payload(sizeof(Packet)) {
		buffer.reserve(size_payload);
    }

	void setPacketSize(size_t size) {
		buffer.reserve(size);
	}

    void append(const char* data, size_t length) {
        buffer.insert(buffer.end(), data, data + length);
    }

    bool hasCompletePacket() const {
        return buffer.size() >= size_payload;
    }

	bool hasOompleteHeader() const {
		return buffer.size() >= sizeof(PacketHeader);
	}

    std::optional<Packet> extractPacket() {
        if (buffer.size() < size_payload) {
            return std::nullopt;
        }

        Packet packet;
        std::memcpy(&packet, buffer.data(), size_payload);

		if (size_t(packet.tail.value) != 255) {
			return std::nullopt;
		}

        // 처리된 데이터를 버퍼에서 제거
        buffer.erase(buffer.begin(), buffer.begin() + size_payload);

        return packet;
    }

	std::string debugBuffer() {
		return std::string(buffer.begin(), buffer.end());
	}
		

	size_t getPacketSize() const {
		return size_payload;
	}

    size_t size() const {
        return buffer.size();
    }

    void clear() {
        buffer.clear();
    }

	const char* data() const {
		return buffer.data();
	}
};