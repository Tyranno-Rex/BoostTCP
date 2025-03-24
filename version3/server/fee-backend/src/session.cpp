#include "../include/session.hpp"
#include "../include/utils.hpp"
#include "../include/memory_pool.hpp"
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>

extern MemoryPool<char[1540]> g_memory_pool;
extern PacketChecker g_packet_checker;

extern std::atomic<int>
 JH_recv_packet_total_cnt;
extern std::atomic<int>
 JH_recv_packet_success_cnt;
extern std::atomic<int>
 JH_recv_packet_fail_cnt;

extern std::atomic<int>
 YJ_recv_packet_total_cnt;
extern std::atomic<int>
 YJ_recv_packet_success_cnt;
extern std::atomic<int>
 YJ_recv_packet_fail_cnt;

extern std::atomic<int>
 ES_recv_packet_total_cnt;
extern std::atomic<int>
 ES_recv_packet_success_cnt;
extern std::atomic<int>
 ES_recv_packet_fail_cnt;

const size_t PACKET_SIZE = 154; // ÆĞÅ¶ Å©±â

void Session::doRead() {

    // shared_from_this()¸¦ »ç¿ëÇÏ¿© ÀÚ±â ÀÚ½ÅÀ» ÂüÁ¶ÇÏ°í ÀÖ´Â shared_ptrÀ» »ı¼º
    auto self = shared_from_this();
    // ¸Ş¸ğ¸® Ç®¿¡¼­ ¹öÆÛ¸¦ ÇÒ´ç¹Ş¾Æ¼­ ÀĞ±â ÀÛ¾÷À» ¼öÇà
    current_buffer = g_memory_pool.acquire();
    // ºñµ¿±â ÀĞ±â ÀÛ¾÷À» ¼öÇà
    socket.async_read_some(
        boost::asio::buffer(current_buffer),
        // ºñµ¿±â ÀÛ¾÷ ¿Ï·á ½Ã È£ÃâµÇ´Â Äİ¹é ÇÔ¼ö
        [this, self](const boost::system::error_code& error, size_t bytes_transferred) {
            if (!error) {
                // ÀĞÀº µ¥ÀÌÅÍ¸¦ Ã³¸®
                handleReceivedData(bytes_transferred);
                // ´ÙÀ½ ÀĞ±â ÀÛ¾÷À» ¼öÇà
                g_memory_pool.release(current_buffer);
                doRead();
            }
            else {
                //LOGI << "Client disconnected";

                g_memory_pool.release(current_buffer);
                stop();
				g_packet_checker.delete_key(SessionID);
				//server.addDeleteSessionID(SessionID);
                server.removeClient(self);
            }
        });
}

// µ¥ÀÌÅÍ Ã³¸® ÇÔ¼ö Ãß°¡
void Session::handleReceivedData(size_t bytes_transferred) {
    std::lock_guard<std::mutex> lock(read_mutex);

    // »õ·Î ¹ŞÀº, µ¥ÀÌÅÍ¸¦ ÀÓ½Ã ¹öÆÛ¿¡ º¹»ç
    std::vector<char> temp_buffer(current_buffer.begin(),
        current_buffer.begin() + bytes_transferred);

    // ÀÌÀü¿¡ ÀúÀåµÈ ºÒ¿ÏÀü ÆĞÅ¶ÀÌ ÀÖÀ¸¸é ÇöÀç ¹ŞÀº µ¥ÀÌÅÍ¿Í ÇÕÄ§
    if (!partial_packet_buffer.empty()) {
        temp_buffer.insert(temp_buffer.begin(),
            partial_packet_buffer.begin(),
            partial_packet_buffer.end());
        partial_packet_buffer.clear();
    }

    // ¿ÏÀüÇÑ ÆĞÅ¶ ´ÜÀ§·Î Ã³¸®
    size_t processed = 0;

    while (processed + PACKET_SIZE <= temp_buffer.size()) {
        // º¤ÅÍ¸¦ ÀÌµ¿ÇÏ´Â ´ë½Å º¹»çÇÏ°Å³ª ÂüÁ¶¸¦ »ç¿ë
        auto packet = std::make_unique<std::vector<char>>(
            temp_buffer.begin() + processed,
            temp_buffer.begin() + processed + PACKET_SIZE
        );
        // session_id¿Í sequence°¡ ¾ø´Â ¹öÀü
		//PacketTask task(std::move(packet), PACKET_SIZE);
        //server.getPacketQueue().push(std::move(task));

		// session_id¿Í sequence°¡ ÀÖ´Â ¹öÀü
        uint32_t received_seqnum = 0;
        std::memcpy(&received_seqnum, packet.get()->data(), sizeof(uint32_t));
		PacketTask task(std::move(packet), PACKET_SIZE, SessionID, received_seqnum);
		server.getPacketQueue().push(std::move(task));
        processed += PACKET_SIZE;
    }

    // ³²Àº µ¥ÀÌÅÍ°¡ ÀÖÀ¸¸é ºÒ¿ÏÀü ÆĞÅ¶À¸·Î ÀúÀå
    if (processed < temp_buffer.size()) {
        partial_packet_buffer.assign(temp_buffer.begin() + processed,
            temp_buffer.end());
    }
}


void Session::stop() {
	socket.close();
}

void processPacketInWorker(int session_id, std::unique_ptr<std::vector<char>>& data, size_t size) {
    const size_t PACKET_SIZE = 154;

    size_t processed = 0;
	uint32_t sequence = 0;
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
            
            if (g_packet_checker.is_in_order(session_id, seq)) {
                // ¿¡·¯ Ä«¿îÆ® ÁøÇà
                if (type == PacketType::JH) {
					JH_recv_packet_fail_cnt++;
				}
				else if (type == PacketType::YJ) {
					YJ_recv_packet_fail_cnt++;
				}
				else if (type == PacketType::ES) {
					ES_recv_packet_fail_cnt++;
				}

                // ÇØ´ç ÆĞÅ¶¿¡ ´ëÇØ¼­´Â Ã³¸® X
                processed += PACKET_SIZE;
				continue;
            }


		    // PacketTail
			int tail = static_cast<int>(data->at(153));
		    if (tail != -1) {
				LOGE << "Invalid tail: " << tail;
			    if (type == PacketType::JH) {
				    JH_recv_packet_fail_cnt++;
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
<<<<<<< HEAD
            //LOGD << message;
=======
		    std::string total_send_cnt = std::to_string(JH_recv_packet_total_cnt + YJ_recv_packet_total_cnt + ES_recv_packet_total_cnt);
>>>>>>> parent of cfe8a2e (í´ë¼ ë©”ëª¨ë¦¬ ë¦­ ë° ì„œë²„ sequence ìˆœì„œ ì²˜ë¦¬ ì™„ë£Œ -> í´ë¼ì´ì–¸íŠ¸ ì„¸ì…˜ ë³„ ì²˜ë¦¬ë˜ëŠ” ìŠ¤ë ˆë“œë¥¼ ì§€ì •í•¨ìœ¼ë¡œì¨ ìˆœì„œë¥¼ ë³´ì¥í•¨.)

		    if (type == PacketType::JH) {
			    JH_recv_packet_success_cnt++;
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