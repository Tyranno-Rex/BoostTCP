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
    const size_t PACKET_SIZE = 150; // 패킷 크기
    size_t processed = 0;

    while (processed + PACKET_SIZE <= temp_buffer.size()) {
        // 벡터를 이동하는 대신 복사하거나 참조를 사용
        auto packet = std::make_unique<std::vector<char>>(
            temp_buffer.begin() + processed,
            temp_buffer.begin() + processed + PACKET_SIZE
        );

        // 패킷 큐에 추가
        PacketTask task(std::move(packet), PACKET_SIZE);
        server.getPacketQueue().push(std::move(task));

        processed += PACKET_SIZE;
    }

    // 남은 데이터가 있으면 불완전 패킷으로 저장
    if (processed < temp_buffer.size()) {
        partial_packet_buffer.assign(temp_buffer.begin() + processed,
            temp_buffer.end());
    }
}

void processPacketInWorker(std::unique_ptr<std::vector<char>>& data, size_t size) {
    const size_t PACKET_SIZE = 150;

    size_t processed = 0;
    while (processed + PACKET_SIZE <= size) {
        std::vector<char> packet(data->begin() + processed, data->begin() + processed + PACKET_SIZE);

        if (packet[0] == 101) {
            JH_recv_packet_total_cnt++;
        }
        else if (packet[0] == 102) {
            YJ_recv_packet_total_cnt++;
        }
        else if (packet[0] == 103) {
            ES_recv_packet_total_cnt++;
        }
        else {
            LOGE << "Unknown packet type";
        }

        if (packet[149] != -1) {
            if (packet[0] == 101) {
                JY_recv_packet_fail_cnt++;
            }
            else if (packet[0] == 102) {
                YJ_recv_packet_fail_cnt++;
            }
            else if (packet[0] == 103) {
                ES_recv_packet_fail_cnt++;
            }
            LOGE << "Invalid tail value";
            return;
        }

        std::string message(packet.begin() + 21, packet.begin() + 21 + 128);
        std::string total_send_cnt = std::to_string(JH_recv_packet_total_cnt + YJ_recv_packet_total_cnt + ES_recv_packet_total_cnt);

        LOGI << message;

        if (packet[0] == 101) {
            JY_recv_packet_success_cnt++;
        }
        else if (packet[0] == 102) {
            YJ_recv_packet_success_cnt++;
        }
        else if (packet[0] == 103) {
            ES_recv_packet_success_cnt++;
        }
        else {
            LOGE << "Unknown packet type";
        }

        processed += PACKET_SIZE;
    }
}