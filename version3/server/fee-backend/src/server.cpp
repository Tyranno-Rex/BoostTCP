#include "../include/server.hpp"
#include "../include/packet.hpp"
#include "../include/session.hpp"
#include "../include/memory_pool.hpp"
#include "../include/utils.hpp"
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <chrono>
#include <iomanip>

extern std::atomic<int> Session_Count;

extern std::atomic<int> JH_recv_packet_total_cnt;
extern std::atomic<int> JY_recv_packet_success_cnt;
extern std::atomic<int> JY_recv_packet_fail_cnt;

extern std::atomic<int> YJ_recv_packet_total_cnt;
extern std::atomic<int> YJ_recv_packet_success_cnt;
extern std::atomic<int> YJ_recv_packet_fail_cnt;

extern std::atomic<int> ES_recv_packet_total_cnt;
extern std::atomic<int> ES_recv_packet_success_cnt;
extern std::atomic<int> ES_recv_packet_fail_cnt;


void processPacketInWorker(PacketTask &task) {
	const size_t PACKET_SIZE = 154;
	auto data = std::move(task.data);
	size_t size = task.size;
	int session_id = task.session_id;
	Session* session = task.session;

	size_t processed = 0;
	while (processed + PACKET_SIZE <= size) {
		try {
			std::vector<char> packet(data->begin() + processed, data->begin() + processed + PACKET_SIZE);

			// PacketHeader
			// seqNum, type, checkSum, size
			uint32_t seq = *reinterpret_cast<uint32_t*>(packet.data());
			PacketType type = static_cast<PacketType>(packet[4]);
			std::string checkSum(packet.begin() + 5, packet.begin() + 21);
			uint32_t packet_size = *reinterpret_cast<uint32_t*>(packet.data() + 21);

			// Packet Count Check
			switch (type) {
			case PacketType::defEchoString:
				break;
			case PacketType::JH:
				JH_recv_packet_total_cnt++;
				break;
			case PacketType::YJ:
				YJ_recv_packet_total_cnt++;
				break;
			case PacketType::ES:
				ES_recv_packet_total_cnt++;
				break;
			default:
				LOGE << "Unknown packet type";
				return;
			}

			// seqNum Check -> session ���� ���� ���
			if (session->getMaxSeq() != seq) {
				LOGE << "Invalid seq: " << seq << ", max_seq: " << session->getMaxSeq();
				// ���� ī��Ʈ ����
				if (type == PacketType::JH) {
					JY_recv_packet_fail_cnt++;
				}
				else if (type == PacketType::YJ) {
					YJ_recv_packet_fail_cnt++;
				}
				else if (type == PacketType::ES) {
					ES_recv_packet_fail_cnt++;
				}

				// �ش� ��Ŷ�� ���ؼ��� ó�� X
				processed += PACKET_SIZE;
				continue;
			}
			session->setMaxSeq(seq + 1);

			// PacketTail
			int tail = static_cast<int>(data->at(153));
			if (tail != -1) {
				LOGE << "Invalid tail: " << tail;
				if (type == PacketType::JH) {
					JY_recv_packet_fail_cnt++;
					return;
				}
				else if (type == PacketType::YJ) {
					YJ_recv_packet_fail_cnt++;
					return;
				}
				else if (type == PacketType::ES) {
					ES_recv_packet_fail_cnt++;
					return;
				}
			}

			std::string message(packet.begin() + 25, packet.begin() + 25 + 128);
			std::string total_send_cnt = std::to_string(JH_recv_packet_total_cnt + YJ_recv_packet_total_cnt + ES_recv_packet_total_cnt);
			//LOGD << message;

			if (type == PacketType::JH) {
				JY_recv_packet_success_cnt++;
			}
			else if (type == PacketType::YJ) {
				YJ_recv_packet_success_cnt++;
			}
			else if (type == PacketType::ES) {
				ES_recv_packet_success_cnt++;
			}
			else {
				LOGE << "Unknown packet type";
			}

			processed += PACKET_SIZE;
		}
		catch (const std::exception& e) {
			LOGE << "Error in processPacketInWorker: " << e.what();
		}
	}
}

void Server::initializeThreadPool() {
    is_running = true;
    size_t thread_count = std::thread::hardware_concurrency();
    std::cout << "Thread count: " << thread_count << std::endl;

    // worker ���� PacketQueue�� thread_count ũ��� �ʱ�ȭ (default constructor�� ȣ���)
    worker_task_queues.resize(thread_count);

    // Worker threads: �� worker�� �ڽ��� PacketQueue���� �۾��� pop�Ͽ� ó��
    for (size_t i = 0; i < thread_count; ++i) {
        worker_threads.emplace_back([this, i]() {
            while (is_running) {
                PacketTask task;
                // �� PacketQueue�� ��ü ����ȭ�� ������ ����.
                if (worker_task_queues[i].pop(task)) {
                    processPacketInWorker(task);
                }
            }
            });
    }

    // Pop thread: packet_queue���� �۾��� pop�� ��, session_id % thread_count�� ���� �ش� worker�� queue�� push
    pop_thread = std::thread([this, thread_count]() {
        while (is_running) {
            PacketTask task;
            if (packet_queue.pop(task)) {
                size_t idx = static_cast<size_t>(task.session_id) % thread_count;
                worker_task_queues[idx].push(std::move(task));
            }
            // �ʿ信 ���� ª�� ��� �ð��� �־� busy-waiting ��ȭ ����
        }
        });

	client_check_thread = std::thread([this]() {
		while (is_running) {
			std::this_thread::sleep_for(std::chrono::seconds(10));

			std::lock_guard<std::mutex> lock(clients_mutex);
			//clients.erase(std::remove_if(clients.begin(), clients.end(),
			//	[](const std::shared_ptr<Session>& session) {
			//		if (!session->isActive()) {
			//			auto now = std::chrono::system_clock::now();
			//			auto last_connect_time = session->getLastConnectTime();
			//			auto last_connect_time_t = std::chrono::system_clock::from_time_t(std::stoi(last_connect_time));
			//			auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - last_connect_time_t).count();
			//			if (diff > CLIENT_CONNECTION_MAINTENANCE_INTERVAL) {
			//				session->stop();
			//				return true;
			//			}
			//		}
			//		return false;
			//	}), clients.end());

			// �����ϴ� ���� �ƴ� ���°��� ����
			for (auto& session : clients) {
				if (!session->isActive()) {
					auto now = std::chrono::system_clock::now();
					auto last_connect_time = session->getLastConnectTime();
					auto last_connect_time_t = std::chrono::system_clock::from_time_t(std::stoi(last_connect_time));
					auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - last_connect_time_t).count();
					if (diff > CLIENT_CONNECTION_MAINTENANCE_INTERVAL) {
						session->doReset();
					}
				}
			}
		}
		});

}

void Server::chatRun() {
    try {
        initializeThreadPool();
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
		doAccept(acceptor);

        // io_context�� ���� �����忡�� ����
        std::vector<std::thread> io_threads;
        size_t thread_count = std::thread::hardware_concurrency() / 2;
        for (size_t i = 0; i < thread_count; ++i) {
            io_threads.emplace_back([this]() {
                io_context.run();
                });
        }
        // ���� �����忡���� io_context ����
        io_context.run();
        // ��� io_context �����尡 ����� ������ ���
        for (auto& thread : io_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error in chatRun: " << e.what() << std::endl;
    }
}

void Server::doAccept(tcp::acceptor& acceptor) {
    acceptor.async_accept(
		[this, &acceptor](const boost::system::error_code& error, tcp::socket socket) {
            if (!error) {
                std::shared_ptr<Session> session = this->session_pool.acquire();
				session.get()->setActive(true);
				session.get()->initialize(std::move(socket), *this);
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
					Session_Count++;
					session.get()->setSessionID(Session_Count);
					LOGI << "New client connected";
                    clients.push_back(session);
                }
                //session->start();
				session.get()->start();
            }
			doAccept(acceptor);
        });
}

void Server::removeClient(std::shared_ptr<Session> client) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(
        std::remove(clients.begin(), clients.end(), client),
        clients.end());
}

void Server::consoleStop() {
	io_context.stop();
}

void Server::chatStop() {
	for (auto& client : clients) {
		//client->stop();
		client.get()->stop();
	}
    
	io_context.stop();
}