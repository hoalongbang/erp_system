// Modules/Manufacturing/DAO/MaintenanceManagementDAO.cpp
#include "Modules/Manufacturing/DAO/MaintenanceManagementDAO.h"
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
namespace Manufacturing {
namespace DAOs {

using json = nlohmann::json;

MaintenanceManagementDAO::MaintenanceManagementDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Manufacturing::DTO::MaintenanceRequestDTO>(connectionPool, "maintenance_requests") { // Pass table name for Request DTO
    Logger::Logger::getInstance().info("MaintenanceManagementDAO: Initialized.");
}

// toMap for MaintenanceRequestDTO
std::map<std::string, std::any> MaintenanceManagementDAO::toMap(const ERP::Manufacturing::DTO::MaintenanceRequestDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["asset_id"] = dto.assetId;
    data["request_type"] = static_cast<int>(dto.requestType);
    data["priority"] = static_cast<int>(dto.priority);
    data["status"] = static_cast<int>(dto.status);
    ERP::DAOHelpers::putOptionalString(data, "description", dto.description);
    data["requested_by_user_id"] = dto.requestedByUserId;
    data["requested_date"] = ERP::Utils::DateUtils::formatDateTime(dto.requestedDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalTime(data, "scheduled_date", dto.scheduledDate);
    ERP::DAOHelpers::putOptionalString(data, "assigned_to_user_id", dto.assignedToUserId);
    ERP::DAOHelpers::putOptionalString(data, "failure_reason", dto.failureReason);

    // Serialize std::map<string, any> to JSON string for metadata
    try {
        if (!dto.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("MaintenanceManagementDAO: toMap (Request) - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaintenanceManagementDAO: Error serializing request metadata.");
        data["metadata_json"] = "";
    }
    return data;
}

// fromMap for MaintenanceRequestDTO
ERP::Manufacturing::DTO::MaintenanceRequestDTO MaintenanceManagementDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Manufacturing::DTO::MaintenanceRequestDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "asset_id", dto.assetId);
        
        int requestTypeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "request_type", requestTypeInt)) dto.requestType = static_cast<ERP::Manufacturing::DTO::MaintenanceRequestType>(requestTypeInt);
        
        int priorityInt;
        if (ERP::DAOHelpers::getPlainValue(data, "priority", priorityInt)) dto.priority = static_cast<ERP::Manufacturing::DTO::MaintenancePriority>(priorityInt);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Manufacturing::DTO::MaintenanceRequestStatus>(statusInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "description", dto.description);
        ERP::DAOHelpers::getPlainValue(data, "requested_by_user_id", dto.requestedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "requested_date", dto.requestedDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "scheduled_date", dto.scheduledDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "assigned_to_user_id", dto.assignedToUserId);
        ERP::DAOHelpers::getOptionalStringValue(data, "failure_reason", dto.failureReason);

        // Deserialize JSON string to std::map<string, any> for metadata
        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                dto.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("MaintenanceManagementDAO: fromMap (Request) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaintenanceManagementDAO: Data type mismatch in fromMap (Request).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("MaintenanceManagementDAO: fromMap (Request) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaintenanceManagementDAO: Unexpected error in fromMap (Request).");
    }
    return dto;
}

// --- MaintenanceActivityDTO specific methods and helpers ---
std::map<std::string, std::any> MaintenanceManagementDAO::toMap(const ERP::Manufacturing::DTO::MaintenanceActivityDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["maintenance_request_id"] = dto.maintenanceRequestId;
    data["activity_description"] = dto.activityDescription;
    data["activity_date"] = ERP::Utils::DateUtils::formatDateTime(dto.activityDate, ERP::Common::DATETIME_FORMAT);
    data["performed_by_user_id"] = dto.performedByUserId;
    data["duration_hours"] = dto.durationHours;
    ERP::DAOHelpers::putOptionalDouble(data, "cost", dto.cost);
    ERP::DAOHelpers::putOptionalString(data, "cost_currency", dto.costCurrency);
    ERP::DAOHelpers::putOptionalString(data, "parts_used", dto.partsUsed);

    // Serialize std::map<string, any> to JSON string for metadata
    try {
        if (!dto.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("MaintenanceManagementDAO: toMap (Activity) - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaintenanceManagementDAO: Error serializing activity metadata.");
        data["metadata_json"] = "";
    }
    return data;
}

ERP::Manufacturing::DTO::MaintenanceActivityDTO MaintenanceManagementDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Manufacturing::DTO::MaintenanceActivityDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "maintenance_request_id", dto.maintenanceRequestId);
        ERP::DAOHelpers::getPlainValue(data, "activity_description", dto.activityDescription);
        ERP::DAOHelpers::getPlainTimeValue(data, "activity_date", dto.activityDate);
        ERP::DAOHelpers::getPlainValue(data, "performed_by_user_id", dto.performedByUserId);
        ERP::DAOHelpers::getPlainValue(data, "duration_hours", dto.durationHours);
        ERP::DAOHelpers::getOptionalDoubleValue(data, "cost", dto.cost);
        ERP::DAOHelpers::getOptionalStringValue(data, "cost_currency", dto.costCurrency);
        ERP::DAOHelpers::getOptionalStringValue(data, "parts_used", dto.partsUsed);

        // Deserialize JSON string to std::map<string, any> for metadata
        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                dto.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("MaintenanceManagementDAO: fromMap (Activity) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaintenanceManagementDAO: Data type mismatch in fromMap (Activity).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("MaintenanceManagementDAO: fromMap (Activity) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaintenanceManagementDAO: Unexpected error in fromMap (Activity).");
    }
    return dto;
}

bool MaintenanceManagementDAO::createMaintenanceActivity(const ERP::Manufacturing::DTO::MaintenanceActivityDTO& activity) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementDAO: Attempting to create new maintenance activity.");
    std::map<std::string, std::any> data = toMap(activity);
    
    std::string columns;
    std::string placeholders;
    std::map<std::string, std::any> params;
    bool first = true;

    for (const auto& pair : data) {
        if (!first) { columns += ", "; placeholders += ", "; }
        columns += pair.first;
        placeholders += "?";
        params[pair.first] = pair.second;
        first = false;
    }

    std::string sql = "INSERT INTO " + maintenanceActivitiesTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaintenanceManagementDAO", "createMaintenanceActivity", sql, params
    );
}

std::optional<ERP::Manufacturing::DTO::MaintenanceActivityDTO> MaintenanceManagementDAO::getMaintenanceActivityById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementDAO: Attempting to get maintenance activity by ID: " + id);
    std::string sql = "SELECT * FROM " + maintenanceActivitiesTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "MaintenanceManagementDAO", "getMaintenanceActivityById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Manufacturing::DTO::MaintenanceActivityDTO> MaintenanceManagementDAO::getMaintenanceActivitiesByRequestId(const std::string& requestId) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementDAO: Retrieving maintenance activities for request ID: " + requestId);
    std::string sql = "SELECT * FROM " + maintenanceActivitiesTableName_ + " WHERE maintenance_request_id = ?;";
    std::map<std::string, std::any> params;
    params["maintenance_request_id"] = requestId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "MaintenanceManagementDAO", "getMaintenanceActivitiesByRequestId", sql, params
    );

    std::vector<ERP::Manufacturing::DTO::MaintenanceActivityDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool MaintenanceManagementDAO::updateMaintenanceActivity(const ERP::Manufacturing::DTO::MaintenanceActivityDTO& activity) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementDAO: Attempting to update maintenance activity with ID: " + activity.id);
    std::map<std::string, std::any> data = toMap(activity);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementDAO: Update maintenance activity called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaintenanceManagementDAO: Update maintenance activity called with empty data or missing ID.");
        return false;
    }

    std::string setClause;
    std::map<std::string, std::any> params;
    bool firstSet = true;

    for (const auto& pair : data) {
        if (pair.first == "id") continue;
        if (!firstSet) setClause += ", ";
        setClause += pair.first + " = ?";
        params[pair.first] = pair.second;
        firstSet = false;
    }

    std::string sql = "UPDATE " + maintenanceActivitiesTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = activity.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaintenanceManagementDAO", "updateMaintenanceActivity", sql, params
    );
}

bool MaintenanceManagementDAO::removeMaintenanceActivity(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementDAO: Attempting to remove maintenance activity with ID: " + id);
    std::string sql = "DELETE FROM " + maintenanceActivitiesTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaintenanceManagementDAO", "removeMaintenanceActivity", sql, params
    );
}

bool MaintenanceManagementDAO::removeMaintenanceActivitiesByRequestId(const std::string& requestId) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementDAO: Attempting to remove all activities for request ID: " + requestId);
    std::string sql = "DELETE FROM " + maintenanceActivitiesTableName_ + " WHERE maintenance_request_id = ?;";
    std::map<std::string, std::any> params;
    params["maintenance_request_id"] = requestId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaintenanceManagementDAO", "removeMaintenanceActivitiesByRequestId", sql, params
    );
}


} // namespace DAOs
} // namespace Manufacturing
} // namespace ERP