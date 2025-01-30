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
#include "packet.hpp"

// Session 전방 선언
class Session;

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#define GET (http::verb::get)
#define POST (http::verb::post)
#define PUT (http::verb::put)
#define PATCH (http::verb::patch)
#define DELETE (http::verb::delete_)

#define expected_hcv 0x02
#define expected_tcv 0x03
#define expected_checksum 0x04
#define expected_size 0x08



class Server {
private:
    boost::asio::io_context& io_context;
    unsigned short port;
    std::vector<std::shared_ptr<Session>> clients;
    std::mutex clients_mutex;

public:
    Server(boost::asio::io_context& io_context, unsigned short port)
        : io_context(io_context), port(port) {
    }

    void chatRun();
    void doAccept(tcp::acceptor& acceptor);
    void removeClient(std::shared_ptr<Session> client);
	void consoleStop();
	void chatStop();
};