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

const size_t PACKET_SIZE = 154; // 패킷 크기

void Session::doRead() {

    // shared_from_this()를 사용하여 자기 자신을 참조하고 있는 shared_ptr을 생성
    auto self = shared_from_this();
    // 메모리 풀에서 버퍼를 할당받아서 읽기 작업을 수행
    current_buffer = g_memory_pool.acquire();
    // 비동기 읽기 작업을 수행
    socket.async_read_some(
        boost::asio::buffer(current_buffer),
        // 비동기 작업 완료 시 호출되는 콜백 함수
        [this, self](const boost::system::error_code& error, size_t bytes_transferred) {
            if (!error) {
                // 읽은 데이터를 처리
                handleReceivedData(bytes_transferred);
                // 다음 읽기 작업을 수행
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

// 데이터 처리 함수 추가
void Session::handleReceivedData(size_t bytes_transferred) {
    std::lock_guard<std::mutex> lock(read_mutex);

    // 새로 받은, 데이터를 임시 버퍼에 복사
    std::vector<char> temp_buffer(current_buffer.begin(),
        current_buffer.begin() + bytes_transferred);

    // 이전에 저장된 불완전 패킷이 있으면 현재 받은 데이터와 합침
    if (!partial_packet_buffer.empty()) {
        temp_buffer.insert(temp_buffer.begin(),
            partial_packet_buffer.begin(),
            partial_packet_buffer.end());
        partial_packet_buffer.clear();
    }

    // 완전한 패킷 단위로 처리
    size_t processed = 0;

    while (processed + PACKET_SIZE <= temp_buffer.size()) {
        // 벡터를 이동하는 대신 복사하거나 참조를 사용
        auto packet = std::make_unique<std::vector<char>>(
            temp_buffer.begin() + processed,
            temp_buffer.begin() + processed + PACKET_SIZE
        );
        // session_id와 sequence가 없는 버전
		//PacketTask task(std::move(packet), PACKET_SIZE);
        //server.getPacketQueue().push(std::move(task));

		// session_id와 sequence가 있는 버전
        uint32_t received_seqnum = 0;
        std::memcpy(&received_seqnum, packet.get()->data(), sizeof(uint32_t));
		PacketTask task(std::move(packet), PACKET_SIZE, SessionID, received_seqnum);
		LOGD << "tasks sid: " << task.session_id << " seq: " << task.seq_num;
		server.getPacketQueue().push(std::move(task));

        processed += PACKET_SIZE;
    }

    // 남은 데이터가 있으면 불완전 패킷으로 저장
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