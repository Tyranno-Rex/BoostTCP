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

// 클라이언트 세션을 관리하는 클래스
class Session : public std::enable_shared_from_this<Session> {
private:
    tcp::socket socket;
    Server& server;
	int SessionID;
    
    //std::array<char, 1540> current_buffer;   // 현재 읽기용 버퍼
	std::shared_ptr<std::array<char, 1540>> current_buffer;   // 현재 읽기용 버퍼
    std::array<char, 154> packet_buffer;    // 패킷 조립용 버퍼
    
    size_t packet_buffer_offset = 0;        // 패킷 버퍼의 현재 위치
    
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

    void handleReceivedData(size_t bytes_transferred); // 데이터 처리 함수 추가
	//void handleReceivedData(std::array<char, 154>& buffer, size_t bytes_transferred); // 데이터 처리 함수 추가 (이건 크기 사이즈 만큼만 가져 오는 함수)
	//void doRead();
private:
    std::vector<char> partial_packet_buffer; // 불완전 패킷 저장 버퍼 추가
    void doRead();
    bool handlePacket(size_t bytes_transferred);
};

void processPacketInWorker(int session_id, std::unique_ptr<std::vector<char>>& data, size_t size);