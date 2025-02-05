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
//    // RAII를 위한 unique_ptr과 커스텀 deleter 사용
//    struct EVP_MD_CTX_Deleter {
//        void operator()(EVP_MD_CTX* ctx) {
//            if (ctx) EVP_MD_CTX_free(ctx);
//        }
//    };
//
//    // unique_ptr을 사용하여 자동 메모리 관리
//    std::unique_ptr<EVP_MD_CTX, EVP_MD_CTX_Deleter> mdctx(EVP_MD_CTX_new());
//    if (!mdctx) {
//        throw std::runtime_error("Failed to create EVP_MD_CTX");
//    }
//
//    // MD5 컨텍스트 초기화
//    const EVP_MD* md = EVP_md5();
//    if (!md) {
//        throw std::runtime_error("Failed to get MD5 algorithm");
//    }
//
//    if (EVP_DigestInit_ex(mdctx.get(), md, nullptr) != 1) {
//        throw std::runtime_error("Failed to initialize digest");
//    }
//
//    // 데이터 업데이트
//    if (EVP_DigestUpdate(mdctx.get(), data.data(), data.size()) != 1) {
//        throw std::runtime_error("Failed to update digest");
//    }
//
//    // 다이제스트 완료
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
//    // RAII를 위한 unique_ptr과 커스텀 deleter 사용
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
//    // MD5란: 128비트 길이의 해시값을 생성하는 해시 함수
//    // 필요한 이유: 데이터의 무결성을 보장하기 위해 사용
//    // MD5 해시값을 저장할 배열
//    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
//    // MD5 해시 계산
//    // EVP_MD_CTX_new: EVP_MD_CTX 객체 생성
//    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
//    // EVP_MD_CTX_new 실패 시 예외 처리
//    if (mdctx == nullptr) {
//        throw std::runtime_error("Failed to create EVP_MD_CTX");
//    }
//
//    // MD5 해시 초기화
//    // DigestInit_ex: 해시 함수 초기화
//    // EVP_md5: MD5 해시 함수
//    // 초기화 작업 하는 이유 : 이전에 사용된 해시 함수의 상태를 초기화하기 위해
//    if (EVP_DigestInit_ex(mdctx, EVP_md5(), nullptr) != 1) {
//        EVP_MD_CTX_free(mdctx);
//        throw std::runtime_error("Failed to initialize digest");
//    }
//
//    // MD5 해시 업데이트
//    // DigestUpdate: 데이터를 해시 함수에 업데이트
//    // data.data(): 데이터의 시작 주소
//    // data.size(): 데이터의 길이
//    if (EVP_DigestUpdate(mdctx, data.data(), data.size()) != 1) {
//        EVP_MD_CTX_free(mdctx);
//        throw std::runtime_error("Failed to update digest");
//    }
//
//    // MD5 해시 최종화
//    unsigned int length = 0;
//    // DigestFinal_ex: 해시 함수를 종료하고 결과를 저장
//    // checksum.data(): 해시 결과를 저장할 배열
//    // length: 해시 결과의 길이
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

    // 현재 시간을 구한다.
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;

    // 현재 시간을 tm 구조체로 변환
    std::time_t time_t_now = seconds.count();
    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);

    // 시간을 문자열로 변환
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%H:%M:%S");
    // 밀리초까지 추가함
    oss << "." << std::setfill('0') << std::setw(3) << milliseconds.count();

    std::string str_retrun = "[" + oss.str() + "] " + message;
    return str_retrun;
}
