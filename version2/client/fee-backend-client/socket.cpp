#pragma once
#include "main.hpp"
#include "socket.hpp"
#include "globals.hpp"

bool send_buffer_success(const boost::system::error_code& ec, std::size_t) { 
    std::scoped_lock lock(cout_mutex);
    if (ec) {
        LOGE << "Error writing to socket: " << ec.message();
        total_send_fail_cnt++;
    } else {
        LOGI << "Message sent successfully";
        total_send_success_cnt++;
    }
    return true;
}

std::shared_ptr<Packet> create_packet(const std::string& message) {
    auto packet = std::make_shared<Packet>();
    packet->header.type = PacketType::ES;
    packet->tail.value = 255;

    std::string msg = std::to_string(total_send_cnt) + " / " + message;
    msg.resize(std::min(msg.size(), size_t(128)), ' ');

    std::memset(packet->payload, 0, sizeof(packet->payload));
    std::memcpy(packet->payload, msg.c_str(), std::min(msg.size(), size_t(128)));
    packet->header.size = static_cast<uint32_t>(sizeof(Packet));

    auto checksum = calculate_checksum(std::vector<char>(msg.begin(), msg.end()));
    std::memcpy(packet->header.checkSum, checksum.data(), checksum.size());

    return packet;
}

void send_buffer(std::shared_ptr<tcp::socket> socket, std::shared_ptr<Packet> packet) {
    boost::asio::async_write(
        *socket,
        boost::asio::buffer(packet.get(), sizeof(Packet)),
        [packet](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            send_buffer_success(ec, bytes_transferred);
        }
    );
}

void handle_sockets(MemoryPool<Socket>& socket_pool, int connection_cnt, const std::string& message, int thread_num) {
    try {
        auto packet = create_packet(message);

        for (int i = 0; i < connection_cnt; ++i) {
            auto socket = socket_pool.acquire();
			socket.get()->connect();
            send_buffer(socket->get_shared_socket(), packet);
			socket_pool.release(socket);
        }
    }
    catch (const std::exception& e) {
        std::scoped_lock lock(cout_mutex);
        LOGE << "Error writing to socket: " << e.what();
    }
}
