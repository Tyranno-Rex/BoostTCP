#include "utils.hpp"

extern std::mutex cout_mutex;
extern std::mutex command_mutex;

std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data) {
    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(mdctx, EVP_md5(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize digest");
    }

    if (EVP_DigestUpdate(mdctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update digest");
    }

    unsigned int length = 0;
    if (EVP_DigestFinal_ex(mdctx, checksum.data(), &length) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(mdctx);
    return checksum;
}

static int total_print = 0;

std::string printMessageWithTime(const std::string& message, bool isDebug) {
    if (isDebug) {
		std::lock_guard<std::mutex> lock(cout_mutex);
		std::cout << message << std::endl;
		total_print++;
		std::cout << "Total print: " << total_print << std::endl;
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
