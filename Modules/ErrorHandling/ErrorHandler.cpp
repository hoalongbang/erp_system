// Modules/ErrorHandling/ErrorHandler.cpp
#include "ErrorHandler.h"
#include "Logger.h" // Include Logger concrete class here

namespace ERP {
namespace ErrorHandling {

// Static member initialization
std::optional<std::string> ErrorHandler::lastUserMessage_ = std::nullopt;

// Static map initialization for default user messages
const std::map<ERP::Common::ErrorCode, std::string> ErrorHandler::defaultUserMessages_ = {
    {ERP::Common::ErrorCode::OK, "Thao tác thành công."},
    {ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài nguyên yêu cầu."},
    {ERP::Common::ErrorCode::InvalidInput, "Dữ liệu nhập không hợp lệ. Vui lòng kiểm tra lại."},
    {ERP::Common::ErrorCode::Unauthorized, "Bạn chưa đăng nhập hoặc phiên làm việc đã hết hạn. Vui lòng đăng nhập lại."},
    {ERP::Common::ErrorCode::AuthenticationFailed, "Tên đăng nhập hoặc mật khẩu không đúng."},
    {ERP::Common::ErrorCode::Forbidden, "Bạn không có quyền thực hiện thao tác này."},
    {ERP::Common::ErrorCode::SessionExpired, "Phiên làm việc của bạn đã hết hạn. Vui lòng đăng nhập lại."},
    {ERP::Common::ErrorCode::DatabaseError, "Đã xảy ra lỗi cơ sở dữ liệu. Vui lòng thử lại."},
    {ERP::Common::ErrorCode::ServerError, "Đã xảy ra lỗi hệ thống nội bộ. Vui lòng liên hệ hỗ trợ."},
    {ERP::Common::ErrorCode::OperationFailed, "Thao tác không thành công. Vui lòng thử lại."},
    {ERP::Common::ErrorCode::InsufficientStock, "Không đủ số lượng tồn kho để thực hiện yêu cầu này."},
    {ERP::Common::ErrorCode::EncryptionError, "Lỗi mã hóa dữ liệu. Vui lòng liên hệ hỗ trợ."},
    {ERP::Common::ErrorCode::DecryptionError, "Lỗi giải mã dữ liệu. Vui lòng liên hệ hỗ trợ."}
};

void ErrorHandler::logError(
    ERP::Common::ErrorCode errorCode,
    const std::string& message,
    const std::optional<std::string>& filePath,
    const std::optional<int>& lineNumber) {

    std::string logMessage = "Error " + ERP::Common::errorCodeToString(errorCode) + ": " + message;
    if (filePath) {
        logMessage += " (File: " + *filePath;
        if (lineNumber) {
            logMessage += ", Line: " + std::to_string(*lineNumber);
        }
        logMessage += ")";
    }

    // Log based on severity of the error code
    switch (errorCode) {
        case ERP::Common::ErrorCode::OK: // Should not be logged as error
            ERP::Logger::Logger::getInstance().info(logMessage);
            break;
        case ERP::Common::ErrorCode::NotFound:
        case ERP::Common::ErrorCode::InvalidInput:
            ERP::Logger::Logger::getInstance().warning(logMessage);
            break;
        case ERP::Common::ErrorCode::Unauthorized:
        case ERP::Common::ErrorCode::AuthenticationFailed:
        case ERP::Common::ErrorCode::Forbidden:
        case ERP::Common::ErrorCode::SessionExpired:
        case ERP::Common::ErrorCode::OperationFailed:
        case ERP::Common::ErrorCode::InsufficientStock:
            ERP::Logger::Logger::getInstance().error(logMessage);
            break;
        case ERP::Common::ErrorCode::DatabaseError:
        case ERP::Common::ErrorCode::ServerError:
        case ERP::Common::ErrorCode::EncryptionError:
        case ERP::Common::ErrorCode::DecryptionError:
            ERP::Logger::Logger::getInstance().critical(logMessage);
            break;
        default:
            ERP::Logger::Logger::getInstance().error(logMessage);
            break;
    }
}

void ErrorHandler::handle(
    ERP::Common::ErrorCode errorCode,
    const std::string& internalMessage,
    const std::optional<std::string>& userMessage,
    const std::optional<std::string>& filePath,
    const std::optional<int>& lineNumber,
    bool throwException) {

    // Log the detailed internal message
    logError(errorCode, internalMessage, filePath, lineNumber);

    // Determine the user-friendly message
    std::string finalUserMessage;
    if (userMessage.has_value()) {
        finalUserMessage = *userMessage;
    } else {
        finalUserMessage = getDefaultUserMessage(errorCode);
    }
    lastUserMessage_ = finalUserMessage; // Store for UI retrieval

    // For critical errors, you might want to log the user message to console/GUI as well
    if (errorCode == ERP::Common::ErrorCode::ServerError || errorCode == ERP::Common::ErrorCode::DatabaseError) {
        // This is where a UI might display a modal message box directly if not handled elsewhere.
        // For now, we just log it as critical so it's visible.
        ERP::Logger::Logger::getInstance().critical("User Message: " + finalUserMessage);
    } else {
        ERP::Logger::Logger::getInstance().warning("User Message: " + finalUserMessage);
    }

    // Optionally throw an exception for the calling code to catch
    if (throwException) {
        throw std::runtime_error(internalMessage);
    }
}

std::optional<std::string> ErrorHandler::getLastUserMessage() {
    return lastUserMessage_;
}

void ErrorHandler::clearLastUserMessage() {
    lastUserMessage_ = std::nullopt;
}

bool ErrorHandler::isInputValidationError(ERP::Common::ErrorCode errorCode) {
    switch (errorCode) {
        case ERP::Common::ErrorCode::InvalidInput:
        case ERP::Common::ErrorCode::NotFound: // Not found can sometimes be due to invalid input ID
            return true;
        default:
            return false;
    }
}

bool ErrorHandler::isAuthenticationError(ERP::Common::ErrorCode errorCode) {
    switch (errorCode) {
        case ERP::Common::ErrorCode::Unauthorized:
        case ERP::Common::ErrorCode::AuthenticationFailed:
        case ERP::Common::ErrorCode::Forbidden:
        case ERP::Common::ErrorCode::SessionExpired:
            return true;
        default:
            return false;
    }
}

std::string ErrorHandler::getDefaultUserMessage(ERP::Common::ErrorCode errorCode) {
    auto it = defaultUserMessages_.find(errorCode);
    if (it != defaultUserMessages_.end()) {
        return it->second;
    }
    return "Đã xảy ra lỗi không xác định. Vui lòng thử lại hoặc liên hệ hỗ trợ.";
}

} // namespace ErrorHandling
} // namespace ERP