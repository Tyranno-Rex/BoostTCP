#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <zlib.h>
#include <mutex>
#include "server.hpp"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Ŭ���̾�Ʈ ������ �����ϴ� Ŭ����
class Session : public std::enable_shared_from_this<Session> {
private:
    tcp::socket socket;
    Server& server;
	int SessionID;
    
    //std::array<char, 1540> current_buffer;   // ���� �б�� ����
	std::shared_ptr<std::array<char, 1540>> current_buffer;   // ���� �б�� ����
    std::array<char, 154> packet_buffer;    // ��Ŷ ������ ����
    
    size_t packet_buffer_offset = 0;        // ��Ŷ ������ ���� ��ġ
    
    std::mutex packet_mutex;
    std::mutex read_mutex;

public:
    Session(tcp::socket socket_, Server& server_)
        : socket(std::move(socket_))
        , server(server_) {
    }

    ~Session() {
    }

    void start() {
        doRead();
    }

	void stop();
	void setSessionID(int id) { SessionID = id; }
	int getSessionID() { return SessionID; }
	//void processPacketInWorker(std::unique_ptr<std::vector<char>>& data, size_t size);

    void handleReceivedData(size_t bytes_transferred); // ������ ó�� �Լ� �߰�
	//void handleReceivedData(std::array<char, 154>& buffer, size_t bytes_transferred); // ������ ó�� �Լ� �߰� (�̰� ũ�� ������ ��ŭ�� ���� ���� �Լ�)
	//void doRead();
private:
    std::vector<char> partial_packet_buffer; // �ҿ��� ��Ŷ ���� ���� �߰�
    void doRead();
    bool handlePacket(size_t bytes_transferred);
};

void processPacketInWorker(int session_id, std::unique_ptr<std::vector<char>>& data, size_t size);