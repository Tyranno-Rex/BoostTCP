#ifndef WRITE_MESSAGES_HPP
#define WRITE_MESSAGES_HPP

#include <string>
#include <boost/asio.hpp>

// 사용자 입력에 따라 소켓 생성/전송 로직을 처리하는 메인 함수
void write_messages(boost::asio::io_context& io_context, const std::string& host, const std::string& port);

#endif // WRITE_MESSAGES_HPP
