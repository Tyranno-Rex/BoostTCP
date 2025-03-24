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
#include <ctime>
#include "server.hpp"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Server Ŭ���� ���� ����
class Server;


// Ŭ���̾�Ʈ ������ �����ϴ� Ŭ����
class Session : public std::enable_shared_from_this<Session> {
private:
    tcp::socket socket;
    //Server& server;
	Server* server;
	int SessionID;
	std::atomic<int> max_seq = 0;
	bool is_connected = true;
	bool is_active = false;
	std::string last_connect_time = "";
    
    std::array<char, 1540> current_buffer;   // ���� �б�� ����
    std::array<char, 154> packet_buffer;    // ��Ŷ ������ ����
    
    size_t packet_buffer_offset = 0;        // ��Ŷ ������ ���� ��ġ
    
    std::mutex packet_mutex;
    std::mutex read_mutex;

public:
    Session(boost::asio::io_context& io_context, Server& server)
        : socket(io_context), server(&server) {
    };

    ~Session() {
    }

    void start() {
        doRead();
    }

	void stop();
	void setSessionID(int id) { SessionID = id; }
	int getSessionID() { return SessionID; }

    void handleReceivedData(size_t bytes_transferred); // ������ ó�� �Լ� �߰�
    void initialize(tcp::socket socket_, Server& server_) {
		socket = std::move(socket_);
		server = &server_;
    }

	int getMaxSeq() { return max_seq; }
    
    void setMaxSeq(int seq) { max_seq = seq; }

	//bool getConnected() { return is_connected; }
	//void setConnected(bool connected) { is_connected = connected; }

	bool isActive() { return is_active; }
	void setActive(bool active) { is_active = active; }

	std::string getLastConnectTime() { return last_connect_time; }
	void setLastConnectTime(std::string time) { last_connect_time = time; }

	void doReset() {
		SessionID = 0;
		max_seq = 0;
		//is_connected = true;
		is_active = false;
		last_connect_time = "";

		memset(packet_buffer.data(), 0, packet_buffer.size());
		memset(current_buffer.data(), 0, current_buffer.size());
	}

private:
    std::vector<char> partial_packet_buffer; // �ҿ��� ��Ŷ ���� ���� �߰�
    void doRead();
    bool handlePacket(size_t bytes_transferred);
};

//void processPacketInWorker(int session_id, std::unique_ptr<std::vector<char>>& data, size_t size);