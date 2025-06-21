// DAOBase/DAOHelpers.h
#ifndef ERP_DAO_HELPERS_H
#define ERP_DAO_HELPERS_H

#include <map>
#include <string>
#include <any>
#include <optional>
#include <chrono>      // For std::chrono::system_clock::time_point
#include <type_traits> // For std::is_same, std::enable_if_t, std::is_base_of
#include <algorithm>   // For std::transform
#include <cctype>      // For ::tolower

// Rút gọn các include paths
#include "Logger.h"      // For Logger::Logger::getInstance().error
#include "ErrorHandler.h" // For ERP::ErrorHandling::ErrorHandler::logError
#include "Common.h"      // For ERP::Common::DATETIME_FORMAT, ERP::Common::EntityStatus
#include "DateUtils.h"   // For ERP::Utils::DateUtils::parseDateTime, formatDateTime

namespace ERP {
    namespace DAOHelpers { // A dedicated namespace for these helpers to avoid name clashes

        // Helper function template for safe std::any_cast (for plain targets like string, int, double, bool)
        // This function attempts to cast a std::any value from a map to a target variable of type T.
        // It handles cases where the key might not exist or the type might not match,
        // logging an error and leaving the target variable unchanged in such cases.
        template<typename T>
        inline void getPlainValue(const std::map<std::string, std::any>& data, const std::string& key, T& target) {
            auto it = data.find(key);
            if (it != data.end()) {
                try {
                    if (it->second.type() == typeid(T)) {
                        target = std::any_cast<T>(it->second);
                    }
                    else if (it->second.type() == typeid(int) && std::is_same<T, double>::value) {
                        // Special handling for implicit conversion from int to double
                        target = static_cast<T>(std::any_cast<int>(it->second));
                    }
                    else if (it->second.type() == typeid(std::string) && std::is_same<T, bool>::value) {
                        // Special handling for string to bool (e.g., "true" to true)
                        std::string s_val = std::any_cast<std::string>(it->second);
                        std::transform(s_val.begin(), s_val.end(), s_val.begin(), ::tolower);
                        target = (s_val == "true" || s_val == "1");
                    }
                    // Corrected: Handling for int to string conversion if needed.
                    // This is for cases where DB might return INTEGER and DTO expects std::string (e.g. for IDs if they are mixed types)
                    else if (it->second.type() == typeid(int) && std::is_same<T, std::string>::value) {
                        target = std::to_string(std::any_cast<int>(it->second));
                    }
                    else if (it->second.type() == typeid(long long) && std::is_same<T, int>::value) {
                        target = static_cast<T>(std::any_cast<long long>(it->second));
                    }
                    else if (it->second.type() == typeid(long long) && std::is_same<T, double>::value) {
                        target = static_cast<T>(std::any_cast<long long>(it->second));
                    }
                    else if (it->second.type() == typeid(long long) && std::is_same<T, std::string>::value) {
                        target = std::to_string(std::any_cast<long long>(it->second));
                    }
                    else if (it->second.type() == typeid(std::any) && !it->second.has_value()) {
                        // Value is an empty std::any (e.g., from std::nullopt inserted into map)
                        // Do nothing, target keeps its default value, effectively treating as null.
                    }
                    else {
                        // Log a general type mismatch if none of the specific cases match
                        ERP::Logger::Logger::getInstance().error(
                            "DAOHelpers::getPlainValue - Type mismatch for key '" + key +
                            "'. Expected target type: " + typeid(T).name() +
                            ", Actual std::any type: " + it->second.type().name()
                        );
                    }
                }
                catch (const std::bad_any_cast& e) {
                    // Log an error if a bad cast occurs, indicating a data type mismatch
                    ERP::Logger::Logger::getInstance().error("DAOHelpers::getPlainValue - Bad any_cast for key '" + key + "': " + e.what());
                }
            }
            // If key not found or type mismatch (not handled by special cases), target retains its default/initial value.
        }

        // Helper function for plain std::chrono::system_clock::time_point targets (non-optional)
        inline void getPlainTimeValue(const std::map<std::string, std::any>& data, const std::string& key, std::chrono::system_clock::time_point& target) {
            auto it = data.find(key);
            if (it != data.end() && it->second.type() == typeid(std::string)) {
                auto parsed = ERP::Utils::DateUtils::parseDateTime(std::any_cast<std::string>(it->second), ERP::Common::DATETIME_FORMAT);
                if (parsed) {
                    target = *parsed;
                }
                else {
                    ERP::Logger::Logger::getInstance().error("DAOHelpers::getPlainTimeValue - Failed to parse datetime for key '" + key + "'.");
                }
            }
            else {
                 // If key not found or not a string, target retains its default/initial value.
            }
        }

        // Helper function for optional string targets (std::optional<std::string>)
        inline void getOptionalStringValue(const std::map<std::string, std::any>& data, const std::string& key, std::optional<std::string>& target) {
            auto it = data.find(key);
            if (it != data.end()) {
                if (it->second.type() == typeid(std::string)) {
                    target = std::any_cast<std::string>(it->second);
                }
                else if (it->second.type() == typeid(std::any) && !it->second.has_value()) {
                    target = std::nullopt;
                }
                else {
                    ERP::Logger::Logger::getInstance().error("DAOHelpers::getOptionalStringValue - Type mismatch for key '" + key + "'. Expected string or empty std::any.");
                    target = std::nullopt;
                }
            }
            else {
                target = std::nullopt;
            }
        }

        // Helper function for optional double targets (std::optional<double>)
        inline void getOptionalDoubleValue(const std::map<std::string, std::any>& data, const std::string& key, std::optional<double>& target) {
            auto it = data.find(key);
            if (it != data.end()) {
                if (it->second.type() == typeid(double)) {
                    target = std::any_cast<double>(it->second);
                }
                else if (it->second.type() == typeid(int)) { // Allow implicit conversion from int to double
                    target = static_cast<double>(std::any_cast<int>(it->second));
                }
                else if (it->second.type() == typeid(long long)) { // Allow implicit conversion from long long to double
                    target = static_cast<double>(std::any_cast<long long>(it->second));
                }
                else if (it->second.type() == typeid(std::any) && !it->second.has_value()) {
                    target = std::nullopt;
                }
                else {
                    ERP::Logger::Logger::getInstance().error("DAOHelpers::getOptionalDoubleValue - Type mismatch for key '" + key + "'. Expected double, int, long long, or empty std::any.");
                    target = std::nullopt;
                }
            }
            else {
                target = std::nullopt;
            }
        }

        // Helper function for optional int targets (std::optional<int>)
        inline void getOptionalIntValue(const std::map<std::string, std::any>& data, const std::string& key, std::optional<int>& target) {
            auto it = data.find(key);
            if (it != data.end()) {
                if (it->second.type() == typeid(int)) {
                    target = std::any_cast<int>(it->second);
                }
                else if (it->second.type() == typeid(long long)) { // Allow implicit conversion from long long to int if safe
                    target = static_cast<int>(std::any_cast<long long>(it->second));
                }
                else if (it->second.type() == typeid(std::any) && !it->second.has_value()) {
                    target = std::nullopt;
                }
                else {
                    ERP::Logger::Logger::getInstance().error("DAOHelpers::getOptionalIntValue - Type mismatch for key '" + key + "'. Expected int, long long, or empty std::any.");
                    target = std::nullopt;
                }
            }
            else {
                target = std::nullopt;
            }
        }

        // Helper function for optional time_point targets (std::optional<std::chrono::system_clock::time_point>)
        inline void getOptionalTimeValue(const std::map<std::string, std::any>& data, const std::string& key, std::optional<std::chrono::system_clock::time_point>& target) {
            auto it = data.find(key);
            if (it != data.end()) {
                if (it->second.type() == typeid(std::string)) {
                    auto parsed = ERP::Utils::DateUtils::parseDateTime(std::any_cast<std::string>(it->second), ERP::Common::DATETIME_FORMAT);
                    if (parsed) target = *parsed;
                    else {
                        ERP::Logger::Logger::getInstance().error("DAOHelpers::getOptionalTimeValue - Failed to parse datetime for key '" + key + "'.");
                        target = std::nullopt;
                    }
                }
                else if (it->second.type() == typeid(std::any) && !it->second.has_value()) {
                    target = std::nullopt;
                }
                else {
                    ERP::Logger::Logger::getInstance().error("DAOHelpers::getOptionalTimeValue - Type mismatch for key '" + key + "'. Expected string or empty std::any.");
                    target = std::nullopt;
                }
            }
            else {
                target = std::nullopt;
            }
        }

        // Helper function to put an optional string into a map
        inline void putOptionalString(std::map<std::string, std::any>& data, const std::string& key, const std::optional<std::string>& value) {
            if (value.has_value()) {
                data[key] = *value;
            }
            else {
                data[key] = std::any(); // Store as empty std::any to represent null/optional
            }
        }

        // Helper function to put an optional time_point into a map
        inline void putOptionalTime(std::map<std::string, std::any>& data, const std::string& key, const std::optional<std::chrono::system_clock::time_point>& value) {
            if (value.has_value()) {
                data[key] = ERP::Utils::DateUtils::formatDateTime(*value, ERP::Common::DATETIME_FORMAT);
            }
            else {
                data[key] = std::any(); // Store as empty std::any to represent null/optional
            }
        }

        // Helper function to put an optional double into a map
        inline void putOptionalDouble(std::map<std::string, std::any>& data, const std::string& key, const std::optional<double>& value) {
            if (value.has_value()) {
                data[key] = *value;
            }
            else {
                data[key] = std::any(); // Store as empty std::any to represent null/optional
            }
        }

        // Helper function to put an optional int into a map
        inline void putOptionalIntValue(std::map<std::string, std::any>& data, const std::string& key, const std::optional<int>& value) {
            if (value.has_value()) {
                data[key] = *value;
            }
            else {
                data[key] = std::any(); // Store as empty std::any to represent null/optional
            }
        }

    } // namespace DAOHelpers
} // namespace ERP

#endif // ERP_DAO_HELPERS_H