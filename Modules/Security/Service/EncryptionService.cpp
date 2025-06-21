// Modules/Security/Service/EncryptionService.cpp
#include "EncryptionService.h" // Đã rút gọn include
#include "Logger.h" // Đã rút gọn include
#include "ErrorHandler.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>
#include <cryptopp/base64.h> // For Base64Encoder/Decoder

#include <stdexcept>
#include <vector>
#include <algorithm> // For std::copy

namespace ERP {
namespace Security {
namespace Service {

EncryptionService& EncryptionService::getInstance() {
    static EncryptionService instance; // Guaranteed to be destroyed, instantiated on first use.
    return instance;
}

EncryptionService::EncryptionService() {
    // Derive AES key from fixed password and a fixed salt (for demo purposes)
    // In a real application, the key would be loaded securely, not hardcoded.
    // The fixed key string should be exactly 32 bytes for AES-256, so I will adjust for that.
    if (FIXED_AES_KEY_STRING.length() != AES_KEY_SIZE) {
        ERP::Logger::Logger::getInstance().critical("EncryptionService", "Fixed AES key string length is incorrect. Expected " + std::to_string(AES_KEY_SIZE) + " bytes.");
        throw std::runtime_error("EncryptionService: Invalid AES key length.");
    }
    key_ = CryptoPP::SecByteBlock(reinterpret_cast<const CryptoPP::byte*>(FIXED_AES_KEY_STRING.data()), AES_KEY_SIZE);
    
    ERP::Logger::Logger::getInstance().info("EncryptionService: Initialized with AES-256.");
}

std::string EncryptionService::generateRandomBytes(size_t size) {
    CryptoPP::AutoSeededRandomPool prng;
    CryptoPP::SecByteBlock bytes(size);
    prng.GenerateBlock(bytes, bytes.size());
    
    std::string encoded;
    CryptoPP::StringSource ss(bytes, bytes.size(), true,
        new CryptoPP::Base64Encoder(
            new CryptoPP::StringSink(encoded)
        ) // Base64Encoder
    ); // StringSource
    return encoded;
}

CryptoPP::SecByteBlock EncryptionService::deriveKey(const std::string& password, const CryptoPP::SecByteBlock& salt) {
    CryptoPP::SecByteBlock derivedKey(AES_KEY_SIZE);
    CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256> pbkdf2; // Using SHA256 for PBKDF2
    pbkdf2.DeriveKey(derivedKey, derivedKey.size(), (const CryptoPP::byte*)password.data(), password.size(), salt, salt.size(), PBKDF2_ITERATIONS);
    return derivedKey;
}

std::string EncryptionService::bytesToBase64(const CryptoPP::SecByteBlock& bytes) {
    std::string encoded;
    CryptoPP::StringSource ss(bytes, bytes.size(), true,
        new CryptoPP::Base64Encoder(
            new CryptoPP::StringSink(encoded),
            false // Do not append newline
        )
    );
    return encoded;
}

CryptoPP::SecByteBlock EncryptionService::base64ToBytes(const std::string& base64String) {
    std::string decoded;
    CryptoPP::StringSource ss(base64String, true,
        new CryptoPP::Base64Decoder(
            new CryptoPP::StringSink(decoded)
        )
    );
    return CryptoPP::SecByteBlock(reinterpret_cast<const CryptoPP::byte*>(decoded.data()), decoded.size());
}


std::string EncryptionService::encrypt(const std::string& plaintext) {
    try {
        CryptoPP::AutoSeededRandomPool prng;
        CryptoPP::SecByteBlock iv(AES_BLOCK_SIZE); // Initialization Vector
        prng.GenerateBlock(iv, iv.size());

        std::string ciphertext;
        CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryptor;
        encryptor.SetKeyWith      IV(key_, key_.size(), iv);

        CryptoPP::StringSource ss1(plaintext, true,
            new CryptoPP::StreamTransformationFilter(encryptor,
                new CryptoPP::StringSink(ciphertext)
            ) // StreamTransformationFilter
        ); // StringSource

        // Prepend IV to ciphertext, then Base64 encode the whole thing
        std::string ivAndCiphertext = bytesToBase64(iv) + "." + bytesToBase64(CryptoPP::SecByteBlock(reinterpret_cast<const CryptoPP::byte*>(ciphertext.data()), ciphertext.size()));
        
        ERP::Logger::Logger::getInstance().debug("EncryptionService: Data encrypted successfully.");
        return ivAndCiphertext;
    } catch (const CryptoPP::Exception& e) {
        ERP::Logger::Logger::getInstance().error("EncryptionService: Crypto++ encryption error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::EncryptionError, "EncryptionService: Crypto++ encryption failed: " + std::string(e.what()));
        throw std::runtime_error("Encryption failed.");
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("EncryptionService: Standard encryption error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::EncryptionError, "EncryptionService: Encryption failed: " + std::string(e.what()));
        throw std::runtime_error("Encryption failed.");
    }
}

std::string EncryptionService::decrypt(const std::string& ciphertext) {
    try {
        size_t dotPos = ciphertext.find('.');
        if (dotPos == std::string::npos) {
            throw std::runtime_error("Invalid encrypted string format. Missing '.' separator.");
        }

        std::string ivBase64 = ciphertext.substr(0, dotPos);
        std::string actualCiphertextBase64 = ciphertext.substr(dotPos + 1);

        CryptoPP::SecByteBlock iv = base64ToBytes(ivBase64);
        CryptoPP::SecByteBlock actualCiphertextBytes = base64ToBytes(actualCiphertextBase64);

        std::string decryptedtext;
        CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryptor;
        decryptor.SetKeyWithIV(key_, key_.size(), iv);

        CryptoPP::AuthenticatedDecryptionFilter df(
            decryptor,
            new CryptoPP::StringSink(decryptedtext)
        );

        CryptoPP::StringSource ss2(actualCiphertextBytes, actualCiphertextBytes.size(), true, &df);
        
        // AuthenticatedDecryptionFilter already throws if not authenticated, so no need for explicit check for df.Get={false}
        ERP::Logger::Logger::getInstance().debug("EncryptionService: Data decrypted successfully.");
        return decryptedtext;
    } catch (const CryptoPP::Exception& e) {
        ERP::Logger::Logger::getInstance().error("EncryptionService: Crypto++ decryption error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DecryptionError, "EncryptionService: Crypto++ decryption failed: " + std::string(e.what()));
        throw std::runtime_error("Decryption failed.");
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("EncryptionService: Standard decryption error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DecryptionError, "EncryptionService: Decryption failed: " + std::string(e.what()));
        throw std::runtime_error("Decryption failed.");
    }
}

} // namespace Service
} // namespace Security
} // namespace ERP