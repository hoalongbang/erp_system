// Modules/ErrorHandling/ErrorHandler.h
#ifndef MODULES_ERRORHANDLING_ERRORHANDLER_H
#define MODULES_ERRORHANDLING_ERRORHANDLER_H
#include <string>       // For std::string
#include <stdexcept>    // For std::runtime_error (for throwing exceptions)
#include <map>          // For std::map (for static error message mappings)
#include <memory>       // For std::shared_ptr
#include <optional>     // For std::optional

// Rút gọn includes
#include "Common.h" // For ErrorCode enum (from Common module)
#include "Logger.h" // For logging (forward declaration below)

// Forward declaration of Logger to avoid circular dependency include
namespace ERP {
namespace Logger {
class Logger; // Forward declare Logger class
}
}

namespace ERP {
namespace ErrorHandling { // Namespace for ErrorHandling module
/**
 * @brief The ErrorHandler class provides static methods for centralized error handling.
 * It is responsible for logging detailed internal error messages and, optionally,
 * providing user-friendly messages. It can also throw exceptions for critical errors.
 */
class ErrorHandler {
public:
    /**
     * @brief Logs an error internally without necessarily throwing an exception or showing a user message.
     * @param errorCode The specific error code from the ErrorCode enum.
     * @param message A detailed internal message for logging.
     * @param filePath Optional: The path of the file where the error occurred.
     * @param lineNumber Optional: The line number in the file where the error occurred.
     */
    static void logError(
        ERP::Common::ErrorCode errorCode,
        const std::string& message,
        const std::optional<std::string>& filePath = std::nullopt,
        const std::optional<int>& lineNumber = std::nullopt);

    /**
     * @brief Handles an error by logging it and providing a user-friendly message.
     * This is the primary entry point for business logic to report errors to the user.
     * @param errorCode The specific error code from the ErrorCode enum.
     * @param internalMessage A detailed internal message for logging.
     * @param userMessage Optional: A user-friendly message to display. If not provided, a default message is used.
     * @param filePath Optional: The path of the file where the error occurred.
     * @param lineNumber Optional: The line number in the file where the error occurred.
     * @param throwException If true, throws a std::runtime_error after logging. Defaults to false.
     */
    static void handle(
        ERP::Common::ErrorCode errorCode,
        const std::string& internalMessage,
        const std::optional<std::string>& userMessage = std::nullopt,
        const std::optional<std::string>& filePath = std::nullopt,
        const std::optional<int>& lineNumber = std::nullopt,
        bool throwException = false);

    /**
     * @brief Gets the last user-friendly error message that was handled.
     * Useful for UI components to retrieve the message to display.
     * @return An optional string containing the last user message.
     */
    static std::optional<std::string> getLastUserMessage();

    /**
     * @brief Clears the last stored user error message.
     */
    static void clearLastUserMessage();

    /**
     * @brief Checks if a given error code indicates an input validation error.
     * @param errorCode The error code to check.
     * @return True if it's an input validation error, false otherwise.
     */
    static bool isInputValidationError(ERP::Common::ErrorCode errorCode);

    /**
     * @brief Checks if a given error code indicates an authentication/authorization error.
     * @param errorCode The error code to check.
     * @return True if it's an authentication/authorization error, false otherwise.
     */
    static bool isAuthenticationError(ERP::Common::ErrorCode errorCode);

private:
    // Static member to store the last user-friendly message
    static std::optional<std::string> lastUserMessage_;

    // Static map for default user messages based on ErrorCode
    static const std::map<ERP::Common::ErrorCode, std::string> defaultUserMessages_;

    // Private helper to get default user message
    static std::string getDefaultUserMessage(ERP::Common::ErrorCode errorCode);
};
} // namespace ErrorHandling
} // namespace ERP
#endif // MODULES_ERRORHANDLING_ERRORHANDLER_H