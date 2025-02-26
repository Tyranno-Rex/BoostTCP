#pragma once
#include "main.hpp"
#include "socket.hpp"
#include "globals.hpp"

void SocketHandler::handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num, std::shared_ptr<std::atomic<int>> pending_ops) {
    try {
        for (int i = 0; i < connection_cnt; ++i) {
            try {
				LOGE << "Starting debug mode with " << thread_num << " threads and " << connection_cnt << " connections.";
                // ��Ŷ ����
                auto packet_data = std::make_shared<Packet>();
                packet_data->header.type = PacketType::ES;
                packet_data->tail.value = 255;

                // �޽����� payload�� ����
                std::string msg = std::to_string(total_send_cnt) + " / " + message;
                // �޽����� �ִ� 128�ڷ� ����
                msg.resize(std::min(msg.size(), (size_t)128), ' ');

                // payload �ʱ�ȭ �� �޽��� ����
                std::memset(packet_data->payload, 0, sizeof(packet_data->payload));
                std::memcpy(packet_data->payload, msg.c_str(), std::min(msg.size(), (size_t)128));

                // header.size�� �޽����� ũ�⸦ ����
                packet_data->header.size = static_cast<uint32_t>(sizeof(Packet));

                // checksum�� ����Ͽ� header.checkSum�� ����
                auto checksum = calculate_checksum(std::vector<char>(msg.begin(), msg.end()));
                std::memcpy(packet_data->header.checkSum, checksum.data(), checksum.size());

                // socket�� pool���� �����´�.
                auto socket = socket_pool.acquire();

                // �񵿱� �۾� ���� �� ī���� ����
                (*pending_ops)++;

                // �ڵ鷯 ����
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
                    // ������ Ǯ�� ��ȯ
                    socket_pool_ptr->release(socket);

                    // �۾� �Ϸ� �� ī���� ����
                    (*pending_ops)--;
                    };

                // �񵿱� ���� ����
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
                // ��Ŷ ����
                Packet packet;
                packet.header.type = PacketType::ES;
                packet.tail.value = 255;

                // �޽����� payload�� ����
                //std::string msg = std::to_string(thread_num) + ":" + std::to_string(i) + " " + message;
                std::string msg = std::to_string(total_send_cnt) + " / " + message;

                // �޽����� �ִ� 128�ڷ� ����
                msg.resize(std::min(msg.size(), (size_t)128), ' ');

                // memset�� ���ؼ� payload�� 0���� �ʱ�ȭ
                std::memset(packet.payload, 0, sizeof(packet.payload));
                // memcpy�� ���ؼ� payload�� �޽����� ����
                std::memcpy(packet.payload, msg.c_str(), std::min(msg.size(), (size_t)128));
                // header.size�� �޽����� ũ�⸦ ����
				packet.header.size = static_cast<uint32_t>(sizeof(packet));

				//std::memcpy(packet.header.checkSum, "1234567890123456", 16);
                // checksum�� ����Ͽ� header.checkSum�� ����
                auto checksum = calculate_checksum(std::vector<char>(msg.begin(), msg.end()));
				std::memcpy(packet.header.checkSum, checksum.data(), checksum.size());

                // socket�� pool���� �����´�.
                auto socket = socket_pool.acquire();
               
                // ���� ó��
                boost::system::error_code ec;
				
                // write�� ���ؼ� ��Ŷ�� ����
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
				// socket�� pool�� ��ȯ
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



//// ��Ŷ�� ����� ���̷ε�� ������ ����
//size_t total_size = sizeof(packet);
//size_t header_size = sizeof(packet.header);
//// ù ��° �κ� ����
//boost::asio::write(*socket, boost::asio::buffer(&packet, header_size));
//// 4���� ������ ���� (1/4)
//boost::asio::write(*socket, boost::asio::buffer(packet.payload, sizeof(packet.payload) / 4));
//// 4���� ������ ���� (2/4)
//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 4, sizeof(packet.payload) / 4));
//// 4���� ������ ���� (3/4)
//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 2, sizeof(packet.payload) / 4));
//// 4���� ������ ���� (4/4)
//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 4 * 3, sizeof(packet.payload) / 4 + 1));

//// 127.0.0.1:7777���� ����
//boost::asio::io_context io_context;
//auto socket = std::make_shared<tcp::socket>(io_context);
//tcp::resolver resolver(io_context);
//auto endpoints = resolver.resolve("127.0.0.1", "7777");
//boost::asio::connect(*socket, endpoints);
//// async_write�� ���ؼ� ��Ŷ�� ����
//boost::asio::async_write(*socket, boost::asio::buffer(&packet, sizeof(packet)), [](const boost::system::error_code& ec, std::size_t) {
//	if (ec) {
//		std::lock_guard<std::mutex> lock(cout_mutex);
//		std::cerr << "Error writing to socket: " << ec.message() << std::endl;
//	}
//	});
//// io_context�� ����
//io_context.run();
//// io_context�� reset
//io_context.restart();
//// socket�� ����
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

        // socket�� pool�� ��ȯ
        socket_pool.release(socket);

    }
});
socket_pool.release(socket);
*/