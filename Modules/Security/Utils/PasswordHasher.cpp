// Modules/Security/Utils/PasswordHasher.cpp
#include "PasswordHasher.h"

// Includes for Crypto++ library (assuming they are set up correctly)
#include <cryptopp/sha.h>       // For SHA256
#include <cryptopp/hex.h>       // For HexEncoder/Decoder
#include <cryptopp/filters.h>   // For StringSource, StringSink

namespace ERP {
    namespace Security {
        namespace Utils {

            std::string PasswordHasher::generateSalt(size_t length) {
                std::random_device rd;
                std::mt19937 generator(rd());
                std::uniform_int_distribution<int> distribution(0, 255); // For byte values

                std::vector<unsigned char> salt(length);
                std::generate(salt.begin(), salt.end(), [&]() { return static_cast<unsigned char>(distribution(generator)); });

                std::string encodedSalt;
                CryptoPP::StringSource ss(salt.data(), salt.size(), true,
                    new CryptoPP::HexEncoder(
                        new CryptoPP::StringSink(encodedSalt)
                    ) // HexEncoder
                ); // StringSource
                return encodedSalt;
            }

            std::string PasswordHasher::hashPassword(const std::string& password, const std::string& salt) {
                // Concatenate password and salt
                std::string passwordWithSalt = password + salt;

                // Hash using SHA256
                std::string hashedPassword;
                CryptoPP::SHA256 hash;
                CryptoPP::StringSource ss(passwordWithSalt, true,
                    new CryptoPP::HashFilter(hash,
                        new CryptoPP::HexEncoder(
                            new CryptoPP::StringSink(hashedPassword)
                        ) // HexEncoder
                    ) // HashFilter
                ); // StringSource
                return hashedPassword;
            }

            bool PasswordHasher::verifyPassword(const std::string& plainPassword, const std::string& storedSalt, const std::string& storedHash) {
                // Hash the plain password with the stored salt
                std::string hashedPlainPassword = hashPassword(plainPassword, storedSalt);

                // Compare the newly hashed password with the stored hash
                return hashedPlainPassword == storedHash;
            }

        } // namespace Utils
    } // namespace Security
} // namespace ERP