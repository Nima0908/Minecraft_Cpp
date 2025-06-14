#include "encryption.hpp"
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdexcept>
#include <vector>

namespace mc::encryption {

std::vector<uint8_t> generateSharedSecret(size_t length) {
    std::vector<uint8_t> secret(length);
    if (RAND_bytes(secret.data(), static_cast<int>(length)) != 1) {
        throw std::runtime_error("Failed to generate shared secret");
    }
    return secret;
}

std::vector<uint8_t> rsaEncrypt(const std::vector<uint8_t>& data, const std::vector<uint8_t>& publicKey) {
    BIO* bio = BIO_new_mem_buf(publicKey.data(), static_cast<int>(publicKey.size()));
    if (!bio) throw std::runtime_error("BIO_new_mem_buf failed");

    RSA* rsa = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!rsa) throw std::runtime_error("Failed to parse RSA public key");

    int keySize = RSA_size(rsa);
    std::vector<uint8_t> encrypted(keySize);

    int result = RSA_public_encrypt(
        static_cast<int>(data.size()),
        data.data(),
        encrypted.data(),
        rsa,
        RSA_PKCS1_PADDING
    );

    RSA_free(rsa);

    if (result == -1) {
        throw std::runtime_error("RSA encryption failed");
    }

    encrypted.resize(result);
    return encrypted;
}

}
