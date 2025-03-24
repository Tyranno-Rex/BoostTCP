#pragma once
#include <vector>
#include <optional>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <zlib.h>
#include <plog/Log.h>

class Session;

struct PacketTask {
    std::unique_ptr<std::vector<char>> data;
    Session* session;
    uint32_t session_id;
    //uint32_t seq_num;
    size_t size;

    PacketTask() = default;

    PacketTask(std::unique_ptr<std::vector<char>>&& buffer, size_t s, uint32_t session_id, Session* session)
        : data(std::move(buffer))
        , size(s)
        , session_id(session_id)
        , session(session) {
    }

    PacketTask(PacketTask&& other) noexcept
        : data(std::move(other.data)), size(other.size),
        session_id(other.session_id), session(other.session) {
    }

    PacketTask& operator=(PacketTask&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
            size = other.size;
            session_id = other.session_id;  // 추가
            session = other.session;        // 추가
        }
        return *this;
    }

    PacketTask(const PacketTask&) = delete;
    PacketTask& operator=(const PacketTask&) = delete;
};

class PacketQueue {
private:
    std::queue<PacketTask> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    bool stopping = false;
    size_t max_queue_size = 1000000;

public:
    PacketQueue() = default;

    // 복사 생성자/대입 연산은 삭제
    PacketQueue(const PacketQueue&) = delete;
    PacketQueue& operator=(const PacketQueue&) = delete;

    // move 생성자: 내부의 tasks와 상태만 이동, mutex와 condition은 새로 기본 생성됨
    PacketQueue(PacketQueue&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex);
        tasks = std::move(other.tasks);
        stopping = other.stopping;
        max_queue_size = other.max_queue_size;
        // mutex와 condition은 기본 상태로 남음.
    }

    PacketQueue& operator=(PacketQueue&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock_this(mutex);
            std::lock_guard<std::mutex> lock_other(other.mutex);
            tasks = std::move(other.tasks);
            stopping = other.stopping;
            max_queue_size = other.max_queue_size;
            // mutex와 condition은 그대로.
        }
        return *this;
    }


    bool push(PacketTask&& task) {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this] { return tasks.size() < max_queue_size || stopping; });
        if (stopping) {
            LOGE << "PUSH PacketQueue is stopping";
            return false;
        }
        tasks.push(std::move(task));
        condition.notify_one();
        return true;
    }

    bool pop(PacketTask& task) {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this] { return !tasks.empty() || stopping; });
        if (stopping && tasks.empty()) {
            LOGE << "POP PacketQueue is stopping";
            return false;
        }

        task = std::move(tasks.front());
        tasks.pop();
        condition.notify_one();
        return true;
    }

    void stop() {
        std::unique_lock<std::mutex> lock(mutex);
        stopping = true;
        condition.notify_all();
    }

    void clear() {
        std::unique_lock<std::mutex> lock(mutex);
        std::queue<PacketTask> empty;
        tasks.swap(empty);
    }
};




// PacketType: 패킷을 구분을 위한 변수
enum class PacketType : uint8_t {
    defEchoString = 100,
    JH = 101,
    YJ = 102,
    ES = 103,
};

// pack(push, 1): 
#pragma pack(push, 1)
struct PacketHeader { 		// header size = 25	
	uint32_t seqNum;			// size = 4
	PacketType type;		// size = 1   
	char checkSum[16];		// size = 16	
	uint32_t size;			// size = 4
};

struct PacketTail {			// tail size = 1
	uint8_t value;          // size = 1
};

struct Packet {				// packet size = 154
	PacketHeader header;    // size = 25
	char payload[128];      // size = 128
	PacketTail tail;        // size = 1
};
#pragma pack(pop)
