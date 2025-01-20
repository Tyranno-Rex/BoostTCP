#include "../include/server.hpp"
#include "../include/packet.hpp"
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <chrono>
#include <iomanip>

int total_connection = 0;
std::mutex cout_mutex;
std::mutex command_mutex;

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

void Server::handleAccept(std::shared_ptr<tcp::socket> socket, tcp::acceptor& acceptor) {
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(socket);
    }
    chatSession(socket);
    auto next_socket = std::make_shared<tcp::socket>(io_context);
    acceptor.async_accept(*next_socket,
        std::bind(&Server::handleAccept, this, next_socket, std::ref(acceptor)));
}

void Server::chatSession(std::shared_ptr<tcp::socket> client_socket) {
    auto temp_buffer = std::make_shared<std::array<char, 1024>>();
    auto packet_buffer = std::make_shared<PacketBuffer>();

    client_socket->async_read_some(
        boost::asio::buffer(*temp_buffer),
        std::bind(&Server::handleRead, this,
            std::placeholders::_1,
            std::placeholders::_2,
            client_socket,
            temp_buffer,
            packet_buffer));
}

void Server::handleRead(const boost::system::error_code& error,
    size_t bytes_transferred,
    std::shared_ptr<tcp::socket> client_socket,
    std::shared_ptr<std::array<char, 1024>> temp_buffer,
    std::shared_ptr<PacketBuffer> packet_buffer) {

    if (!error) {
        if (bytes_transferred > 0) {
			 packet_buffer->append(temp_buffer->data(), bytes_transferred); 
			
    //         if (!packet_buffer->hasOompleteHeader()) {
    //             if (temp_buffer->size() < sizeof(PacketHeader)) {
    //                 std::cerr << "PacketHeader size is too small" << std::endl;
    //                 return;
    //             }
    //             packet_buffer->append(temp_buffer->data(), bytes_transferred);
    //         }
	//         else {
	//			   packet_buffer->append(temp_buffer->data(), bytes_transferred);
	//         }
            

            while (auto maybe_packet = packet_buffer->extractPacket()) {
                Packet& packet = *maybe_packet;
                std::vector<char> payload_data(packet.payload,
					packet.payload + sizeof(packet.payload));

                auto calculated_checksum = calculate_checksum(payload_data);
                bool checksum_valid = std::memcmp(packet.header.checkSum,
                    calculated_checksum.data(), MD5_DIGEST_LENGTH) == 0;
                if (!checksum_valid) {
                    packet_buffer->packet_clear();
                    std::cerr << "Checksum validation failed for packet" << std::endl;
                    continue;
                }

				std::string message(packet.payload, sizeof(packet.payload));
				printMessageWithTime(message, true);
            }

			packet_buffer->clear();
        }

        client_socket->async_read_some(
            boost::asio::buffer(*temp_buffer),
            std::bind(&Server::handleRead, this,
                std::placeholders::_1,
                std::placeholders::_2,
                client_socket,
                temp_buffer,
                packet_buffer));
    }
    else {
        if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
        }
        else {
            std::cerr << "Error in chat session: " << error.message() << std::endl;
        }
    }
}


void Server::chatRun() {
    try {
        tcp::acceptor acceptor(io_context, { tcp::v4(), static_cast<boost::asio::ip::port_type>(port) });

        auto first_socket = std::make_shared<tcp::socket>(io_context);
        acceptor.async_accept(*first_socket,
            std::bind(&Server::handleAccept, this, first_socket, std::ref(acceptor)));

        io_context.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error in chatRun: " << e.what() << std::endl;
    }
}