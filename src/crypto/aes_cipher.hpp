#pragma once

#include <openssl/evp.h>
#include <vector>

namespace mc {

class AESCipher {
public:
  AESCipher(const std::vector<uint8_t> &key);
  ~AESCipher();

  std::vector<uint8_t> encrypt(const std::vector<uint8_t> &data);
  std::vector<uint8_t> decrypt(const std::vector<uint8_t> &data);

private:
  EVP_CIPHER_CTX *encryptCtx_;
  EVP_CIPHER_CTX *decryptCtx_;
};

} // namespace mc
