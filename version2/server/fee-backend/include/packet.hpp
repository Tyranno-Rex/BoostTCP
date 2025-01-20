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
    std::map<PacketType, std::vector<char>> type_buffers;

public:
	PacketBuffer() {
	}
    void append(const char* data, size_t size) {
        if (size < sizeof(PacketHeader)) {
            return;
        }

        // 데이터의 시작점에서 패킷 헤더를 읽음
        const PacketHeader* header = reinterpret_cast<const PacketHeader*>(data);
        PacketType type = header->type;

        // 해당 타입의 버퍼에 데이터 추가
        auto& buffer = type_buffers[type];
        buffer.insert(buffer.end(), data, data + size);
    }

    std::optional<Packet> extractPacket() {
        // 각 타입별 버퍼에서 완성된 패킷 확인
        for (auto& [type, buffer] : type_buffers) {
            if (buffer.size() >= sizeof(Packet)) {
                // 패킷 헤더 확인
                const PacketHeader* header = reinterpret_cast<const PacketHeader*>(buffer.data());
                size_t total_size = sizeof(Packet);

                // 완성된 패킷이 있으면 추출
                if (buffer.size() >= total_size) {
                    Packet packet;
                    std::memcpy(&packet, buffer.data(), sizeof(Packet));

                    // 처리된 데이터는 버퍼에서 제거
                    buffer.erase(buffer.begin(), buffer.begin() + total_size);

                    return packet;
                }
            }
        }
        return std::nullopt;
    }

    void clear() {
        type_buffers.clear();
    }

    void packet_clear() {
        for (auto& [type, buffer] : type_buffers) {
            if (!buffer.empty() && buffer.size() >= sizeof(Packet)) {
                buffer.erase(buffer.begin(), buffer.begin() + sizeof(Packet));
            }
        }
    }
};

/*
class PacketBuffer {
private:
    std::vector<char> buffer;
    size_t packet_size;

public:
    PacketBuffer() : packet_size(sizeof(Packet)) {
        buffer.reserve(packet_size);
    }

	void setPacketSize(size_t size) {
		buffer.reserve(size);
	}

    void append(const char* data, size_t length) {
        buffer.insert(buffer.end(), data, data + length);
    }

    bool hasCompletePacket() const {
		return buffer.size() >= packet_size;
    }

	bool hasOompleteHeader() const {
		return buffer.size() >= sizeof(PacketHeader);
	}

    std::optional<Packet> extractPacket() {
        if (buffer.size() < sizeof(PacketHeader)) {
            return std::nullopt;
        }
        PacketHeader header;
        std::memcpy(&header, buffer.data(), sizeof(PacketHeader));
        if (buffer.size() < header.size) {
            return std::nullopt;
        }
        Packet packet;
		std::memcpy(&packet, buffer.data(), sizeof(Packet));
        // 유효성 검사: tail.value 확인
        if (packet.tail.value != 255) {
            // 유효하지 않은 데이터 -> 헤더 크기만큼 삭제
            buffer.erase(buffer.begin(), buffer.begin() + sizeof(PacketHeader));
            return std::nullopt;
        }
        // 유효한 패킷 -> 처리 후 buffer에서 제거
		buffer.erase(buffer.begin(), buffer.begin() + sizeof(Packet));
        return packet;
    }

	std::string debugBuffer() {
		return std::string(buffer.begin(), buffer.end());
	}
		

	size_t getPacketSize() const {
        return packet_size;
    }

    size_t size() const {
        return buffer.size();
    }

    void clear() {
        buffer.clear();
    }

	void packet_clear() {
		buffer.erase(buffer.begin(), buffer.begin() + packet_size);
	}

	const char* data() const {
		return buffer.data();
	}
};
*/
