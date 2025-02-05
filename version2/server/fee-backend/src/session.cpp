#include "../include/session.hpp"
#include "../include/utils.hpp"
#include "../include/memory_pool.hpp"
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>

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
                std::vector<char> buffer(current_buffer.begin(),
                    current_buffer.begin() + bytes_transferred);

                PacketTask task(std::move(buffer), bytes_transferred, self);
                server.getPacketQueue().push(std::move(task));

                g_memory_pool.release(current_buffer);
                doRead();
            }
            else {
                std::cerr << "Client disconnected" << std::endl;
				LOGI << "Client disconnected";
                g_memory_pool.release(current_buffer);
                stop();
                server.removeClient(self);
            }
        });
}

void Session::processPacketInWorker(std::unique_ptr<std::vector<char>>& data, size_t size) {
    std::lock_guard<std::mutex> lock(packet_mutex);

    if (packet_buffer_offset + size > packet_buffer.size()) {
		LOGE << "Buffer overflow";
        packet_buffer_offset = 0;
        return;
    }

    std::memcpy(packet_buffer.data() + packet_buffer_offset,
        data->data(),
        size);
    packet_buffer_offset += size;

    while (packet_buffer_offset >= sizeof(Packet)) {
        Packet* packet = reinterpret_cast<Packet*>(packet_buffer.data());

        // 체크섬 검증
        std::vector<char> payload_data(packet->payload,
            packet->payload + sizeof(packet->payload));
        auto calculated_checksum = calculate_checksum(payload_data);
        if (std::memcmp(packet->header.checkSum,
            calculated_checksum.data(),
            MD5_DIGEST_LENGTH) != 0) {
			LOGE << "Checksum validation failed";
            packet_buffer_offset = 0;
            return;
        }

        // tail 값 검증
        if (packet->tail.value != 255) {
			LOGE << "Invalid tail value";
            packet_buffer_offset = 0;
            return;
        }

        // 메시지 처리
        std::string message(packet->payload, sizeof(packet->payload));
		//printMessageWithTime(message, true);
		LOGI << message;

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
}