#include "../include/session.hpp"
#include "../include/utils.hpp"
#include "../include/memory_pool.hpp"

extern MemoryPool g_memory_pool;

void Session::stop() {
	socket.close();
}

void Session::doRead() {
    auto self = shared_from_this();
    current_buffer = g_memory_pool.acquire();

    socket.async_read_some(
        boost::asio::buffer(current_buffer),
        [this, self](const boost::system::error_code& error, size_t bytes_transferred) {
            if (!error) {
                if (handlePacket(bytes_transferred)) {
                    g_memory_pool.release(current_buffer);
                    doRead();
                }
            }
            else {
				std::cerr << "client disconnected" << std::endl;
                // 에러 발생 시 세션 정리
                g_memory_pool.release(current_buffer);
                stop();  // 소켓 닫기
                server.removeClient(shared_from_this());  // 서버에서 제거
            }
        });
}

bool Session::handlePacket(size_t bytes_transferred) {
    // 받은 데이터를 패킷 버퍼에 추가
    if (packet_buffer_offset + bytes_transferred > packet_buffer.size()) {
        std::cerr << "Buffer overflow" << std::endl;
        packet_buffer_offset = 0;
        return false;
    }

    std::memcpy(packet_buffer.data() + packet_buffer_offset,
        current_buffer.data(),
        bytes_transferred);
    packet_buffer_offset += bytes_transferred;

    // 완전한 패킷이 수신되었는지 확인
    while (packet_buffer_offset >= sizeof(Packet)) {
        Packet* packet = reinterpret_cast<Packet*>(packet_buffer.data());

        // 체크섬 검증
        std::vector<char> payload_data(packet->payload,
            packet->payload + sizeof(packet->payload));
        auto calculated_checksum = calculate_checksum(payload_data);

        if (std::memcmp(packet->header.checkSum,
            calculated_checksum.data(),
            MD5_DIGEST_LENGTH) != 0) {
            std::cerr << "Checksum validation failed" << std::endl;
            packet_buffer_offset = 0;
            return false;
        }

        // tail 값 검증
        if (packet->tail.value != 255) {
            std::cerr << "Invalid tail value" << std::endl;
            packet_buffer_offset = 0;
            return false;
        }

        // 메시지 처리
        std::string message(packet->payload, sizeof(packet->payload));
        printMessageWithTime(message, true);

        // 처리된 패킷 제거
        if (packet_buffer_offset > sizeof(Packet)) {
            std::memmove(packet_buffer.data(),
                packet_buffer.data() + sizeof(Packet),
                packet_buffer_offset - sizeof(Packet));
            packet_buffer_offset -= sizeof(Packet);
        }
        else {
            packet_buffer_offset = 0;
        }
    }
    return true;
}