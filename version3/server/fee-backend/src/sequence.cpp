#include <unordered_map>
#include <atomic>
#include <mutex>

class PacketChecker {
private:
    std::unordered_map<int, std::atomic<int>> last_seq;
    std::mutex mtx;  // delete_key를 위한 mutex

public:
    bool is_in_order(int key, int seq) {
        auto it = last_seq.find(key);
        if (it == last_seq.end()) {
            // 새로운 키를 추가할 때만 동기화 필요
            std::lock_guard<std::mutex> lock(mtx);
            last_seq[key] = seq;
            return true;
        }

        int expected = it->second.load();
        if (seq == expected + 1) {
            it->second.store(seq);
            return true;
        }

        return false;
    }

    void delete_key(int key) {
        std::lock_guard<std::mutex> lock(mtx);
        last_seq.erase(key);
    }
};
