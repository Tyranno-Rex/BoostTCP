#pragma once

#include "main.hpp"
#include "socket.hpp"

extern std::mutex cout_mutex;
extern std::mutex command_mutex;

void handle_sockets(SocketPool& socket_pool, int connection_cnt, const std::string message, int thread_num) {
    try {
        for (int i = 0; i < connection_cnt; ++i) {
            try {
                // ��Ŷ ����
                Packet packet;
                packet.header.type = PacketType::ES;
                packet.tail.value = 255;

                // �޽����� payload�� ����
                std::string msg = std::to_string(thread_num) + ":" + std::to_string(i) + " " + message;
                msg = printMessageWithTime(msg, false);
                // �޽����� �ִ� 128�ڷ� ����
                msg.resize(std::min(msg.size(), (size_t)128), ' ');

                // memset�� ���ؼ� payload�� 0���� �ʱ�ȭ
                std::memset(packet.payload, 0, sizeof(packet.payload));
                // memcpy�� ���ؼ� payload�� �޽����� ����
                std::memcpy(packet.payload, msg.c_str(), std::min(msg.size(), (size_t)128));
                // header.size�� �޽����� ũ�⸦ ����
				packet.header.size = static_cast<uint32_t>(sizeof(packet));

                // checksum�� ����Ͽ� header.checkSum�� ����
                auto checksum = calculate_checksum(std::vector<char>(msg.begin(), msg.end()));
                std::memcpy(packet.header.checkSum, checksum.data(), MD5_DIGEST_LENGTH);

                // socket�� pool���� �����´�.
                auto socket = socket_pool.acquire();
                
                // socket�� ���� ��Ŷ�� �� ���� ����
                boost::asio::write(*socket, boost::asio::buffer(&packet, sizeof(packet)));

				//// ��Ŷ�� ����� ���̷ε�� ������ ����
				//size_t total_size = sizeof(packet);
				//size_t header_size = sizeof(packet.header);
                // ù ��° �κ� ����
				//boost::asio::write(*socket, boost::asio::buffer(&packet, header_size));
				//// 4���� ������ ���� (1/4)
				//boost::asio::write(*socket, boost::asio::buffer(packet.payload, sizeof(packet.payload) / 4));
				//// 4���� ������ ���� (2/4)
				//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 4, sizeof(packet.payload) / 4));
				//// 4���� ������ ���� (3/4)
				//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 2, sizeof(packet.payload) / 4));
				//// 4���� ������ ���� (4/4)
				//boost::asio::write(*socket, boost::asio::buffer(packet.payload + sizeof(packet.payload) / 4 * 3, sizeof(packet.payload) / 4 + 1));

                // socket�� ��ȯ�Ѵ�.
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

                // ��Ŷ�� �� ���� ������ ����
                auto socket = socket_pool.acquire();



				//��Ŷ��ü�� ������ �� �ƴ϶�, ��Ŷ�� ������ü�� ������ ������� ����
				BYTE* buffer = reinterpret_cast<BYTE*>(&packet);
				boost::asio::write(*socket, boost::asio::buffer(buffer, sizeof(buffer)));

    //            // ù ��° �κ� ����
				//boost::asio::write(*socket, boost::asio::buffer(&packet, sizeof(packet) / 2));

    //            // �� ��° �κ� ����
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
