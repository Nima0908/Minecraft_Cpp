#include "encryption.hpp"
#include "../util/logger.hpp"
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <stdexcept>
#include <vector>
#include <string>

namespace mc::encryption {

std::vector<uint8_t> generateSharedSecret(size_t length) {
    mc::log(mc::LogLevel::DEBUG, "Generating shared secret of length: " + std::to_string(length));
    std::vector<uint8_t> secret(length);
    if (RAND_bytes(secret.data(), static_cast<int>(length)) != 1) {
        mc::log(mc::LogLevel::ERROR, "RAND_bytes failed");
        throw std::runtime_error("Failed to generate shared secret");
    }
    mc::log(mc::LogLevel::DEBUG, "Shared secret generated successfully.");
    return secret;
}

std::vector<uint8_t> rsaEncrypt(const std::vector<uint8_t>& data, const std::vector<uint8_t>& publicKey) {
    mc::log(mc::LogLevel::DEBUG, "Beginning RSA encryption. Data size: " + std::to_string(data.size()) +
                                 ", Public key size: " + std::to_string(publicKey.size()));

    BIO* bio = BIO_new_mem_buf(publicKey.data(), static_cast<int>(publicKey.size()));
    if (!bio) throw std::runtime_error("BIO_new_mem_buf failed");

    RSA* rsa = d2i_RSA_PUBKEY_bio(bio, nullptr);
    BIO_free(bio);
    if (!rsa) throw std::runtime_error("Failed to parse RSA public key");

    int keySize = RSA_size(rsa);
    std::vector<uint8_t> encrypted(keySize);

    mc::log(mc::LogLevel::DEBUG, "Performing RSA_public_encrypt...");
    int result = RSA_public_encrypt(
        static_cast<int>(data.size()),
        data.data(),
        encrypted.data(),
        rsa,
        RSA_PKCS1_PADDING
    );

    RSA_free(rsa);

    if (result == -1) {
        mc::log(mc::LogLevel::ERROR, "RSA_public_encrypt failed");
        throw std::runtime_error("RSA encryption failed");
    }

    encrypted.resize(result);
    mc::log(mc::LogLevel::DEBUG, "RSA encryption successful. Encrypted size: " + std::to_string(result));
    return encrypted;
}

std::string computeServerHash(const std::string& serverID,
                              const std::vector<uint8_t>& sharedSecret,
                              const std::vector<uint8_t>& publicKey) {
    mc::log(mc::LogLevel::DEBUG, "Computing server hash...");
    mc::log(mc::LogLevel::DEBUG, "  Server ID: " + serverID);
    mc::log(mc::LogLevel::DEBUG, "  Shared secret length: " + std::to_string(sharedSecret.size()));
    mc::log(mc::LogLevel::DEBUG, "  Public key length: " + std::to_string(publicKey.size()));

    SHA_CTX shaCtx;
    SHA1_Init(&shaCtx);
    SHA1_Update(&shaCtx, serverID.data(), serverID.size());
    SHA1_Update(&shaCtx, sharedSecret.data(), sharedSecret.size());
    SHA1_Update(&shaCtx, publicKey.data(), publicKey.size());

    uint8_t digest[SHA_DIGEST_LENGTH];
    SHA1_Final(digest, &shaCtx);

    BIGNUM* num = BN_bin2bn(digest, SHA_DIGEST_LENGTH, nullptr);
    if (!num) throw std::runtime_error("BN_bin2bn failed");

    if (digest[0] & 0x80) BN_set_negative(num, 1);

    char* hexStr = BN_bn2hex(num);
    std::string hashStr = hexStr;

    BN_free(num);
    OPENSSL_free(hexStr);

    for (char& c : hashStr) c = std::tolower(c);

    mc::log(mc::LogLevel::DEBUG, "Computed server hash: " + hashStr);
    return hashStr;
}

}
