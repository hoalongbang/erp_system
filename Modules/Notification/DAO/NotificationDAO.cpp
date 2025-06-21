// Modules/Notification/DAO/NotificationDAO.cpp
#include "Modules/Notification/DAO/NotificationDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "DAOHelpers.h"
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions
#include <nlohmann/json.hpp> // For JSON serialization/deserialization of std::map<string, any>
#include <sstream>
#include <stdexcept>
#include <typeinfo> // For std::bad_any_cast

namespace ERP {
namespace Notification {
namespace DAOs {

using json = nlohmann::json;

NotificationDAO::NotificationDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Notification::DTO::NotificationDTO>(connectionPool, "notifications") { // Pass table name for NotificationDTO
    Logger::Logger::getInstance().info("NotificationDAO: Initialized.");
}

// toMap for NotificationDTO
std::map<std::string, std::any> NotificationDAO::toMap(const ERP::Notification::DTO::NotificationDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["user_id"] = dto.userId;
    data["title"] = dto.title;
    data["message"] = dto.message;
    data["type"] = static_cast<int>(dto.type);
    data["priority"] = static_cast<int>(dto.priority);
    data["sent_time"] = ERP::Utils::DateUtils::formatDateTime(dto.sentTime, ERP::Common::DATETIME_FORMAT);
    data["is_read"] = dto.isRead;
    ERP::DAOHelpers::putOptionalString(data, "sender_id", dto.senderId);
    ERP::DAOHelpers::putOptionalString(data, "related_entity_id", dto.relatedEntityId);
    ERP::DAOHelpers::putOptionalString(data, "related_entity_type", dto.relatedEntityType);

    // Serialize std::map<string, any> to JSON string for custom_data and metadata
    try {
        if (!dto.customData.empty()) {
            json customDataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.customData));
            data["custom_data_json"] = customDataJson.dump();
        } else {
            data["custom_data_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("NotificationDAO: toMap - Error serializing custom_data: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "NotificationDAO: Error serializing custom data.");
        data["custom_data_json"] = "";
    }

    try {
        if (!dto.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("NotificationDAO: toMap - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "NotificationDAO: Error serializing metadata.");
        data["metadata_json"] = "";
    }
    return data;
}

// fromMap for NotificationDTO
ERP::Notification::DTO::NotificationDTO NotificationDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Notification::DTO::NotificationDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "user_id", dto.userId);
        ERP::DAOHelpers::getPlainValue(data, "title", dto.title);
        ERP::DAOHelpers::getPlainValue(data, "message", dto.message);
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) dto.type = static_cast<ERP::Notification::DTO::NotificationType>(typeInt);
        
        int priorityInt;
        if (ERP::DAOHelpers::getPlainValue(data, "priority", priorityInt)) dto.priority = static_cast<ERP::Notification::DTO::NotificationPriority>(priorityInt);
        
        ERP::DAOHelpers::getPlainTimeValue(data, "sent_time", dto.sentTime);
        ERP::DAOHelpers::getPlainValue(data, "is_read", dto.isRead);
        ERP::DAOHelpers::getOptionalStringValue(data, "sender_id", dto.senderId);
        ERP::DAOHelpers::getOptionalStringValue(data, "related_entity_id", dto.relatedEntityId);
        ERP::DAOHelpers::getOptionalStringValue(data, "related_entity_type", dto.relatedEntityType);

        // Deserialize JSON string to std::map<string, any> for custom_data and metadata
        if (data.count("custom_data_json") && data.at("custom_data_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("custom_data_json"));
            if (!jsonStr.empty()) {
                dto.customData = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                dto.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("NotificationDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "NotificationDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("NotificationDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "NotificationDAO: Unexpected error in fromMap.");
    }
    return dto;
}

} // namespace DAOs
} // namespace Notification
} // namespace ERP