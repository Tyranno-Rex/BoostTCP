#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <array>
#include <zlib.h> 
#include <openssl/md5.h>
#include <openssl/evp.h>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

const char expected_hcv = 0x02;
const char expected_tcv = 0x03; 


struct PacketHeader {
    short checkValue;
    int PacketType;
	int PacketSize;
};

struct Packet {
	PacketHeader pheader;
	char payload[128];
};


void send_http_request(boost::asio::io_context& io_context, tcp::resolver::results_type& http_endpoints, const std::string& target, http::verb method, const std::string& body = "") {
    try {
		// HTTP 요청 보내기
        beast::tcp_stream stream(io_context);
        stream.connect(http_endpoints);

		// 요청 작성
        http::request<http::string_body> req{ method, target, 11 };
        req.set(http::field::host, "localhost");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        std::cout << body << std::endl;

		// 요청 보내기
        http::write(stream, req);

        // 응답 읽기
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

		// 응답 출력
        std::cout << res << std::endl;

        // 연결 닫기
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ ec };
    }
    catch (std::exception& e) {
        std::cerr << "Exception in HTTP request: " << e.what() << std::endl;
    }
}

std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data) {
	// MD5란: 128비트 길이의 해시값을 생성하는 해시 함수
	// 필요한 이유: 데이터의 무결성을 보장하기 위해 사용
	// MD5 해시값을 저장할 배열
    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
	// MD5 해시 계산
	// EVP_MD_CTX_new: EVP_MD_CTX 객체 생성
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
	// EVP_MD_CTX_new 실패 시 예외 처리
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

	// MD5 해시 초기화
	// DigestInit_ex: 해시 함수 초기화
	// EVP_md5: MD5 해시 함수
	// 초기화 작업 하는 이유 : 이전에 사용된 해시 함수의 상태를 초기화하기 위해
    if (EVP_DigestInit_ex(mdctx, EVP_md5(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize digest");
    }

	// MD5 해시 업데이트
	// DigestUpdate: 데이터를 해시 함수에 업데이트
	// data.data(): 데이터의 시작 주소
	// data.size(): 데이터의 길이
    if (EVP_DigestUpdate(mdctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update digest");
    }
    
	// MD5 해시 최종화
    unsigned int length = 0;
	// DigestFinal_ex: 해시 함수를 종료하고 결과를 저장
	// checksum.data(): 해시 결과를 저장할 배열
	// length: 해시 결과의 길이
    if (EVP_DigestFinal_ex(mdctx, checksum.data(), &length) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(mdctx);
    return checksum;
}

void read_messages(tcp::socket& socket) {
    try {
        while (true) {
			// 수신할 데이터 크기를 먼저 읽음
			// hcv: Header Check Value
            std::array<char, 1> hcv;
			// checksum: MD5 해시값
            std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
			// size: 데이터 크기
            std::array<char, 4> size;
			// tcv: Tail Check Value
            std::array<char, 1> tcv;

			// 데이터 수신
            boost::asio::read(socket, boost::asio::buffer(hcv));
            boost::asio::read(socket, boost::asio::buffer(checksum));
            boost::asio::read(socket, boost::asio::buffer(size));

			// 데이터 크기를 읽어서 payload 크기를 알아냄
            uint32_t payload_size = *reinterpret_cast<uint32_t*>(size.data());
            std::vector<char> payload(payload_size);

			// 데이터 수신
            boost::asio::read(socket, boost::asio::buffer(payload));
            boost::asio::read(socket, boost::asio::buffer(tcv));

            // 수신된 CheckSum 출력 (디버깅용)
   //         std::cout << "Received checksum: ";
   //         for (unsigned char byte : checksum) {
   //             std::cout << std::hex << static_cast<int>(byte);
   //         }
			//// 수신된 데이터 크기 출력 (디버깅용)
   //         std::cout << std::dec << std::endl;

            std::string message(payload.begin(), payload.end());
            std::cout << message << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in read thread: " << e.what() << std::endl;
    }
}

// 아래 달게 되는 주석들은 내용을 처음 보는 사람에게 설명하기 위한 주석입니다.
//void write_messages(tcp::socket& chat_socket, tcp::resolver::results_type& http_endpoints, boost::asio::io_context& io_context) {
//    try {
//        while (true) {
//			// 메시지 입력
//            std::string message;
//            std::getline(std::cin, message);
//
//			// 메시지를 payload로 변환
//            std::vector<char> payload(message.begin(), message.end());
//            uint32_t payload_size = payload.size();
//            auto checksum = calculate_checksum(payload);
//
//			// hcv: Header Check Value
//			// checksum: MD5 해시값
//			// size: 데이터 크기
//			// tcv: Tail Check Value
//            std::array<char, 1> hcv = { expected_hcv };
//            std::array<char, 4> size = *reinterpret_cast<std::array<char, 4>*>(&payload_size);
//            std::array<char, 1> tcv = { expected_tcv };
//
//			// 명령어인지 채팅 메시지인지 판단
//			// /debug의 경우 스페이스를 띄우고 작성된 메시지를 while문으로 계속해서 서버에 전송
//            if (message.rfind("/debug", 0) == 0) {
//                boost::asio::thread_pool pool(4); // Create a thread pool with 4 threads
//
//                while (true) {
//                    std::string debug_message = message.substr(7);
//                    std::vector<char> debug_payload(debug_message.begin(), debug_message.end());
//                    uint32_t debug_payload_size = debug_payload.size();
//                    auto debug_checksum = calculate_checksum(debug_payload);
//
//                    std::array<char, 1> debug_hcv = { expected_hcv };
//                    std::array<char, 4> debug_size = *reinterpret_cast<std::array<char, 4>*>(&debug_payload_size);
//                    std::array<char, 1> debug_tcv = { expected_tcv };
//
//                    boost::asio::post(pool, [&chat_socket, debug_hcv, debug_checksum, debug_size, debug_payload, debug_tcv]() {
//                        boost::asio::write(chat_socket, boost::asio::buffer(debug_hcv));
//                        boost::asio::write(chat_socket, boost::asio::buffer(debug_checksum.data(), debug_checksum.size()));
//                        boost::asio::write(chat_socket, boost::asio::buffer(debug_size));
//                        boost::asio::write(chat_socket, boost::asio::buffer(debug_payload));
//                        boost::asio::write(chat_socket, boost::asio::buffer(debug_tcv));
//                        });
//                }
//
//                pool.join(); // Wait for all threads to finish
//            }
//            if (message.rfind("/", 0) == 0) { // 명령어는 '/'로 시작
//                //send_http_request(io_context, http_endpoints, "/", http::verb::post, message);
//				if (message == "/room" && message.size() == 5) {
//                    send_http_request(io_context, http_endpoints, "/api/v1/room", http::verb::get);
//                }
//				if (message.rfind("/room/create", 0) == 0) {
//					// create뒤에 스페이스를 띄우고 방 이름을 입력하면 방을 생성할 수 있음
//                    // space를 기준으로 문자열 파싱
//                    //std::string chatRoomId = ctx.formParam("id");
//                    //std::string UserId = ctx.formParam("UserId");
//					std::string id = message.substr(13);
//					// 현재 클라이언트의 IP주소를 UserId로 사용
//					std::string UserId = chat_socket.local_endpoint().address().to_string();
//					std::string body = "{\"id\":\"" + id + "\",\"user_id\":\"" + UserId + "\"}";
//					// std::cout << "body:" << body << std::endl;
//					send_http_request(io_context, http_endpoints, "/api/v1/room/create", http::verb::post, body);
//                }
//                else if (message.rfind("/delete", 0) == 0) {
//                    send_http_request(io_context, http_endpoints, message, http::verb::delete_);
//                }
//                else {
//                    send_http_request(io_context, http_endpoints, "/", http::verb::post, message);
//                }
//            }
//            else { // 채팅 메시지
//                boost::asio::write(chat_socket, boost::asio::buffer(hcv));
//                boost::asio::write(chat_socket, boost::asio::buffer(checksum.data(), checksum.size()));
//                boost::asio::write(chat_socket, boost::asio::buffer(size));
//                boost::asio::write(chat_socket, boost::asio::buffer(payload));
//                boost::asio::write(chat_socket, boost::asio::buffer(tcv));
//            }
//        }
//    }
//    catch (std::exception& e) {
//        std::cerr << "Exception in write thread: " << e.what() << std::endl;
//    }
//}


void write_messages(tcp::socket& chat_socket, tcp::resolver::results_type& http_endpoints, boost::asio::io_context& io_context) {
    try {
        while (true) {
            std::string message;
            std::getline(std::cin, message);

            if (message.rfind("/debug", 0) == 0) {
                while (true) {
                    const int NUM_THREADS = 10;  // 원하는 스레드 수 설정
                    boost::asio::thread_pool pool(NUM_THREADS);
                    std::string debug_message = message.substr(7);

                    // 각 스레드별로 한 번씩만 메시지 전송
                    for (int i = 0; i < NUM_THREADS; ++i) {
					    // 각 메시지에 자신의 스레드 번호를 추가
                        boost::asio::post(pool, [&chat_socket, debug_message, i]() {
                            try {
                                std::vector<char> debug_payload(debug_message.begin(), debug_message.end());
                                uint32_t debug_payload_size = debug_payload.size();
                                auto debug_checksum = calculate_checksum(debug_payload);

                                std::array<char, 1> debug_hcv = { expected_hcv };
                                std::array<char, 4> debug_size = *reinterpret_cast<std::array<char, 4>*>(&debug_payload_size);
                                std::array<char, 1> debug_tcv = { expected_tcv };

                                // 모든 write 작업을 하나의 연속된 작업으로 처리
                                std::vector<boost::asio::const_buffer> buffers = {
                                    boost::asio::buffer(debug_hcv),
                                    boost::asio::buffer(debug_checksum.data(), debug_checksum.size()),
                                    boost::asio::buffer(debug_size),
                                    boost::asio::buffer(debug_payload),
                                    boost::asio::buffer(debug_tcv)
                                };
                                boost::asio::write(chat_socket, buffers);
                            }
                            catch (const std::exception& e) {
                                std::cerr << "Error in debug thread " << i << ": " << e.what() << std::endl;
								exit(1);
                            }
                            });
                    }

                    pool.wait();  // 모든 작업이 완료될 때까지 대기
                    continue;
                }
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: client <host> <chat_port> <http_port>\n";
        return 1;
    }

    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type chat_endpoints = resolver.resolve(argv[1], argv[2]);
        tcp::resolver::results_type http_endpoints = resolver.resolve(argv[1], argv[3]);

        tcp::socket chat_socket(io_context);

        boost::asio::connect(chat_socket, chat_endpoints);

		// 채팅 서버와 HTTP 서버에 대한 연결을 만들고, 채팅 서버에 대한 소켓을 만듦
        std::thread read_thread(read_messages, std::ref(chat_socket));
        std::thread write_thread(write_messages, std::ref(chat_socket), std::ref(http_endpoints), std::ref(io_context));
        
		// 쓰레드가 종료될 때까지 대기
        read_thread.join();
        write_thread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}