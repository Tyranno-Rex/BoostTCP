//#pragma once
//#include <vector>
//#include <optional>
//
//// PacketType: 패킷을 구분을 위한 변수
//enum class PacketType : uint8_t {
//    defEchoString = 100,
//    JH = 101,
//    YJ = 102,
//    ES = 103,
//};
//
//// pack(push, 1): 
//#pragma pack(push, 1)
//
//struct PacketHeader {
//	PacketType type;           // 패킷 타입: 100
//    char checkSum[16];
//    uint32_t size;
//};
//
//struct PacketTail {
//    uint8_t value;
//};
//
//struct Packet {
//    PacketHeader header;
//    char payload[128];
//    PacketTail tail;              // 총 
//};
//#pragma pack(pop)
//
//
//class PacketBuffer {
//private:
//    std::vector<char> buffer;
//    size_t packet_size;
//
//public:
//    PacketBuffer() : packet_size(sizeof(Packet)) {
//        buffer.reserve(packet_size * 2);
//    }
//
//	void setPacketSize(size_t size) {
//		buffer.reserve(size);
//	}
//
//    void append(const char* data, size_t length) {
//        buffer.insert(buffer.end(), data, data + length);
//    }
//
//    bool hasCompletePacket() const {
//		return buffer.size() >= packet_size;
//    }
//
//	bool hasOompleteHeader() const {
//		return buffer.size() >= sizeof(PacketHeader);
//	}
//
//    std::optional<Packet> extractPacket() {
//        if (buffer.size() < packet_size) {
//            return std::nullopt;
//        }
//
//        Packet packet;
//        std::memcpy(&packet, buffer.data(), packet_size);
//
//        if (size_t(packet.tail.value) != 255) {
//			buffer.erase(buffer.begin(), buffer.begin() + packet_size);
//            return std::nullopt;
//        }
//
//        // Remove processed data from the buffer
//        buffer.erase(buffer.begin(), buffer.begin() + packet_size);
//
//        return packet;
//    }
//
//	std::string debugBuffer() {
//		return std::string(buffer.begin(), buffer.end());
//	}
//		
//
//	size_t getPacketSize() const {
//        return packet_size;
//    }
//
//    size_t size() const {
//        return buffer.size();
//    }
//
//    void clear() {
//        buffer.clear();
//    }
//
//	void packet_clear() {
//		buffer.erase(buffer.begin(), buffer.begin() + packet_size);
//	}
//
//	const char* data() const {
//		return buffer.data();
//	}
//};


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
    size_t read_pos = 0;
    size_t write_pos = 0;
    size_t packet_size;
    static const size_t INITIAL_BUFFER_SIZE = 4096;  // 4KB (약 28개의 패킷을 저장 가능)

public:
    PacketBuffer() :
        packet_size(sizeof(Packet)),
        buffer(INITIAL_BUFFER_SIZE) {
    }

    void setPacketSize(size_t size) {
        packet_size = size;
        if (buffer.size() < size * 2) {
            buffer.resize(size * 2);
        }
    }

    void append(const char* data, size_t length) {
        if (write_pos + length > buffer.size()) {
            compact();

            if (write_pos + length > buffer.size()) {
                buffer.resize(std::max(buffer.size() * 2, write_pos + length));
            }
        }

        std::memcpy(buffer.data() + write_pos, data, length);
        write_pos += length;
    }

    bool hasCompletePacket() const {
        return (write_pos - read_pos) >= packet_size;
    }

    bool hasOompleteHeader() const {
        return (write_pos - read_pos) >= sizeof(PacketHeader);
    }

    std::optional<Packet> extractPacket() {
        if ((write_pos - read_pos) < packet_size) {
            return std::nullopt;
        }

        Packet packet;
        std::memcpy(&packet, buffer.data() + read_pos, packet_size);

        if (size_t(packet.tail.value) != 255) {
			std::cout << "compact" << std::endl;
            read_pos += packet_size;
            if (read_pos >= buffer.size() / 2) {
                compact();
            }
            return std::nullopt;
        }

        read_pos += packet_size;
        if (read_pos >= buffer.size() / 2) {
            compact();
        }

        return packet;
    }

    std::string debugBuffer() const {
        return std::string(buffer.data() + read_pos, buffer.data() + write_pos);
    }

    size_t getPacketSize() const {
        return packet_size;
    }

    size_t size() const {
        return write_pos - read_pos;
    }

    void clear() {
        read_pos = write_pos = 0;
    }

    void packet_clear() {
        if (read_pos + packet_size <= write_pos) {
            read_pos += packet_size;
            if (read_pos >= buffer.size() / 2) {
                compact();
            }
        }
    }

    const char* data() const {
        return buffer.data() + read_pos;
    }

private:
    void compact() {
        if (read_pos == 0) return;

        size_t remaining = write_pos - read_pos;
        if (remaining > 0) {
            std::memmove(buffer.data(), buffer.data() + read_pos, remaining);
        }
        write_pos = remaining;
        read_pos = 0;
    }
};