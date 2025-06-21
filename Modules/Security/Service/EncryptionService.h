// Modules/Security/Service/EncryptionService.h
#ifndef MODULES_SECURITY_SERVICE_ENCRYPTIONSERVICE_H
#define MODULES_SECURITY_SERVICE_ENCRYPTIONSERVICE_H
#include <string>
#include <memory>   // For std::shared_ptr
#include <vector>   // For storing byte vectors
#include <stdexcept> // For exceptions
#include <cryptopp/aes.h> // Crypto++ AES
#include <cryptopp/modes.h> // Crypto++ modes (CBC)
#include <cryptopp/filters.h> // Crypto++ filters
#include <cryptopp/osrng.h> // Crypto++ AutoSeededRandomPool
#include <cryptopp/hex.h> // Crypto++ HexEncoder/Decoder
#include <cryptopp/pwdbased.h> // Crypto++ PBKDF2
#include <cryptopp/sha.h> // Crypto++ SHA1

#include "Logger.h" // Đã rút gọn include
#include "ErrorHandler.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include

namespace ERP {
namespace Security {
namespace Service {

/**
 * @brief Singleton class for handling encryption and decryption operations.
 * Uses AES-256 in CBC mode with PBKDF2 for key derivation.
 * Provides a secure way to store sensitive data in the database.
 */
class EncryptionService {
public:
    /**
     * @brief Gets the singleton instance of EncryptionService.
     * @return Reference to the EncryptionService instance.
     */
    static EncryptionService& getInstance();

    // Deleted copy constructor and assignment operator to enforce singleton
    EncryptionService(const EncryptionService&) = delete;
    EncryptionService& operator=(const EncryptionService&) = delete;

    /**
     * @brief Encrypts a plaintext string.
     * The salt and IV are generated internally and prepended to the ciphertext.
     * @param plaintext The string to encrypt.
     * @return The encrypted string (Base64 encoded, with salt and IV prepended).
     * @throws std::runtime_error if encryption fails.
     */
    std::string encrypt(const std::string& plaintext);

    /**
     * @brief Decrypts an encrypted string.
     * The salt and IV are extracted from the encrypted string.
     * @param ciphertext The encrypted string (Base64 encoded, with salt and IV prepended).
     * @return The decrypted plaintext string.
     * @throws std::runtime_error if decryption fails or input format is invalid.
     */
    std::string decrypt(const std::string& ciphertext);

private:
    EncryptionService(); // Private constructor for singleton
    ~EncryptionService() = default;

    // Fixed key/password for simplicity in demo. In real app, this would be loaded securely.
    // Ideally, the key would be rotated and managed by a secure key management system.
    const std::string FIXED_AES_KEY_STRING = "ThisIsAStrongAndSecureEncryptionKeyForERP12345"; // 32 bytes for AES-256
    CryptoPP::SecByteBlock key_; // AES Key
    
    // Parameters for PBKDF2
    const int PBKDF2_ITERATIONS = 10000;
    const size_t AES_KEY_SIZE = CryptoPP::AES::DEFAULT_KEYLENGTH; // 16 bytes for AES-128, 32 for AES-256
    const size_t AES_BLOCK_SIZE = CryptoPP::AES::BLOCKSIZE; // 16 bytes for AES

    // Helper functions
    std::string generateRandomBytes(size_t size);
    CryptoPP::SecByteBlock deriveKey(const std::string& password, const CryptoPP::SecByteBlock& salt);
    std::string bytesToBase64(const CryptoPP::SecByteBlock& bytes);
    CryptoPP::SecByteBlock base64ToBytes(const std::string& base64String);
};

} // namespace Service
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_ENCRYPTIONSERVICE_H