// Modules/Utils/StringUtils.h
#ifndef MODULES_UTILS_STRINGUTILS_H
#define MODULES_UTILS_STRINGUTILS_H

#include <string>   // For std::string
#include <vector>   // For std::vector
#include <sstream>  // For std::stringstream
#include <algorithm> // For std::transform, std::remove_if
#include <cctype>   // For ::isspace, ::tolower

namespace ERP {
    namespace Utils {

        /**
         * @brief The StringUtils class provides utility functions for string manipulation.
         */
        class StringUtils {
        public:
            /**
             * @brief Trims leading and trailing whitespace from a string.
             * @param s The string to trim.
             * @return The trimmed string.
             */
            static std::string trim(const std::string& s);

            /**
             * @brief Converts a string to uppercase.
             * @param s The string to convert.
             * @return The uppercase string.
             */
            static std::string toUpper(const std::string& s);

            /**
             * @brief Converts a string to lowercase.
             * @param s The string to convert.
             * @return The lowercase string.
             */
            static std::string toLower(const std::string& s);

            /**
             * @brief Splits a string into a vector of strings based on a delimiter.
             * @param s The string to split.
             * @param delimiter The delimiter character.
             * @return A vector of substrings.
             */
            static std::vector<std::string> split(const std::string& s, char delimiter);

            /**
             * @brief Joins a vector of strings into a single string using a delimiter.
             * @param elements The vector of strings to join.
             * @param delimiter The delimiter string.
             * @return The joined string.
             */
            static std::string join(const std::vector<std::string>& elements, const std::string& delimiter);

            /**
             * @brief Checks if a string contains another substring (case-sensitive).
             * @param haystack The string to search within.
             * @param needle The substring to search for.
             * @return True if haystack contains needle, false otherwise.
             */
            static bool contains(const std::string& haystack, const std::string& needle);

            /**
             * @brief Checks if a string contains another substring (case-insensitive).
             * @param haystack The string to search within.
             * @param needle The substring to search for.
             * @return True if haystack contains needle (case-insensitive), false otherwise.
             */
            static bool containsIgnoreCase(const std::string& haystack, const std::string& needle);


        private:
            // Private constructor/destructor to make it a purely static utility class
            StringUtils() = delete;
            ~StringUtils() = delete;
        };

    } // namespace Utils
} // namespace ERP

#endif // MODULES_UTILS_STRINGUTILS_H