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
 YJ_recv_packet_total_cnt2;

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

// --------------------------------------------------------------------------------------------
//void Session::doRead() {
//	// shared_from_this()�� ����Ͽ� �ڱ� �ڽ��� �����ϰ� �ִ� shared_ptr�� ����
//    auto self = shared_from_this();
//
//	// �޸� Ǯ���� ���۸� �Ҵ�޾Ƽ� �б� �۾��� ����
//    current_buffer = g_memory_pool.acquire();
//
//	// �񵿱� �б� �۾��� ����
//    socket.async_read_some(
//        boost::asio::buffer(current_buffer),
//		// �񵿱� �۾� �Ϸ� �� ȣ��Ǵ� �ݹ� �Լ�
//        [this, self](const boost::system::error_code& error, size_t bytes_transferred) {
//            if (!error) {
//				// ���� �����͸� ��Ŷ ť�� �־ ó��
//                std::vector<char> buffer(current_buffer.begin(),
//                    current_buffer.begin() + bytes_transferred);
//
//                PacketTask task(std::move(buffer), bytes_transferred, self);
//                server.getPacketQueue().push(std::move(task));
//
//				// ���� �б� �۾��� ����
//                g_memory_pool.release(current_buffer);
//                doRead();
//            }
//            else {
//                std::cerr << "Client disconnected" << std::endl;
//				LOGI << "Client disconnected";
//                g_memory_pool.release(current_buffer);
//                stop();
//                server.removeClient(self);
//            }
//        });
//}
// --------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------
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
    //std::lock_guard<std::mutex> lock(read_mutex);
    if (bytes_transferred != 150) {
		LOGE << "Received not 150 bytes " << bytes_transferred;
    }
	YJ_recv_packet_total_cnt2++;

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
        // ������ ��Ŷ�� �����Ͽ� ó��
        std::vector<char> packet(temp_buffer.begin() + processed,
            temp_buffer.begin() + processed + PACKET_SIZE);

        // ��Ŷ ť�� �߰�
        PacketTask task(std::move(packet), PACKET_SIZE, shared_from_this());
        server.getPacketQueue().push(std::move(task));

        processed += PACKET_SIZE;
		cnt++;
    }
    
    if (bytes_transferred != 150) {
		LOGE << "processed: " << processed << " cnt: " << cnt << " bytes_transferred: " << bytes_transferred;
    }

    // ���� �����Ͱ� ������ �ҿ��� ��Ŷ���� ����
    if (processed < temp_buffer.size()) {
        partial_packet_buffer.assign(temp_buffer.begin() + processed,
            temp_buffer.end());
    }
}
// --------------------------------------------------------------------------------------------


//void Session::doRead() {
//    // shared_from_this()�� ����Ͽ� �ڱ� �ڽ��� �����ϰ� �ִ� shared_ptr�� ����
//    auto self = shared_from_this();
//    // �޸� Ǯ���� ���۸� �Ҵ�޾Ƽ� �б� �۾��� ����
//    std::array<char, 150> buffer = g_memory_pool.acquire();
//    // �񵿱� �б� �۾��� ����
//    socket.async_read_some(
//        boost::asio::buffer(buffer),
//        // �񵿱� �۾� �Ϸ� �� ȣ��Ǵ� �ݹ� �Լ�
//        [this, self, &buffer](const boost::system::error_code& error, size_t bytes_transferred) mutable {
//            if (!error) {
//                // ���� �����͸� ó��
//                handleReceivedData(buffer, bytes_transferred);
//                // ���� �б� �۾��� ����
//                g_memory_pool.release(buffer);
//                doRead();
//            }
//            else {
//                std::cerr << "Client disconnected" << std::endl;
//                LOGI << "Client disconnected";
//                g_memory_pool.release(buffer);
//                stop();
//                server.removeClient(self);
//            }
//        });
//}
//
//// ������ ó�� �Լ� �߰�
//void Session::handleReceivedData(std::array<char, 150>& buffer, size_t bytes_transferred) {
//    // ���� ���� �����͸� �ӽ� ���ۿ� ����
//    std::vector<char> temp_buffer(buffer.begin(), buffer.begin() + bytes_transferred);
//
//    // ������ ����� �ҿ��� ��Ŷ�� ������ ���� ���� �����Ϳ� ��ħ
//    if (!partial_packet_buffer.empty()) {
//        temp_buffer.insert(temp_buffer.begin(), partial_packet_buffer.begin(), partial_packet_buffer.end());
//        partial_packet_buffer.clear();
//    }
//
//    // ������ ��Ŷ ������ ó��
//    const size_t PACKET_SIZE = 150; // ��Ŷ ũ��
//    size_t processed = 0;
//
//    while (processed + PACKET_SIZE <= temp_buffer.size()) {
//        // ������ ��Ŷ�� �����Ͽ� ó��
//        std::vector<char> packet(temp_buffer.begin() + processed, temp_buffer.begin() + processed + PACKET_SIZE);
//
//        // ��Ŷ ť�� �߰�
//        PacketTask task(std::move(packet), PACKET_SIZE, shared_from_this());
//        server.getPacketQueue().push(std::move(task));
//
//        processed += PACKET_SIZE;
//    }
//
//    // ���� �����Ͱ� ������ �ҿ��� ��Ŷ���� ����
//    if (processed < temp_buffer.size()) {
//        partial_packet_buffer.assign(temp_buffer.begin() + processed, temp_buffer.end());
//    }
//}



















#include <fstream>

std::mutex log_mutex;
std::ofstream logFile("log.txt", std::ios::app);  // ���α׷� ���� �� ���� ����

void logError(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (logFile.is_open()) {
        logFile << "[ERROR] " << message << std::endl;
    }
}

void logInfo(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (logFile.is_open()) {
        logFile << "[INFO] " << message << std::endl;
    }
}

// ���α׷� ���� �� ȣ���Ͽ� ���� �ݱ�
void closeLogFile() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

//enum class PacketType : uint8_t {
//    defEchoString = 100,
//    JH = 101,
//    YJ = 102,
//    ES = 103,
//};

//struct PacketHeader {
//    PacketType type;           // ��Ŷ Ÿ��: 100
//    char checkSum[16];
//    uint32_t size;
//};
//
//struct PacketTail {
//    uint8_t value;
//};
//
//struct Packet {
//    PacketHeader header; // size = 21
//    char payload[128];   // size = 128
//    PacketTail tail;    // size = 1
//};


void Session::processPacketInWorker(std::unique_ptr<std::vector<char>>& data, size_t size) {
    std::lock_guard<std::mutex> lock(packet_mutex);

	// ���ʿ� �Ѿ���� ���� 150�� ������ �ű��� �ʰ�, �׳� �ٷ� �м��ϰ� ó���Ѵ�.
    
	/*PacketType pt = data->at(0) == 101 ? PacketType::JH : data->at(0) == 102 ? PacketType::YJ : PacketType::ES;
	std::cout << "PacketType: " << static_cast<int>(pt) << std::endl;*/

	if (data->at(0) == 101) {
		JH_recv_packet_total_cnt++;
	}
	else if (data->at(0) == 102) {
		YJ_recv_packet_total_cnt++;
	}
	else if (data->at(0) == 103) {
		ES_recv_packet_total_cnt++;
	}

	// üũ�� ����
	std::vector<char> payload_data(data->begin() + 1, data->begin() + 1 + 128);
	auto calculated_checksum = calculate_checksum(payload_data);
	if (std::memcmp(data->data() + 17,
		calculated_checksum.data(),
		MD5_DIGEST_LENGTH) != 0) {

		if (data->at(0) == 101) {
			JY_recv_packet_fail_cnt++;
		}
		else if (data->at(0) == 102) {
			YJ_recv_packet_fail_cnt++;
		}
		else if (data->at(0) == 103) {
			ES_recv_packet_fail_cnt++;
		}
		LOGE << "Checksum validation failed";
		return;
	}


	// tail �� ����
	if (data->at(149) != 255) {
		if (data->at(0) == 101) {
			JY_recv_packet_fail_cnt++;
		}
		else if (data->at(0) == 102) {
			YJ_recv_packet_fail_cnt++;
		}
		else if (data->at(0) == 103) {
			ES_recv_packet_fail_cnt++;
		}
		LOGE << "Invalid tail value";
		return;
	}

	// �޽��� ó��
	std::string message(data->begin() + 1, data->begin() + 1 + 128);
	std::string total_send_cnt = std::to_string(JH_recv_packet_total_cnt + YJ_recv_packet_total_cnt + ES_recv_packet_total_cnt);



    




    /*
   
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
		std::string total_send_cnt = std::to_string(JH_recv_packet_total_cnt + YJ_recv_packet_total_cnt + ES_recv_packet_total_cnt);
		//LOGI << message;
		logInfo(total_send_cnt + " " + message);

        
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
    */
}

// ��α׸� ���� ���� ����
// async_read_some�Լ��� current_buffer�� 150�� �����ϸ� 150��ŭ �˾Ƽ� �ľ����شٰ� �����ϰ� ������ �Ѿ.
// �ش� �Լ��� ������ ���� ��Ȳ�� ���ؼ� �߰��Ǿ���.
// 1. ��Ŷ�� 330 ����Ʈ�� ������ �ڿ� 120 ����Ʈ�� ���´ٰ� ����.
// 2. ���� �ڵ忡�� ��Ŷ�� 150, 150�� ó���ϰ�, ������ 30 ����Ʈ�� �����Ѵ�. 
//      ���� ������ �Ǵ� 120����Ʈ�� 30����Ʈ�� �����ϴ� ���� �ƴ϶� ���� 120����Ʈ�� �ϳ��� task�� ������ �ȴ�.
// 3. �׷��� �Ǹ� 30�� 120�� �ǹ̰� ���� ���̱� ������ �ش� ��Ŷ�� ������ ���ǵǴ� ���̴�.
// 4. ���� ������ ����� �ҿ��� ��Ŷ�� ������ ���� ���� �����Ϳ� ��ġ�� ������ �ʿ��ϴ�.