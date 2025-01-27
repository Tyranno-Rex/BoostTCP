#include "../include/server.hpp"
#include "../include/packet.hpp"
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <chrono>
#include <iomanip>

int total_connection = 0;
std::mutex cout_mutex;
std::mutex command_mutex;

MemoryPool g_memory_pool(1000);

std::string printMessageWithTime(const std::string& message, bool isDebug) {
    if (isDebug) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << message << std::endl;
        total_connection++;
		if (total_connection % 1000 == 0) {
			std::cout << "Total connection: " << total_connection << std::endl;
		}
        return message;

    }

    // ���� �ð��� ���Ѵ�.
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;

    // ���� �ð��� tm ����ü�� ��ȯ
    std::time_t time_t_now = seconds.count();
    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);

    // �ð��� ���ڿ��� ��ȯ
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%H:%M:%S");
    // �и��ʱ��� �߰���
    oss << "." << std::setfill('0') << std::setw(3) << milliseconds.count();

    std::string str_retrun = "[" + oss.str() + "] " + message;
    return str_retrun;
}

std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data) {
    // MD5��: 128��Ʈ ������ �ؽð��� �����ϴ� �ؽ� �Լ�
    // �ʿ��� ����: �������� ���Ἲ�� �����ϱ� ���� ���
    // MD5 �ؽð��� ������ �迭
    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
    // MD5 �ؽ� ���
    // EVP_MD_CTX_new: EVP_MD_CTX ��ü ����
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    // EVP_MD_CTX_new ���� �� ���� ó��
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    // MD5 �ؽ� �ʱ�ȭ
    // DigestInit_ex: �ؽ� �Լ� �ʱ�ȭ
    // EVP_md5: MD5 �ؽ� �Լ�
    // �ʱ�ȭ �۾� �ϴ� ���� : ������ ���� �ؽ� �Լ��� ���¸� �ʱ�ȭ�ϱ� ����
    if (EVP_DigestInit_ex(mdctx, EVP_md5(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize digest");
    }

    // MD5 �ؽ� ������Ʈ
    // DigestUpdate: �����͸� �ؽ� �Լ��� ������Ʈ
    // data.data(): �������� ���� �ּ�
    // data.size(): �������� ����
    if (EVP_DigestUpdate(mdctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update digest");
    }

    // MD5 �ؽ� ����ȭ
    unsigned int length = 0;
    // DigestFinal_ex: �ؽ� �Լ��� �����ϰ� ����� ����
    // checksum.data(): �ؽ� ����� ������ �迭
    // length: �ؽ� ����� ����
    if (EVP_DigestFinal_ex(mdctx, checksum.data(), &length) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(mdctx);
    return checksum;
}

void ClientSession::doRead() {
    auto self = shared_from_this();
	current_buffer = g_memory_pool.acquire();

    socket.async_read_some(
        boost::asio::buffer(current_buffer),
        [this, self](const boost::system::error_code& error, size_t bytes_transferred) {
            if (!error) {
                if (handlePacket(bytes_transferred)) {
                    g_memory_pool.release(current_buffer);
                    doRead();
                }
            }
            else {
                std::cerr << "Read error: " << error.message() << std::endl;
                g_memory_pool.release(current_buffer);
            }
        });
}

bool ClientSession::handlePacket(size_t bytes_transferred) {
    // ���� �����͸� ��Ŷ ���ۿ� �߰�
    if (packet_buffer_offset + bytes_transferred > packet_buffer.size()) {
        std::cerr << "Buffer overflow" << std::endl;
        packet_buffer_offset = 0;
        return false;
    }

    std::memcpy(packet_buffer.data() + packet_buffer_offset,
        current_buffer.data(),
        bytes_transferred);
    packet_buffer_offset += bytes_transferred;

    // ������ ��Ŷ�� ���ŵǾ����� Ȯ��
    while (packet_buffer_offset >= sizeof(Packet)) {
        Packet* packet = reinterpret_cast<Packet*>(packet_buffer.data());

        // üũ�� ����
        std::vector<char> payload_data(packet->payload,
            packet->payload + sizeof(packet->payload));
        auto calculated_checksum = calculate_checksum(payload_data);

        if (std::memcmp(packet->header.checkSum,
            calculated_checksum.data(),
            MD5_DIGEST_LENGTH) != 0) {
            std::cerr << "Checksum validation failed" << std::endl;
            packet_buffer_offset = 0;
            return false;
        }

        // tail �� ����
        if (packet->tail.value != 255) {
            std::cerr << "Invalid tail value" << std::endl;
            packet_buffer_offset = 0;
            return false;
        }

        // �޽��� ó��
        std::string message(packet->payload, sizeof(packet->payload));
        printMessageWithTime(message, true);

        // ó���� ��Ŷ ����
        if (packet_buffer_offset > sizeof(Packet)) {
            std::memmove(packet_buffer.data(),
                packet_buffer.data() + sizeof(Packet),
                packet_buffer_offset - sizeof(Packet));
            packet_buffer_offset -= sizeof(Packet);
        }
        else {
            packet_buffer_offset = 0;
        }
    }
    return true;
}

void Server::chatRun() {
    try {
        tcp::acceptor acceptor(io_context,
            tcp::endpoint(tcp::v4(), port));
        doAccept(acceptor);
        io_context.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error in chatRun: " << e.what() << std::endl;
    }
}

void Server::doAccept(tcp::acceptor& acceptor) {
    acceptor.async_accept(
        [this, &acceptor](const boost::system::error_code& error, tcp::socket socket) {
            if (!error) {
				auto session = std::make_shared<ClientSession>(std::move(socket), *this);
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
					std::cout << "New client connected" << std::endl;
                    clients.push_back(session);
                }
                session->start();
            }

            doAccept(acceptor); // ���� ���� ���
        });
}

void Server::removeClient(std::shared_ptr<ClientSession> client) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(
        std::remove(clients.begin(), clients.end(), client),
        clients.end());
}