// Modules/Utils/Utils.cpp
#include "Utils.h"
#include "Logger.h" // Standard includes (for logging errors if any)

namespace ERP {
    namespace Utils {

        std::string Utils::generateUUID() {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, 15);
            static std::uniform_int_distribution<> dis2(8, 11); // For the 4th block's first digit (8, 9, a, or b)

            std::stringstream ss;
            ss << std::hex;
            for (int i = 0; i < 8; ++i) {
                ss << dis(gen);
            }
            ss << "-";
            for (int i = 0; i < 4; ++i) {
                ss << dis(gen);
            }
            ss << "-4"; // UUID version 4
            for (int i = 0; i < 3; ++i) {
                ss << dis(gen);
            }
            ss << "-";
            ss << dis2(gen);
            for (int i = 0; i < 3; ++i) {
                ss << dis(gen);
            }
            ss << "-";
            for (int i = 0; i < 12; ++i) {
                ss << dis(gen);
            }
            return ss.str();
        }

    } // namespace Utils
} // namespace ERP