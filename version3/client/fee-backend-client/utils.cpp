#include "utils.hpp"

extern std::mutex cout_mutex;
extern std::mutex command_mutex;

std::wstring stringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

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
