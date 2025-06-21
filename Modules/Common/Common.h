// Modules/Common/Common.h
#ifndef MODULES_COMMON_COMMON_H
#define MODULES_COMMON_COMMON_H
#include <string>   // For std::string
#include <ctime>    // For time_t (used indirectly by chrono)
#include <chrono>   // For std::chrono::system_clock (timestamps)
#include <map>      // For std::map used in string conversions
#include <stdexcept> // For std::out_of_range

namespace ERP {
    namespace Common {
        /**
         * @brief Enum defining general status for entities in the system.
         * This provides a consistent way to manage the lifecycle state of various data objects.
         */
        enum class EntityStatus {
            ACTIVE = 1,     // The entity is active and in normal use.
            INACTIVE = 0,   // The entity is inactive and not currently in use.
            PENDING = 2,    // The entity is awaiting approval or further processing.
            DELETED = 3,    // The entity is marked as deleted (soft delete).
            UNKNOWN = 99    // NEW: The entity's status is unknown or not specified.
        };

        /**
         * @brief Converts an EntityStatus enum value to its string representation.
         * @param status The EntityStatus to convert.
         * @return The string representation of the status.
         */
        inline std::string entityStatusToString(EntityStatus status) {
            static const std::map<EntityStatus, std::string> statusMap = {
                {EntityStatus::ACTIVE, "Active"},
                {EntityStatus::INACTIVE, "Inactive"},
                {EntityStatus::PENDING, "Pending"},
                {EntityStatus::DELETED, "Deleted"},
                {EntityStatus::UNKNOWN, "Unknown"}
            };
            auto it = statusMap.find(status);
            if (it != statusMap.end()) {
                return it->second;
            }
            return "Unknown";
        }

        /**
         * @brief Converts a string representation to an EntityStatus enum value.
         * @param statusString The string to convert.
         * @return The corresponding EntityStatus.
         */
        inline EntityStatus stringToEntityStatus(const std::string& statusString) {
            static const std::map<std::string, EntityStatus> stringToStatusMap = {
                {"Active", EntityStatus::ACTIVE},
                {"Inactive", EntityStatus::INACTIVE},
                {"Pending", EntityStatus::PENDING},
                {"Deleted", EntityStatus::DELETED},
                {"Unknown", EntityStatus::UNKNOWN}
            };
            auto it = stringToStatusMap.find(statusString);
            if (it != stringToStatusMap.end()) {
                return it->second;
            }
            return EntityStatus::UNKNOWN;
        }

        /**
         * @brief Enum defining common error codes throughout the system.
         * These codes provide a structured way to categorize and handle different types of errors.
         */
        enum class ErrorCode {
            OK = 0,                     // Operation successful.
            NotFound = 100,             // Requested resource or entity not found.
            InvalidInput = 200,         // Input data is invalid or malformed.
            Unauthorized = 300,         // User is not authenticated.
            AuthenticationFailed = 301, // User authentication failed (e.g., wrong password).
            Forbidden = 400,            // User is authenticated but does not have permission to perform the action.
            SessionExpired = 401,       // User session has expired.
            DatabaseError = 500,        // An error occurred during a database operation.
            ServerError = 501,          // An unexpected internal server error occurred.
            OperationFailed = 600,      // A business logic operation failed for a non-specific reason.
            InsufficientStock = 700,    // Not enough stock to fulfill a request.
            EncryptionError = 800,      // Error during data encryption.
            DecryptionError = 801       // Error during data decryption.
        };

        /**
         * @brief Converts an ErrorCode enum value to its string representation.
         * @param code The ErrorCode to convert.
         * @return The string representation of the error code.
         */
        inline std::string errorCodeToString(ErrorCode code) {
            static const std::map<ErrorCode, std::string> codeMap = {
                {ErrorCode::OK, "OK"},
                {ErrorCode::NotFound, "NotFound"},
                {ErrorCode::InvalidInput, "InvalidInput"},
                {ErrorCode::Unauthorized, "Unauthorized"},
                {ErrorCode::AuthenticationFailed, "AuthenticationFailed"},
                {ErrorCode::Forbidden, "Forbidden"},
                {ErrorCode::SessionExpired, "SessionExpired"},
                {ErrorCode::DatabaseError, "DatabaseError"},
                {ErrorCode::ServerError, "ServerError"},
                {ErrorCode::OperationFailed, "OperationFailed"},
                {ErrorCode::InsufficientStock, "InsufficientStock"},
                {ErrorCode::EncryptionError, "EncryptionError"},
                {ErrorCode::DecryptionError, "DecryptionError"}
            };
            auto it = codeMap.find(code);
            if (it != codeMap.end()) {
                return it->second;
            }
            return "UnknownError";
        }

        /**
         * @brief Enum for log severity levels.
         */
        enum class LogSeverity {
            DEBUG,
            INFO,
            WARNING,
            ERROR,
            CRITICAL
        };

        /**
         * @brief Converts a LogSeverity enum value to its string representation.
         * @param severity The LogSeverity to convert.
         * @return The string representation of the log severity.
         */
        inline std::string logSeverityToString(LogSeverity severity) {
            static const std::map<LogSeverity, std::string> severityMap = {
                {LogSeverity::DEBUG, "DEBUG"},
                {LogSeverity::INFO, "INFO"},
                {LogSeverity::WARNING, "WARNING"},
                {LogSeverity::ERROR, "ERROR"},
                {LogSeverity::CRITICAL, "CRITICAL"}
            };
            auto it = severityMap.find(severity);
            if (it != severityMap.end()) {
                return it->second;
            }
            return "UNKNOWN";
        }


        /**
         * @brief Global constant for the standard datetime format used across the system.
         * This ensures consistency in date and time string representations for storage and display.
         */
        const std::string DATETIME_FORMAT = "%Y-%m-%d %H:%M:%S";

    } // namespace Common
} // namespace ERP
#endif // MODULES_COMMON_COMMON_H