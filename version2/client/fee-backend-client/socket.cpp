#pragma once
#include "main.hpp"
#include "socket.hpp"
#include "globals.hpp"

void SocketHandler::handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num, std::shared_ptr<std::atomic<int>> pending_ops) {
    try {
        for (int i = 0; i < connection_cnt; ++i) {
            try {
				LOGE << "Starting debug mode with " << thread_num << " threads and " << connection_cnt << " connections.";
                // 패킷 생성
                auto packet_data = std::make_shared<Packet>();
                packet_data->header.type = PacketType::ES;
                packet_data->tail.value = 255;

                // 메시지를 payload에 복사
                std::string msg = std::to_string(total_send_cnt) + " / " + message;
                // 메시지를 최대 128자로 제한
                msg.resize(std::min(msg.size(), (size_t)128), ' ');

                // payload 초기화 및 메시지 복사
                std::memset(packet_data->payload, 0, sizeof(packet_data->payload));
                std::memcpy(packet_data->payload, msg.c_str(), std::min(msg.size(), (size_t)128));

                // header.size에 메시지의 크기를 저장
                packet_data->header.size = static_cast<uint32_t>(sizeof(Packet));

                // checksum을 계산하여 header.checkSum에 저장
                auto checksum = calculate_checksum(std::vector<char>(msg.begin(), msg.end()));
                std::memcpy(packet_data->header.checkSum, checksum.data(), checksum.size());

                // socket을 pool에서 가져온다.
                auto socket = socket_pool.acquire();

                // 비동기 작업 시작 시 카운터 증가
                (*pending_ops)++;

                // 핸들러 수정
                auto write_handler = [socket, socket_pool_ptr = &socket_pool, msg, pending_ops]
                (boost::system::error_code ec, std::size_t length) {
                    if (ec) {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        LOGE << "Error writing to socket: " << ec.message();
                        total_send_cnt++;
                        total_send_fail_cnt++;
                    }
                    else {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        LOGI << msg;
                        total_send_cnt++;
                        total_send_success_cnt++;
                    }
                    socket_pool_size = socket_pool_ptr->open_socket_count();
                    // 소켓을 풀에 반환
                    socket_pool_ptr->release(socket);

                    // 작업 완료 시 카운터 감소
                    (*pending_ops)--;
                    };

                // 비동기 쓰기 시작
                //boost::asio::async_write(
                //    *socket,
                //    boost::asio::buffer(packet_data.get(), sizeof(Packet)),
                //    [packet_data, write_handler](boost::system::error_code ec, std::size_t length) {
                //        write_handler(ec, length);
                //    }
                //);

                auto self(shared_from_this());
                boost::asio::async_write(*socket, boost::asio::buffer(packet_data.get(), sizeof(Packet)),
	                std::bind(write_handler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

                );
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


/*
void handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num) {
    try {
        for (int i = 0; i < connection_cnt; ++i) {
            try {
                // 패킷 생성
                Packet packet;
                packet.header.type = PacketType::ES;
                packet.tail.value = 255;

                // 메시지를 payload에 복사
                //std::string msg = std::to_string(thread_num) + ":" + std::to_string(i) + " " + message;
                std::string msg = std::to_string(total_send_cnt) + " / " + message;

                // 메시지를 최대 128자로 제한
                msg.resize(std::min(msg.size(), (size_t)128), ' ');

                // memset을 통해서 payload를 0으로 초기화
                std::memset(packet.payload, 0, sizeof(packet.payload));
                // memcpy를 통해서 payload에 메시지를 복사
                std::memcpy(packet.payload, msg.c_str(), std::min(msg.size(), (size_t)128));
                // header.size에 메시지의 크기를 저장
				packet.header.size = static_cast<uint32_t>(sizeof(packet));

				//std::memcpy(packet.header.checkSum, "1234567890123456", 16);
                // checksum을 계산하여 header.checkSum에 저장
                auto checksum = calculate_checksum(std::vector<char>(msg.begin(), msg.end()));
				std::memcpy(packet.header.checkSum, checksum.data(), checksum.size());

                // socket을 pool에서 가져온다.
                auto socket = socket_pool.acquire();
               
                // 에러 처리
                boost::system::error_code ec;
				
                // write를 통해서 패킷을 전송
                boost::asio::write(*socket, boost::asio::buffer(&packet, sizeof(packet)), ec);

                if (ec) {
	                std::lock_guard<std::mutex> lock(cout_mutex);
	                LOGE << "Error writing to socket: " << ec.message();
				    total_send_cnt++;
	                total_send_fail_cnt++;
                }
                else {
	                std::lock_guard<std::mutex> lock(cout_mutex);
	                LOGI << msg;
                    total_send_cnt++;
                    total_send_success_cnt++;
                }
				// socket을 pool에 반환
				 socket_pool.release(socket);

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
*/



//// 패킷을 헤더와 페이로드로 나누어 전송
//size_t total_size = sizeof(packet);
//size_t header_size = sizeof(packet.header);
//// 첫 번째 부분 전송
//boost::asio::write(*socket, boost::asio::buffer(&packet, header_size));
//// 4개로 나누어 전송 (1/4)
//boost::asio::write(*socket, boost::asio::buffer(packet.payload, sizeof(packet.payload) / 4));
//// 4개로 나누어 전송 (2/4)
//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 4, sizeof(packet.payload) / 4));
//// 4개로 나누어 전송 (3/4)
//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 2, sizeof(packet.payload) / 4));
//// 4개로 나누어 전송 (4/4)
//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 4 * 3, sizeof(packet.payload) / 4 + 1));

//// 127.0.0.1:7777으로 연결
//boost::asio::io_context io_context;
//auto socket = std::make_shared<tcp::socket>(io_context);
//tcp::resolver resolver(io_context);
//auto endpoints = resolver.resolve("127.0.0.1", "7777");
//boost::asio::connect(*socket, endpoints);
//// async_write를 통해서 패킷을 전송
//boost::asio::async_write(*socket, boost::asio::buffer(&packet, sizeof(packet)), [](const boost::system::error_code& ec, std::size_t) {
//	if (ec) {
//		std::lock_guard<std::mutex> lock(cout_mutex);
//		std::cerr << "Error writing to socket: " << ec.message() << std::endl;
//	}
//	});
//// io_context를 실행
//io_context.run();
//// io_context를 reset
//io_context.restart();
//// socket을 닫음
//socket->close();


/*
boost::asio::async_write(*socket, boost::asio::buffer(&packet, sizeof(packet)),
    [msg, &socket_pool, socket](const boost::system::error_code& ec, std::size_t) {
    if (ec) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        LOGE << "Error writing to socket: " << ec.message();
        total_send_fail_cnt++;
    }
    else {
        std::lock_guard<std::mutex> lock(cout_mutex);
        LOGI << msg;
        total_send_success_cnt++;

        // socket을 pool에 반환
        socket_pool.release(socket);

    }
});
socket_pool.release(socket);
*/