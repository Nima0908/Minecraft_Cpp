#include "aes_cipher.hpp"
#include <cstring>
#include <stdexcept>

namespace mc {

AESCipher::AESCipher(const std::vector<uint8_t>& key) {
    if (key.size() != 16) {
        throw std::runtime_error("AES key must be 16 bytes (128 bits).");
    }

    encryptCtx_ = EVP_CIPHER_CTX_new();
    decryptCtx_ = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(encryptCtx_, EVP_aes_128_cfb8(), nullptr, key.data(), key.data());
    EVP_DecryptInit_ex(decryptCtx_, EVP_aes_128_cfb8(), nullptr, key.data(), key.data());
}

AESCipher::~AESCipher() {
    EVP_CIPHER_CTX_free(encryptCtx_);
    EVP_CIPHER_CTX_free(decryptCtx_);
}

std::vector<uint8_t> AESCipher::encrypt(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out(data.size());
    int outlen = 0;

    EVP_EncryptUpdate(encryptCtx_, out.data(), &outlen, data.data(), data.size());
    return out;
}

std::vector<uint8_t> AESCipher::decrypt(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out(data.size());
    int outlen = 0;

    EVP_DecryptUpdate(decryptCtx_, out.data(), &outlen, data.data(), data.size());
    return out;
}

}
