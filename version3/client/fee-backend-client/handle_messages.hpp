#ifndef WRITE_MESSAGES_HPP
#define WRITE_MESSAGES_HPP

#include <string>
#include <boost/asio.hpp>

// ����� �Է¿� ���� ���� ����/���� ������ ó���ϴ� ���� �Լ�
void write_messages(boost::asio::io_context& io_context, const std::string& host, const std::string& port);

#endif // WRITE_MESSAGES_HPP
