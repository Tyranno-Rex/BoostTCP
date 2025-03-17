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

const size_t PACKET_SIZE = 154; // ��Ŷ ũ��

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
                // ���� �����͸� ó��
                handleReceivedData(bytes_transferred);
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

// ������ ó�� �Լ� �߰�
void Session::handleReceivedData(size_t bytes_transferred) {
    std::lock_guard<std::mutex> lock(read_mutex);

    // ���� ����, �����͸� �ӽ� ���ۿ� ����
    std::vector<char> temp_buffer(current_buffer.begin(),
        current_buffer.begin() + bytes_transferred);

    // ������ ����� �ҿ��� ��Ŷ�� ������ ���� ���� �����Ϳ� ��ħ
    if (!partial_packet_buffer.empty()) {
        temp_buffer.insert(temp_buffer.begin(),
            partial_packet_buffer.begin(),
            partial_packet_buffer.end());
        partial_packet_buffer.clear();
    }

    // ������ ��Ŷ ������ ó��
    size_t processed = 0;

    while (processed + PACKET_SIZE <= temp_buffer.size()) {
        // ���͸� �̵��ϴ� ��� �����ϰų� ������ ���
        auto packet = std::make_unique<std::vector<char>>(
            temp_buffer.begin() + processed,
            temp_buffer.begin() + processed + PACKET_SIZE
        );
        // session_id�� sequence�� ���� ����
		//PacketTask task(std::move(packet), PACKET_SIZE);
        //server.getPacketQueue().push(std::move(task));

		// session_id�� sequence�� �ִ� ����
        uint32_t received_seqnum = 0;
        std::memcpy(&received_seqnum, packet.get()->data(), sizeof(uint32_t));
		PacketTask task(std::move(packet), PACKET_SIZE, SessionID, received_seqnum);
		LOGD << "tasks sid: " << task.session_id << " seq: " << task.seq_num;
		server.getPacketQueue().push(std::move(task));

        processed += PACKET_SIZE;
    }

    // ���� �����Ͱ� ������ �ҿ��� ��Ŷ���� ����
    if (processed < temp_buffer.size()) {
        partial_packet_buffer.assign(temp_buffer.begin() + processed,
            temp_buffer.end());
    }
}


void Session::stop() {
	socket.close();
}

void processPacketInWorker(std::unique_ptr<std::vector<char>>& data, size_t size) {
	LOGD << "Packet size: " << size;
    const size_t PACKET_SIZE = 154;

    size_t processed = 0;
	uint32_t sequence = 0;
    while (processed + PACKET_SIZE <= size) {
        try {
            std::vector<char> packet(data->begin() + processed, data->begin() + processed + PACKET_SIZE);

		    // PacketHeader
		    uint32_t seq = *reinterpret_cast<uint32_t*>(packet.data());
		    PacketType type = static_cast<PacketType>(packet[4]);
		    std::string checkSum(packet.begin() + 5, packet.begin() + 21);
		    uint32_t packet_size = *reinterpret_cast<uint32_t*>(packet.data() + 21);

		    switch (type) {
		    case PacketType::defEchoString:
			    break;
		    case PacketType::JH:
			    JH_recv_packet_total_cnt++;
			    break;
		    case PacketType::YJ:
			    YJ_recv_packet_total_cnt++;
			    break;
		    case PacketType::ES:
			    ES_recv_packet_total_cnt++;
			    break;
		    default:
			    LOGE << "Unknown packet type";
			    return;
		    }

		    // PacketTail
            
			int tail = static_cast<int>(data->at(153));
		    if (tail != -1) {
				LOGE << "Invalid tail: " << tail;
			    if (type == PacketType::JH) {
				    JY_recv_packet_fail_cnt++;
                    return;
			    }
			    else if (type == PacketType::YJ) {
				    YJ_recv_packet_fail_cnt++;
                    return;
                }
			    else if (type == PacketType::ES) {
				    ES_recv_packet_fail_cnt++;
                    return;
                }
		    }
            

		    std::string message(packet.begin() + 25, packet.begin() + 25 + 128);
		    std::string total_send_cnt = std::to_string(JH_recv_packet_total_cnt + YJ_recv_packet_total_cnt + ES_recv_packet_total_cnt);

            LOGI << "seq : " << seq << " message : " << message;

		    if (type == PacketType::JH) {
			    JY_recv_packet_success_cnt++;
		    }
		    else if (type == PacketType::YJ) {
			    YJ_recv_packet_success_cnt++;
		    }
		    else if (type == PacketType::ES) {
			    ES_recv_packet_success_cnt++;
		    }
		    else {
			    LOGE << "Unknown packet type";
		    }

		    processed += PACKET_SIZE;
		}
        catch (const std::exception& e) {
            LOGE << "Error in processPacketInWorker: " << e.what();
        }
    }
}