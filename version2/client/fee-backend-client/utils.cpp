#include "utils.hpp"


extern std::mutex cout_mutex;
extern std::mutex command_mutex;

std::array<unsigned char, CryptoPP::MD5::DIGESTSIZE> calculate_checksum(const std::vector<char>& data) {
    std::array<unsigned char, CryptoPP::MD5::DIGESTSIZE> checksum;

    try {
        CryptoPP::MD5 hash;
        hash.CalculateDigest(checksum.data(), reinterpret_cast<const unsigned char*>(data.data()), data.size());
    }
    catch (const CryptoPP::Exception& e) {
        throw std::runtime_error(std::string("MD5 calculation failed: ") + e.what());
    }

    return checksum;
}


//std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data) {
//    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum{};
//
//    // RAII�� ���� unique_ptr�� Ŀ���� deleter ���
//    struct EVP_MD_CTX_Deleter {
//        void operator()(EVP_MD_CTX* ctx) {
//            if (ctx) EVP_MD_CTX_free(ctx);
//        }
//    };
//
//    // unique_ptr�� ����Ͽ� �ڵ� �޸� ����
//    std::unique_ptr<EVP_MD_CTX, EVP_MD_CTX_Deleter> mdctx(EVP_MD_CTX_new());
//    if (!mdctx) {
//        throw std::runtime_error("Failed to create EVP_MD_CTX");
//    }
//
//    // MD5 ���ؽ�Ʈ �ʱ�ȭ
//    const EVP_MD* md = EVP_md5();
//    if (!md) {
//        throw std::runtime_error("Failed to get MD5 algorithm");
//    }
//
//    if (EVP_DigestInit_ex(mdctx.get(), md, nullptr) != 1) {
//        throw std::runtime_error("Failed to initialize digest");
//    }
//
//    // ������ ������Ʈ
//    if (EVP_DigestUpdate(mdctx.get(), data.data(), data.size()) != 1) {
//        throw std::runtime_error("Failed to update digest");
//    }
//
//    // ��������Ʈ �Ϸ�
//    unsigned int length = 0;
//    if (EVP_DigestFinal_ex(mdctx.get(), checksum.data(), &length) != 1) {
//        throw std::runtime_error("Failed to finalize digest");
//    }
//
//    if (length != MD5_DIGEST_LENGTH) {
//        throw std::runtime_error("Unexpected digest length");
//    }
//
//    return checksum;
//}

//std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data) {
//    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
//
//    // RAII�� ���� unique_ptr�� Ŀ���� deleter ���
//    auto mdctx_deleter = [](EVP_MD_CTX* ctx) { if (ctx) EVP_MD_CTX_free(ctx); };
//    std::unique_ptr<EVP_MD_CTX, decltype(mdctx_deleter)> mdctx(EVP_MD_CTX_new(), mdctx_deleter);
//
//    if (!mdctx) {
//        throw std::runtime_error("Failed to create EVP_MD_CTX");
//    }
//
//    if (EVP_DigestInit_ex(mdctx.get(), EVP_md5(), nullptr) != 1) {
//        throw std::runtime_error("Failed to initialize digest");
//    }
//
//    if (EVP_DigestUpdate(mdctx.get(), data.data(), data.size()) != 1) {
//        throw std::runtime_error("Failed to update digest");
//    }
//
//    unsigned int length = 0;
//    if (EVP_DigestFinal_ex(mdctx.get(), checksum.data(), &length) != 1) {
//        throw std::runtime_error("Failed to finalize digest");
//    }
//
//    return checksum;
//}

//std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data) {
//    // MD5��: 128��Ʈ ������ �ؽð��� �����ϴ� �ؽ� �Լ�
//    // �ʿ��� ����: �������� ���Ἲ�� �����ϱ� ���� ���
//    // MD5 �ؽð��� ������ �迭
//    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
//    // MD5 �ؽ� ���
//    // EVP_MD_CTX_new: EVP_MD_CTX ��ü ����
//    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
//    // EVP_MD_CTX_new ���� �� ���� ó��
//    if (mdctx == nullptr) {
//        throw std::runtime_error("Failed to create EVP_MD_CTX");
//    }
//
//    // MD5 �ؽ� �ʱ�ȭ
//    // DigestInit_ex: �ؽ� �Լ� �ʱ�ȭ
//    // EVP_md5: MD5 �ؽ� �Լ�
//    // �ʱ�ȭ �۾� �ϴ� ���� : ������ ���� �ؽ� �Լ��� ���¸� �ʱ�ȭ�ϱ� ����
//    if (EVP_DigestInit_ex(mdctx, EVP_md5(), nullptr) != 1) {
//        EVP_MD_CTX_free(mdctx);
//        throw std::runtime_error("Failed to initialize digest");
//    }
//
//    // MD5 �ؽ� ������Ʈ
//    // DigestUpdate: �����͸� �ؽ� �Լ��� ������Ʈ
//    // data.data(): �������� ���� �ּ�
//    // data.size(): �������� ����
//    if (EVP_DigestUpdate(mdctx, data.data(), data.size()) != 1) {
//        EVP_MD_CTX_free(mdctx);
//        throw std::runtime_error("Failed to update digest");
//    }
//
//    // MD5 �ؽ� ����ȭ
//    unsigned int length = 0;
//    // DigestFinal_ex: �ؽ� �Լ��� �����ϰ� ����� ����
//    // checksum.data(): �ؽ� ����� ������ �迭
//    // length: �ؽ� ����� ����
//    if (EVP_DigestFinal_ex(mdctx, checksum.data(), &length) != 1) {
//        EVP_MD_CTX_free(mdctx);
//        throw std::runtime_error("Failed to finalize digest");
//    }
//
//    EVP_MD_CTX_free(mdctx);
//    return checksum;
//}


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
