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
