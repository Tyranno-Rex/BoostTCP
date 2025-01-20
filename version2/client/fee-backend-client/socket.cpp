#pragma once

#include "main.hpp"
#include "socket.hpp"

extern std::mutex cout_mutex;
extern std::mutex command_mutex;

void handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num) {
    try {
        for (int i = 0; i < connection_cnt; ++i) {
            try {
                // 패킷 생성
                Packet packet;
                packet.header.type = PacketType::ES;
                packet.tail.value = 255;

                // 메시지를 payload에 복사
                std::string msg = std::to_string(thread_num) + ":" + std::to_string(i) + " " + message;
                msg = printMessageWithTime(msg, false);
                // 메시지를 최대 128자로 제한
                msg.resize(std::min(msg.size(), (size_t)128), ' ');

                // memset을 통해서 payload를 0으로 초기화
                std::memset(packet.payload, 0, sizeof(packet.payload));
                // memcpy를 통해서 payload에 메시지를 복사
                std::memcpy(packet.payload, msg.c_str(), std::min(msg.size(), (size_t)128));
                // header.size에 메시지의 크기를 저장
				packet.header.size = static_cast<uint32_t>(sizeof(packet));

                // checksum을 계산하여 header.checkSum에 저장
                auto checksum = calculate_checksum(std::vector<char>(msg.begin(), msg.end()));
                std::memcpy(packet.header.checkSum, checksum.data(), MD5_DIGEST_LENGTH);

                // socket을 pool에서 가져온다.
                auto socket = socket_pool.acquire();
                
                // socket을 통해 패킷을 한 번에 전송
                boost::asio::write(*socket, boost::asio::buffer(&packet, sizeof(packet)));

				//// 패킷을 헤더와 페이로드로 나누어 전송
				//size_t total_size = sizeof(packet);
				//size_t header_size = sizeof(packet.header);
                // 첫 번째 부분 전송
				//boost::asio::write(*socket, boost::asio::buffer(&packet, header_size));
				//// 4개로 나누어 전송 (1/4)
				//boost::asio::write(*socket, boost::asio::buffer(packet.payload, sizeof(packet.payload) / 4));
				//// 4개로 나누어 전송 (2/4)
				//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 4, sizeof(packet.payload) / 4));
				//// 4개로 나누어 전송 (3/4)
				//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 2, sizeof(packet.payload) / 4));
				//// 4개로 나누어 전송 (4/4)
				//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 4 * 3, sizeof(packet.payload) / 4 + 1));

                // socket을 반환한다.
                socket_pool.release(socket);

                printMessageWithTime(msg, true);

            }
            catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "Error handling socket " << i << ": " << e.what() << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Error handling sockets: " << e.what() << std::endl;
    }
}

void handle_sockets2(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num) {
    try {
        for (int i = 0; i < connection_cnt; ++i) {
            try {
                Packet packet;
                packet.header.type = PacketType::ES;
                packet.tail.value = size_t(255);

                std::string msg = std::to_string(thread_num) + ":" + std::to_string(i) + " " + message;
                msg = printMessageWithTime(msg, false);
                msg.resize(std::min(msg.size(), (size_t)128), ' ');

                std::memset(packet.payload, 0, sizeof(packet.payload));
                std::memcpy(packet.payload, msg.c_str(), std::min(msg.size(), (size_t)128));
                packet.header.size = static_cast<uint32_t>(msg.size());


                auto checksum = calculate_checksum(std::vector<char>(msg.begin(), msg.end()));
                std::memcpy(packet.header.checkSum, checksum.data(), MD5_DIGEST_LENGTH);

                // 패킷을 두 개로 나누어 전송
                auto socket = socket_pool.acquire();



				//패킷자체를 보내는 게 아니라, 패킷의 버퍼자체를 보내는 방식으로 변경
				BYTE* buffer = reinterpret_cast<BYTE*>(&packet);
				boost::asio::write(*socket, boost::asio::buffer(buffer, sizeof(buffer)));

    //            // 첫 번째 부분 전송
				//boost::asio::write(*socket, boost::asio::buffer(&packet, sizeof(packet) / 2));

    //            // 두 번째 부분 전송
				//boost::asio::write(*socket, boost::asio::buffer(reinterpret_cast<char*>(&packet) + sizeof(packet) / 2, sizeof(packet) - sizeof(packet) / 2));

    //            socket_pool.release(socket);

            }
            catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "Error handling socket " << i << ": " << e.what() << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Error handling sockets: " << e.what() << std::endl;
    }
}
