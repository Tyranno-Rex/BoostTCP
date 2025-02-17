#include "../include/session.hpp"
#include "../include/utils.hpp"
#include "../include/memory_pool.hpp"
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>

extern MemoryPool g_memory_pool;

extern std::atomic<int>
JH_recv_packet_total_cnt;
extern std::atomic<int>
 JY_recv_packet_success_cnt;
extern std::atomic<int>
 JY_recv_packet_fail_cnt;

extern std::atomic<int>
 YJ_recv_packet_total_cnt;
extern std::atomic<int>
 YJ_recv_packet_success_cnt;
extern std::atomic<int>
 YJ_recv_packet_fail_cnt;

extern std::atomic<int>
 ES_recv_packet_total_cnt;
extern std::atomic<int>
 ES_recv_packet_success_cnt;
extern std::atomic<int>
 ES_recv_packet_fail_cnt;

void Session::stop() {
	socket.close();
}

void Session::doRead() {
	// shared_from_this()�� ����Ͽ� �ڱ� �ڽ��� �����ϰ� �ִ� shared_ptr�� ����
    auto self = shared_from_this();

	// �޸� Ǯ���� ���۸� �Ҵ�޾Ƽ� �б� �۾��� ����
    current_buffer = g_memory_pool.acquire();

	// �񵿱� �б� �۾��� ����
    socket.async_read_some(
        boost::asio::buffer(current_buffer),
		// �񵿱� �۾� �Ϸ� �� ȣ��Ǵ� �ݹ� �Լ�
        [this, self](const boost::system::error_code& error, size_t bytes_transferred) {
            if (!error) {
				// ���� �����͸� ��Ŷ ť�� �־ ó��
                std::vector<char> buffer(current_buffer.begin(),
                    current_buffer.begin() + bytes_transferred);

                PacketTask task(std::move(buffer), bytes_transferred, self);
                server.getPacketQueue().push(std::move(task));

				// ���� �б� �۾��� ����
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


		// ��Ŷ Ÿ�Կ� ���� ī��Ʈ ����
		if (packet->header.type == PacketType::JH) {
			JH_recv_packet_total_cnt++;
		}
		else if (packet->header.type == PacketType::YJ) {
			YJ_recv_packet_total_cnt++;
		}
		else if (packet->header.type == PacketType::ES) {
			ES_recv_packet_total_cnt++;
        }
        else {
			LOGI << "Unknown packet type";
        }

        // üũ�� ����
        std::vector<char> payload_data(packet->payload,
            packet->payload + sizeof(packet->payload));
        /*
        auto calculated_checksum = calculate_checksum(payload_data);
        if (std::memcmp(packet->header.checkSum,
            calculated_checksum.data(),
            MD5_DIGEST_LENGTH) != 0) {

            // ��Ŷ Ÿ�Կ� ���� ī��Ʈ ����
            if (packet->header.type == PacketType::JH) {
                JY_recv_packet_fail_cnt++;
            }
            else if (packet->header.type == PacketType::YJ) {
				YJ_recv_packet_fail_cnt++;
            }
            else if (packet->header.type == PacketType::ES) {
				ES_recv_packet_fail_cnt++;
            }
            
			LOGE << "Checksum validation failed";
            packet_buffer_offset = 0;
            return;
        }
        */

        // tail �� ����
        if (packet->tail.value != 255) {

			// ��Ŷ Ÿ�Կ� ���� ī��Ʈ ����
			if (packet->header.type == PacketType::JH) {
				JY_recv_packet_fail_cnt++;
			}
			else if (packet->header.type == PacketType::YJ) {
				YJ_recv_packet_fail_cnt++;
			}
			else if (packet->header.type == PacketType::ES) {
				ES_recv_packet_fail_cnt++;
			}

			LOGE << "Invalid tail value";
            packet_buffer_offset = 0;
            return;
        }

        // �޽��� ó��
        std::string message(packet->payload, sizeof(packet->payload));
		LOGI << message;
        
		// ��Ŷ Ÿ�Կ� ���� ī��Ʈ ����
		if (packet->header.type == PacketType::JH) {
			JY_recv_packet_success_cnt++;
		}
		else if (packet->header.type == PacketType::YJ) {
			YJ_recv_packet_success_cnt++;
		}
		else if (packet->header.type == PacketType::ES) {
			ES_recv_packet_success_cnt++;
		}

        if (ES_recv_packet_success_cnt > ES_recv_packet_total_cnt) {
			LOGE << "ES success count is greater than total count";
        }

        // ó���� ��Ŷ ����
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