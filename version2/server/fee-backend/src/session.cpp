#include "../include/session.hpp"
#include "../include/utils.hpp"
#include "../include/memory_pool.hpp"
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>

extern MemoryPool g_memory_pool;

extern std::atomic<int>
 total_connection_packet;

extern std::atomic<int>
 JH_recv_packet_total_cnt;
extern std::atomic<int>
 JH_recv_packet_success_cnt;
extern std::atomic<int>
 JH_recv_packet_fail_cnt;

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
//	// shared_from_this()를 사용하여 자기 자신을 참조하고 있는 shared_ptr을 생성
//    auto self = shared_from_this();
//
//	// 메모리 풀에서 버퍼를 할당받아서 읽기 작업을 수행
//    current_buffer = g_memory_pool.acquire();
//
//	// 비동기 읽기 작업을 수행
//    socket.async_read_some(
//        boost::asio::buffer(current_buffer),
//		// 비동기 작업 완료 시 호출되는 콜백 함수
//        [this, self](const boost::system::error_code& error, size_t bytes_transferred) {
//            if (!error) {
//				// 읽은 데이터를 패킷 큐에 넣어서 처리
//                std::vector<char> buffer(current_buffer.begin(),
//                    current_buffer.begin() + bytes_transferred);
//
//                PacketTask task(std::move(buffer), bytes_transferred, self);
//                server.getPacketQueue().push(std::move(task));
//
//				// 다음 읽기 작업을 수행
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
    if (bytes_transferred != 150) {
		LOGE << "Received not 150 bytes " << bytes_transferred;
    }

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
	int cnt = 0;

    while (processed + PACKET_SIZE <= temp_buffer.size()) {
        // 완전한 패킷을 추출하여 처리
        std::vector<char> packet(temp_buffer.begin() + processed,
            temp_buffer.begin() + processed + PACKET_SIZE);

        // 패킷 큐에 추가
        PacketTask task(std::move(packet), PACKET_SIZE, shared_from_this());
        server.getPacketQueue().push(std::move(task));

        processed += PACKET_SIZE;
		cnt++;
    }

	total_connection_packet += cnt;
    
    // 남은 데이터가 있으면 불완전 패킷으로 저장
    if (processed < temp_buffer.size()) {
        partial_packet_buffer.assign(temp_buffer.begin() + processed,
            temp_buffer.end());
    }
}
// --------------------------------------------------------------------------------------------


//void Session::doRead() {
//    // shared_from_this()를 사용하여 자기 자신을 참조하고 있는 shared_ptr을 생성
//    auto self = shared_from_this();
//    // 메모리 풀에서 버퍼를 할당받아서 읽기 작업을 수행
//    std::array<char, 150> buffer = g_memory_pool.acquire();
//    // 비동기 읽기 작업을 수행
//    socket.async_read_some(
//        boost::asio::buffer(buffer),
//        // 비동기 작업 완료 시 호출되는 콜백 함수
//        [this, self, &buffer](const boost::system::error_code& error, size_t bytes_transferred) mutable {
//            if (!error) {
//                // 읽은 데이터를 처리
//                handleReceivedData(buffer, bytes_transferred);
//                // 다음 읽기 작업을 수행
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
//// 데이터 처리 함수 추가
//void Session::handleReceivedData(std::array<char, 150>& buffer, size_t bytes_transferred) {
//    // 새로 받은 데이터를 임시 버퍼에 복사
//    std::vector<char> temp_buffer(buffer.begin(), buffer.begin() + bytes_transferred);
//
//    // 이전에 저장된 불완전 패킷이 있으면 현재 받은 데이터와 합침
//    if (!partial_packet_buffer.empty()) {
//        temp_buffer.insert(temp_buffer.begin(), partial_packet_buffer.begin(), partial_packet_buffer.end());
//        partial_packet_buffer.clear();
//    }
//
//    // 완전한 패킷 단위로 처리
//    const size_t PACKET_SIZE = 150; // 패킷 크기
//    size_t processed = 0;
//
//    while (processed + PACKET_SIZE <= temp_buffer.size()) {
//        // 완전한 패킷을 추출하여 처리
//        std::vector<char> packet(temp_buffer.begin() + processed, temp_buffer.begin() + processed + PACKET_SIZE);
//
//        // 패킷 큐에 추가
//        PacketTask task(std::move(packet), PACKET_SIZE, shared_from_this());
//        server.getPacketQueue().push(std::move(task));
//
//        processed += PACKET_SIZE;
//    }
//
//    // 남은 데이터가 있으면 불완전 패킷으로 저장
//    if (processed < temp_buffer.size()) {
//        partial_packet_buffer.assign(temp_buffer.begin() + processed, temp_buffer.end());
//    }
//}



















#include <fstream>

std::mutex log_mutex;
std::ofstream logFile("log.txt", std::ios::app);  // 프로그램 시작 시 파일 열기

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

// 프로그램 종료 시 호출하여 파일 닫기
void closeLogFile() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Session::processPacketInWorker(std::unique_ptr<std::vector<char>>& data, size_t size) {
    std::lock_guard<std::mutex> lock(packet_mutex);

    // 패킷 크기 검증 (150 바이트인지 확인)
    if (size != sizeof(Packet)) {
        LOGE << "Invalid packet size: " << size << " expected: " << sizeof(Packet);
        return;
    }

    // data를 Packet 구조체로 해석
    Packet* packet = reinterpret_cast<Packet*>(data->data());

    // 패킷 타입에 따라 카운트 증가
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
        LOGE << "Unknown packet type: " << static_cast<int>(packet->header.type);
        return;
    }

    // 체크섬 검증
   // std::vector<char> payload_data(packet->payload, packet->payload + sizeof(packet->payload));
   // auto calculated_checksum = calculate_checksum(payload_data);
   // if (std::memcmp(packet->header.checkSum, calculated_checksum.data(), MD5_DIGEST_LENGTH) != 0) {
   //     if (packet->header.type == PacketType::JH) {
			//JH_recv_packet_total_cnt++;
   //     }
   //     else if (packet->header.type == PacketType::YJ) {
   //         YJ_recv_packet_fail_cnt++;
   //     }
   //     else if (packet->header.type == PacketType::ES) {
   //         ES_recv_packet_fail_cnt++;
   //     }
   //     LOGE << "Checksum validation failed";
   //     return;
   // }

    // tail 값 검증
    if (packet->tail.value != 255) {
        if (packet->header.type == PacketType::JH) {
            JH_recv_packet_success_cnt++;
        }
        else if (packet->header.type == PacketType::YJ) {
            YJ_recv_packet_fail_cnt++;
        }
        else if (packet->header.type == PacketType::ES) {
            ES_recv_packet_fail_cnt++;
        }
        LOGE << "Invalid tail value: " << static_cast<int>(packet->tail.value);
        return;
    }

    // 메시지 처리
    std::string message(packet->payload, sizeof(packet->payload));
    std::string total_recv_cnt = std::to_string(JH_recv_packet_total_cnt + YJ_recv_packet_total_cnt + ES_recv_packet_total_cnt);
    LOGI << total_recv_cnt + " " + message;

    // 성공 카운트 증가
    if (packet->header.type == PacketType::JH) {
		JH_recv_packet_success_cnt++;
    }
    else if (packet->header.type == PacketType::YJ) {
        YJ_recv_packet_success_cnt++;
    }
    else if (packet->header.type == PacketType::ES) {
        ES_recv_packet_success_cnt++;
    }

    




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


		// 패킷 타입에 따라 카운트 증가
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

        // 체크섬 검증
        std::vector<char> payload_data(packet->payload,
            packet->payload + sizeof(packet->payload));

        auto calculated_checksum = calculate_checksum(payload_data);
        if (std::memcmp(packet->header.checkSum,
            calculated_checksum.data(),
            MD5_DIGEST_LENGTH) != 0) {

            // 패킷 타입에 따라 카운트 증가
            if (packet->header.type == PacketType::JH) {
                JH_recv_packet_success_cnt++;
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


        // tail 값 검증
        if (packet->tail.value != 255) {

			// 패킷 타입에 따라 카운트 증가
			if (packet->header.type == PacketType::JH) {
				JH_recv_packet_success_cnt++;
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

        // 메시지 처리
        std::string message(packet->payload, sizeof(packet->payload));
		std::string total_send_cnt = std::to_string(JH_recv_packet_total_cnt + YJ_recv_packet_total_cnt + ES_recv_packet_total_cnt);
		//LOGI << message;
		logInfo(total_send_cnt + " " + message);

        
		// 패킷 타입에 따라 카운트 증가
		if (packet->header.type == PacketType::JH) {
			JH_recv_packet_success_cnt++;
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
    */
}

// 블로그를 위한 내용 정리
// async_read_some함수가 current_buffer을 150을 지정하면 150만큼 알아서 파씽해준다고 생각하고 간단히 넘어감.
// 해당 함수는 다음과 같은 상황을 위해서 추가되었다.
// 1. 패킷이 330 바이트가 들어오고 뒤에 120 바이트가 들어온다고 하자.
// 2. 이전 코드에서 패킷은 150, 150을 처리하고, 나머지 30 바이트를 저장한다. 
//      이후 들어오게 되는 120바이트도 30바이트와 결합하는 것이 아니라 따로 120바이트가 하나의 task로 저장이 된다.
// 3. 그렇게 되면 30과 120은 의미가 없는 값이기 때문에 해당 패킷의 내용은 유실되는 것이다.
// 4. 따라서 이전에 저장된 불완전 패킷이 있으면 현재 받은 데이터와 합치는 과정이 필요하다.