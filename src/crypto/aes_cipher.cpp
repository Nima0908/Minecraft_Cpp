#include "aes_cipher.hpp"
#include "../util/logger.hpp"
#include <cstring>
#include <stdexcept>

namespace mc::crypto {

AESCipher::AESCipher(const std::vector<uint8_t> &key) {
  if (key.size() != 16) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "AES key must be 16 bytes (128 bits). Provided: " +
                       std::to_string(key.size()));
    throw std::runtime_error("AES key must be 16 bytes (128 bits).");
  }

  encryptCtx_ = EVP_CIPHER_CTX_new();
  decryptCtx_ = EVP_CIPHER_CTX_new();

  if (!encryptCtx_ || !decryptCtx_) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to create cipher contexts.");
    throw std::runtime_error("Failed to create cipher contexts.");
  }

  EVP_EncryptInit_ex(encryptCtx_, EVP_aes_128_cfb8(), nullptr, key.data(),
                     key.data());
  EVP_DecryptInit_ex(decryptCtx_, EVP_aes_128_cfb8(), nullptr, key.data(),
                     key.data());

  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "AES CFB8 cipher initialized successfully.");
}

std::vector<uint8_t> AESCipher::encrypt(const std::vector<uint8_t> &data) {
  mc::utils::log(mc::utils::LogLevel::DEBUG, "Encrypting data block of size: " +
                                                 std::to_string(data.size()));
  std::vector<uint8_t> out(data.size());
  int outlen = 0;

  EVP_EncryptUpdate(encryptCtx_, out.data(), &outlen, data.data(), data.size());

  return out;
}

std::vector<uint8_t> AESCipher::decrypt(const std::vector<uint8_t> &data) {
  mc::utils::log(mc::utils::LogLevel::DEBUG, "Decrypting data block of size: " +
                                                 std::to_string(data.size()));
  std::vector<uint8_t> out(data.size());
  int outlen = 0;

  EVP_DecryptUpdate(decryptCtx_, out.data(), &outlen, data.data(), data.size());

  return out;
}

AESCipher::~AESCipher() {
  EVP_CIPHER_CTX_free(encryptCtx_);
  EVP_CIPHER_CTX_free(decryptCtx_);
}

} // namespace mc::crypto
