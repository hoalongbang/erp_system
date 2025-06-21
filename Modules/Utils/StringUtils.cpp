// Modules/Utils/StringUtils.cpp
#include "StringUtils.h"

namespace ERP {
    namespace Utils {

        std::string StringUtils::trim(const std::string& s) {
            std::string s_copy = s;
            s_copy.erase(s_copy.begin(), std::find_if(s_copy.begin(), s_copy.end(), [](unsigned char ch) {
                return !std::isspace(ch);
                }));
            s_copy.erase(std::find_if(s_copy.rbegin(), s_copy.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
                }).base(), s_copy.end());
            return s_copy;
        }

        std::string StringUtils::toUpper(const std::string& s) {
            std::string s_copy = s;
            std::transform(s_copy.begin(), s_copy.end(), s_copy.begin(), ::toupper);
            return s_copy;
        }

        std::string StringUtils::toLower(const std::string& s) {
            std::string s_copy = s;
            std::transform(s_copy.begin(), s_copy.end(), s_copy.begin(), ::tolower);
            return s_copy;
        }

        std::vector<std::string> StringUtils::split(const std::string& s, char delimiter) {
            std::vector<std::string> tokens;
            std::string token;
            std::istringstream tokenStream(s);
            while (std::getline(tokenStream, token, delimiter)) {
                tokens.push_back(token);
            }
            return tokens;
        }

        std::string StringUtils::join(const std::vector<std::string>& elements, const std::string& delimiter) {
            if (elements.empty()) {
                return "";
            }
            std::ostringstream oss;
            for (size_t i = 0; i < elements.size(); ++i) {
                oss << elements[i];
                if (i < elements.size() - 1) {
                    oss << delimiter;
                }
            }
            return oss.str();
        }

        bool StringUtils::contains(const std::string& haystack, const std::string& needle) {
            return haystack.find(needle) != std::string::npos;
        }

        bool StringUtils::containsIgnoreCase(const std::string& haystack, const std::string& needle) {
            std::string haystackLower = toLower(haystack);
            std::string needleLower = toLower(needle);
            return haystackLower.find(needleLower) != std::string::npos;
        }

    } // namespace Utils
} // namespace ERP