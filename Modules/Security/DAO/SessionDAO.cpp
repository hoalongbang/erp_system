// Modules/Security/DAO/SessionDAO.cpp
#include "Modules/Security/DAO/SessionDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "DAOHelpers.h"
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions
#include <sstream>
#include <stdexcept>
#include <typeinfo> // For std::bad_any_cast

namespace ERP {
namespace Security {
namespace DAOs {

SessionDAO::SessionDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Security::DTO::SessionDTO>(connectionPool, "sessions") { // Pass table name for SessionDTO
    Logger::Logger::getInstance().info("SessionDAO: Initialized.");
}

// toMap for SessionDTO
std::map<std::string, std::any> SessionDAO::toMap(const ERP::Security::DTO::SessionDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["user_id"] = dto.userId;
    data["token"] = dto.token;
    data["expiration_time"] = ERP::Utils::DateUtils::formatDateTime(dto.expirationTime, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalString(data, "ip_address", dto.ipAddress);
    ERP::DAOHelpers::putOptionalString(data, "user_agent", dto.userAgent);
    ERP::DAOHelpers::putOptionalString(data, "device_info", dto.deviceInfo);

    return data;
}

// fromMap for SessionDTO
ERP::Security::DTO::SessionDTO SessionDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Security::DTO::SessionDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "user_id", dto.userId);
        ERP::DAOHelpers::getPlainValue(data, "token", dto.token);
        ERP::DAOHelpers::getPlainTimeValue(data, "expiration_time", dto.expirationTime);
        ERP::DAOHelpers::getOptionalStringValue(data, "ip_address", dto.ipAddress);
        ERP::DAOHelpers::getOptionalStringValue(data, "user_agent", dto.userAgent);
        ERP::DAOHelpers::getOptionalStringValue(data, "device_info", dto.deviceInfo);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("SessionDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "SessionDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("SessionDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "SessionDAO: Unexpected error in fromMap.");
    }
    return dto;
}

} // namespace DAOs
} // namespace Security
} // namespace ERP