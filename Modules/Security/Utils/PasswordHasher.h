// Modules/Security/Utils/PasswordHasher.h
#ifndef MODULES_SECURITY_UTILS_PASSWORDHASHER_H
#define MODULES_SECURITY_UTILS_PASSWORDHASHER_H

#include <string>
#include <vector>
#include <random>       // For std::random_device, std::mt19937
#include <algorithm>    // For std::generate

// For Crypto++ library (assuming it's installed via vcpkg or similar)
#include <cryptopp/sha.h>       // For SHA hashing
#include <cryptopp/hex.h>       // For Hex encoding
#include <cryptopp/filters.h>   // For CryptoPP::StringSource, CryptoPP::StringSink

namespace ERP {
    namespace Security {
        namespace Utils {

            /**
             * @brief The PasswordHasher class provides utility functions for hashing and verifying passwords.
             * It uses a salt to protect against rainbow table attacks.
             * Uses SHA256 for hashing.
             */
            class PasswordHasher {
            public:
                /**
                 * @brief Generates a random salt for password hashing.
                 * @param length The desired length of the salt in bytes.
                 * @return A randomly generated salt as a hex-encoded string.
                 */
                static std::string generateSalt(size_t length = 16); // Default salt length 16 bytes (32 hex chars)

                /**
                 * @brief Hashes a password using a provided salt.
                 * Uses SHA256 for hashing.
                 * @param password The plain text password.
                 * @param salt The salt to use for hashing (hex-encoded string).
                 * @return The hex-encoded hash of the password.
                 */
                static std::string hashPassword(const std::string& password, const std::string& salt);

                /**
                 * @brief Verifies a plain text password against a stored hash and salt.
                 * @param plainPassword The plain text password to verify.
                 * @param storedSalt The stored salt (hex-encoded string).
                 * @param storedHash The stored hash (hex-encoded string).
                 * @return True if the plain password matches the stored hash (after hashing with salt), false otherwise.
                 */
                static bool verifyPassword(const std::string& plainPassword, const std::string& storedSalt, const std::string& storedHash);

            private:
                // Private constructor/destructor to make it a purely static utility class
                PasswordHasher() = delete;
                ~PasswordHasher() = delete;
            };

        } // namespace Utils
    } // namespace Security
} // namespace ERP

#endif // MODULES_SECURITY_UTILS_PASSWORDHASHER_H