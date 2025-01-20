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
    PacketHeader header;
    char payload[128];
    PacketTail tail;              // �� 
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

        // �������� ���������� ��Ŷ ����� ����
        const PacketHeader* header = reinterpret_cast<const PacketHeader*>(data);
        PacketType type = header->type;

        // �ش� Ÿ���� ���ۿ� ������ �߰�
        auto& buffer = type_buffers[type];
        buffer.insert(buffer.end(), data, data + size);
    }

    std::optional<Packet> extractPacket() {
        // �� Ÿ�Ժ� ���ۿ��� �ϼ��� ��Ŷ Ȯ��
        for (auto& [type, buffer] : type_buffers) {
            if (buffer.size() >= sizeof(Packet)) {
                // ��Ŷ ��� Ȯ��
                const PacketHeader* header = reinterpret_cast<const PacketHeader*>(buffer.data());
                size_t total_size = sizeof(Packet);

                // �ϼ��� ��Ŷ�� ������ ����
                if (buffer.size() >= total_size) {
                    Packet packet;
                    std::memcpy(&packet, buffer.data(), sizeof(Packet));

                    // ó���� �����ʹ� ���ۿ��� ����
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
        // ��ȿ�� �˻�: tail.value Ȯ��
        if (packet.tail.value != 255) {
            // ��ȿ���� ���� ������ -> ��� ũ�⸸ŭ ����
            buffer.erase(buffer.begin(), buffer.begin() + sizeof(PacketHeader));
            return std::nullopt;
        }
        // ��ȿ�� ��Ŷ -> ó�� �� buffer���� ����
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
