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
ReceiveConnectionCnt;
extern std::atomic<int>
Total_Packet_Cnt;
extern std::atomic<int>
Total_Packet_Cnt2;
extern std::atomic<int>
Total_Packet_Cnt3;

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
    if (bytes_transferred != 150) {
		LOGE << "Received not 150 bytes " << bytes_transferred;
    }
	ReceiveConnectionCnt++;

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
    const size_t PACKET_SIZE = 150; // ��Ŷ ũ��
    size_t processed = 0;
	int cnt = 0;

    while (processed + PACKET_SIZE <= temp_buffer.size()) {
        // ���͸� �̵��ϴ� ��� �����ϰų� ������ ���
        auto packet = std::make_unique<std::vector<char>>(
            temp_buffer.begin() + processed,
            temp_buffer.begin() + processed + PACKET_SIZE
        );

        // ��Ŷ ť�� �߰�
        PacketTask task(std::move(packet), PACKET_SIZE);
        server.getPacketQueue().push(std::move(task));

        processed += PACKET_SIZE;
        cnt++;
    }

	Total_Packet_Cnt += cnt;
    
    if (bytes_transferred != 150) {
		LOGE << "processed: " << processed << " cnt: " << cnt << " bytes_transferred: " << bytes_transferred;
    }

    // ���� �����Ͱ� ������ �ҿ��� ��Ŷ���� ����
    if (processed < temp_buffer.size()) {
        partial_packet_buffer.assign(temp_buffer.begin() + processed,
            temp_buffer.end());
    }
}

void processPacketInWorker(std::unique_ptr<std::vector<char>>& data, size_t size) {
    const size_t PACKET_SIZE = 150;
    int cnt = 0;
    int cnt2 = 0;

    cnt2 = size / PACKET_SIZE;

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
        cnt++;
    }
    Total_Packet_Cnt2 += cnt;
    Total_Packet_Cnt3 += cnt2;
}

// ��α׸� ���� ���� ����
// async_read_some�Լ��� current_buffer�� 150�� �����ϸ� 150��ŭ �˾Ƽ� �ľ����شٰ� �����ϰ� ������ �Ѿ.
// �ش� �Լ��� ������ ���� ��Ȳ�� ���ؼ� �߰��Ǿ���.
// 1. ��Ŷ�� 330 ����Ʈ�� ������ �ڿ� 120 ����Ʈ�� ���´ٰ� ����.
// 2. ���� �ڵ忡�� ��Ŷ�� 150, 150�� ó���ϰ�, ������ 30 ����Ʈ�� �����Ѵ�. 
//      ���� ������ �Ǵ� 120����Ʈ�� 30����Ʈ�� �����ϴ� ���� �ƴ϶� ���� 120����Ʈ�� �ϳ��� task�� ������ �ȴ�.
// 3. �׷��� �Ǹ� 30�� 120�� �ǹ̰� ���� ���̱� ������ �ش� ��Ŷ�� ������ ���ǵǴ� ���̴�.
// 4. ���� ������ ����� �ҿ��� ��Ŷ�� ������ ���� ���� �����Ϳ� ��ġ�� ������ �ʿ��ϴ�.