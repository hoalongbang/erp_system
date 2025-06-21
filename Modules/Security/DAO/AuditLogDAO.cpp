// Modules/Security/DAO/AuditLogDAO.cpp
#include "Modules/Security/DAO/AuditLogDAO.h"
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
namespace Security {
namespace DAOs {

using json = nlohmann::json;

AuditLogDAO::AuditLogDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Security::DTO::AuditLogDTO>(connectionPool, "audit_logs") { // Pass table name for AuditLogDTO
    Logger::Logger::getInstance().info("AuditLogDAO: Initialized.");
}

// toMap for AuditLogDTO
std::map<std::string, std::any> AuditLogDAO::toMap(const ERP::Security::DTO::AuditLogDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["user_id"] = dto.userId;
    data["user_name"] = dto.userName;
    ERP::DAOHelpers::putOptionalString(data, "session_id", dto.sessionId);
    data["action_type"] = static_cast<int>(dto.actionType);
    data["severity"] = static_cast<int>(dto.severity);
    data["module"] = dto.module;
    data["sub_module"] = dto.subModule;
    ERP::DAOHelpers::putOptionalString(data, "entity_id", dto.entityId);
    ERP::DAOHelpers::putOptionalString(data, "entity_type", dto.entityType);
    ERP::DAOHelpers::putOptionalString(data, "entity_name", dto.entityName);
    ERP::DAOHelpers::putOptionalString(data, "ip_address", dto.ipAddress);
    ERP::DAOHelpers::putOptionalString(data, "user_agent", dto.userAgent);
    ERP::DAOHelpers::putOptionalString(data, "workstation_id", dto.workstationId);

    ERP::DAOHelpers::putOptionalString(data, "production_line_id", dto.productionLineId);
    ERP::DAOHelpers::putOptionalString(data, "shift_id", dto.shiftId);
    ERP::DAOHelpers::putOptionalString(data, "batch_number", dto.batchNumber);
    ERP::DAOHelpers::putOptionalString(data, "part_number", dto.partNumber);

    // Serialize std::map<string, any> to JSON string for before_data, after_data, and metadata
    try {
        if (!dto.beforeData.empty()) {
            json beforeDataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.beforeData));
            data["before_data_json"] = beforeDataJson.dump();
        } else {
            data["before_data_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("AuditLogDAO: toMap - Error serializing before_data: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "AuditLogDAO: Error serializing before data.");
        data["before_data_json"] = "";
    }

    try {
        if (!dto.afterData.empty()) {
            json afterDataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.afterData));
            data["after_data_json"] = afterDataJson.dump();
        } else {
            data["after_data_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("AuditLogDAO: toMap - Error serializing after_data: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "AuditLogDAO: Error serializing after data.");
        data["after_data_json"] = "";
    }

    ERP::DAOHelpers::putOptionalString(data, "change_reason", dto.changeReason);

    try {
        if (!dto.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("AuditLogDAO: toMap - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "AuditLogDAO: Error serializing metadata.");
        data["metadata_json"] = "";
    }

    ERP::DAOHelpers::putOptionalString(data, "comments", dto.comments);
    ERP::DAOHelpers::putOptionalString(data, "approval_id", dto.approvalId);
    data["is_compliant"] = dto.isCompliant;
    ERP::DAOHelpers::putOptionalString(data, "compliance_note", dto.complianceNote);

    return data;
}

// fromMap for AuditLogDTO
ERP::Security::DTO::AuditLogDTO AuditLogDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Security::DTO::AuditLogDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "user_id", dto.userId);
        ERP::DAOHelpers::getPlainValue(data, "user_name", dto.userName);
        ERP::DAOHelpers::getOptionalStringValue(data, "session_id", dto.sessionId);
        
        int actionTypeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "action_type", actionTypeInt)) dto.actionType = static_cast<ERP::Security::DTO::AuditActionType>(actionTypeInt);
        
        int severityInt;
        if (ERP::DAOHelpers::getPlainValue(data, "severity", severityInt)) dto.severity = static_cast<ERP::Common::LogSeverity>(severityInt);
        
        ERP::DAOHelpers::getPlainValue(data, "module", dto.module);
        ERP::DAOHelpers::getPlainValue(data, "sub_module", dto.subModule);
        ERP::DAOHelpers::getOptionalStringValue(data, "entity_id", dto.entityId);
        ERP::DAOHelpers::getOptionalStringValue(data, "entity_type", dto.entityType);
        ERP::DAOHelpers::getOptionalStringValue(data, "entity_name", dto.entityName);
        ERP::DAOHelpers::getOptionalStringValue(data, "ip_address", dto.ipAddress);
        ERP::DAOHelpers::getOptionalStringValue(data, "user_agent", dto.userAgent);
        ERP::DAOHelpers::getOptionalStringValue(data, "workstation_id", dto.workstationId);

        ERP::DAOHelpers::getOptionalStringValue(data, "production_line_id", dto.productionLineId);
        ERP::DAOHelpers::getOptionalStringValue(data, "shift_id", dto.shiftId);
        ERP::DAOHelpers::getOptionalStringValue(data, "batch_number", dto.batchNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "part_number", dto.partNumber);

        // Deserialize JSON string from before_data, after_data, and metadata
        if (data.count("before_data_json") && data.at("before_data_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("before_data_json"));
            if (!jsonStr.empty()) {
                dto.beforeData = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
        if (data.count("after_data_json") && data.at("after_data_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("after_data_json"));
            if (!jsonStr.empty()) {
                dto.afterData = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }

        ERP::DAOHelpers::getOptionalStringValue(data, "change_reason", dto.changeReason);

        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                dto.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }

        ERP::DAOHelpers::getOptionalStringValue(data, "comments", dto.comments);
        ERP::DAOHelpers::getOptionalStringValue(data, "approval_id", dto.approvalId);
        ERP::DAOHelpers::getPlainValue(data, "is_compliant", dto.isCompliant);
        ERP::DAOHelpers::getOptionalStringValue(data, "compliance_note", dto.complianceNote);

    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("AuditLogDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "AuditLogDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("AuditLogDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "AuditLogDAO: Unexpected error in fromMap.");
    }
    return dto;
}

} // namespace DAOs
} // namespace Security
} // namespace ERP