// Modules/Integration/DAO/DeviceConfigDAO.cpp
#include "DeviceConfigDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions

namespace ERP {
namespace Integration {
namespace DAOs {

DeviceConfigDAO::DeviceConfigDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Integration::DTO::DeviceConfigDTO>(connectionPool, "device_configs") {
    ERP::Logger::Logger::getInstance().info("DeviceConfigDAO: Initialized.");
}

std::map<std::string, std::any> DeviceConfigDAO::toMap(const ERP::Integration::DTO::DeviceConfigDTO& config) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(config); // BaseDTO fields

    data["device_name"] = config.deviceName;
    data["device_identifier"] = config.deviceIdentifier;
    data["type"] = static_cast<int>(config.type);
    ERP::DAOHelpers::putOptionalString(data, "connection_string", config.connectionString);
    ERP::DAOHelpers::putOptionalString(data, "ip_address", config.ipAddress);
    data["connection_status"] = static_cast<int>(config.connectionStatus);
    ERP::DAOHelpers::putOptionalString(data, "location_id", config.locationId);
    ERP::DAOHelpers::putOptionalString(data, "notes", config.notes);
    data["is_critical"] = config.isCritical;

    return data;
}

ERP::Integration::DTO::DeviceConfigDTO DeviceConfigDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Integration::DTO::DeviceConfigDTO config;
    ERP::Utils::DTOUtils::fromMap(data, config); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "device_name", config.deviceName);
        ERP::DAOHelpers::getPlainValue(data, "device_identifier", config.deviceIdentifier);
        
        int typeInt;
        ERP::DAOHelpers::getPlainValue(data, "type", typeInt);
        config.type = static_cast<ERP::Integration::DTO::DeviceType>(typeInt);

        ERP::DAOHelpers::getOptionalStringValue(data, "connection_string", config.connectionString);
        ERP::DAOHelpers::getOptionalStringValue(data, "ip_address", config.ipAddress);
        
        int connectionStatusInt;
        ERP::DAOHelpers::getPlainValue(data, "connection_status", connectionStatusInt);
        config.connectionStatus = static_cast<ERP::Integration::DTO::ConnectionStatus>(connectionStatusInt);

        ERP::DAOHelpers::getOptionalStringValue(data, "location_id", config.locationId);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", config.notes);
        ERP::DAOHelpers::getPlainValue(data, "is_critical", config.isCritical);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("DeviceConfigDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "DeviceConfigDAO: Data type mismatch in fromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("DeviceConfigDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "DeviceConfigDAO: Unexpected error in fromMap: " + std::string(e.what()));
    }
    return config;
}

bool DeviceConfigDAO::save(const ERP::Integration::DTO::DeviceConfigDTO& config) {
    return create(config);
}

std::optional<ERP::Integration::DTO::DeviceConfigDTO> DeviceConfigDAO::findById(const std::string& id) {
    return getById(id);
}

bool DeviceConfigDAO::update(const ERP::Integration::DTO::DeviceConfigDTO& config) {
    return DAOBase<ERP::Integration::DTO::DeviceConfigDTO>::update(config);
}

bool DeviceConfigDAO::remove(const std::string& id) {
    return DAOBase<ERP::Integration::DTO::DeviceConfigDTO>::remove(id);
}

std::vector<ERP::Integration::DTO::DeviceConfigDTO> DeviceConfigDAO::findAll() {
    return DAOBase<ERP::Integration::DTO::DeviceConfigDTO>::findAll();
}

std::optional<ERP::Integration::DTO::DeviceConfigDTO> DeviceConfigDAO::getDeviceConfigByIdentifier(const std::string& identifier) {
    std::map<std::string, std::any> filters;
    filters["device_identifier"] = identifier;
    std::vector<ERP::Integration::DTO::DeviceConfigDTO> results = get(filters); // Use templated get
    if (!results.empty()) {
        return results[0];
    }
    return std::nullopt;
}

std::vector<ERP::Integration::DTO::DeviceConfigDTO> DeviceConfigDAO::getDeviceConfigs(const std::map<std::string, std::any>& filters) {
    return get(filters); // Use templated get
}

int DeviceConfigDAO::countDeviceConfigs(const std::map<std::string, std::any>& filters) {
    return count(filters); // Use templated count
}

// --- DeviceEventLog operations ---

std::map<std::string, std::any> DeviceConfigDAO::deviceEventLogToMap(const ERP::Integration::DTO::DeviceEventLogDTO& eventLog) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(eventLog); // BaseDTO fields

    data["device_id"] = eventLog.deviceId;
    data["event_type"] = static_cast<int>(eventLog.eventType);
    data["event_time"] = ERP::Utils::DateUtils::formatDateTime(eventLog.eventTime, ERP::Common::DATETIME_FORMAT);
    data["event_description"] = eventLog.eventDescription;
    data["event_data_json"] = ERP::Utils::DTOUtils::mapToJsonString(eventLog.eventData); // Convert map to JSON string
    ERP::DAOHelpers::putOptionalString(data, "notes", eventLog.notes);

    return data;
}

ERP::Integration::DTO::DeviceEventLogDTO DeviceConfigDAO::deviceEventLogFromMap(const std::map<std::string, std::any>& data) const {
    ERP::Integration::DTO::DeviceEventLogDTO eventLog;
    ERP::Utils::DTOUtils::fromMap(data, eventLog); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "device_id", eventLog.deviceId);
        
        int eventTypeInt;
        ERP::DAOHelpers::getPlainValue(data, "event_type", eventTypeInt);
        eventLog.eventType = static_cast<ERP::Integration::DTO::DeviceEventType>(eventTypeInt);

        ERP::DAOHelpers::getPlainTimeValue(data, "event_time", eventLog.eventTime);
        ERP::DAOHelpers::getPlainValue(data, "event_description", eventLog.eventDescription);
        
        // Convert JSON string to map
        std::string eventDataJsonString;
        ERP::DAOHelpers::getPlainValue(data, "event_data_json", eventDataJsonString);
        eventLog.eventData = ERP::Utils::DTOUtils::jsonStringToMap(eventDataJsonString);

        ERP::DAOHelpers::getOptionalStringValue(data, "notes", eventLog.notes);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("DeviceConfigDAO: deviceEventLogFromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "DeviceConfigDAO: Data type mismatch in deviceEventLogFromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("DeviceConfigDAO: deviceEventLogFromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "DeviceConfigDAO: Unexpected error in deviceEventLogFromMap: " + std::string(e.what()));
    }
    return eventLog;
}

bool DeviceConfigDAO::createDeviceEventLog(const ERP::Integration::DTO::DeviceEventLogDTO& eventLog) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("DeviceConfigDAO::createDeviceEventLog: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }

    std::string sql = "INSERT INTO " + deviceEventLogTableName_ + " (id, device_id, event_type, event_time, event_description, event_data_json, notes, status, created_at, created_by, updated_at, updated_by) VALUES (:id, :device_id, :event_type, :event_time, :event_description, :event_data_json, :notes, :status, :created_at, :created_by, :updated_at, :updated_by);";
    
    std::map<std::string, std::any> params = deviceEventLogToMap(eventLog);
    // Remove updated_at/by as they are not used in create
    params.erase("updated_at");
    params.erase("updated_by");

    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("DeviceConfigDAO::createDeviceEventLog: Failed to create device event log. Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to create device event log.", "Không thể tạo nhật ký sự kiện thiết bị.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}

std::vector<ERP::Integration::DTO::DeviceEventLogDTO> DeviceConfigDAO::getDeviceEventLogsByDeviceId(const std::string& deviceId) {
    std::map<std::string, std::any> filters;
    filters["device_id"] = deviceId;
    return getDeviceEventLogs(filters);
}

std::vector<ERP::Integration::DTO::DeviceEventLogDTO> DeviceConfigDAO::getDeviceEventLogs(const std::map<std::string, std::any>& filters) {
    std::vector<std::map<std::string, std::any>> results = executeQuery(deviceEventLogTableName_, filters);
    std::vector<ERP::Integration::DTO::DeviceEventLogDTO> eventLogs;
    for (const auto& row : results) {
        eventLogs.push_back(deviceEventLogFromMap(row));
    }
    return eventLogs;
}

int DeviceConfigDAO::countDeviceEventLogs(const std::map<std::string, std::any>& filters) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("DeviceConfigDAO::countDeviceEventLogs: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return 0;
    }

    std::string sql = "SELECT COUNT(*) FROM " + deviceEventLogTableName_;
    std::string whereClause = buildWhereClause(filters);
    sql += whereClause;

    std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
    connectionPool_->releaseConnection(conn);

    if (!results.empty() && results[0].count("COUNT(*)")) {
        return std::any_cast<long long>(results[0].at("COUNT(*)")); // SQLite COUNT(*) returns long long
    }
    return 0;
}

bool DeviceConfigDAO::removeDeviceEventLogsByDeviceId(const std::string& deviceId) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("DeviceConfigDAO::removeDeviceEventLogsByDeviceId: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }
    
    std::string sql = "DELETE FROM " + deviceEventLogTableName_ + " WHERE device_id = :device_id;";
    std::map<std::string, std::any> params;
    params["device_id"] = deviceId;

    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("DeviceConfigDAO::removeDeviceEventLogsByDeviceId: Failed to remove device event logs for device_id " + deviceId + ". Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove device event logs.", "Không thể xóa nhật ký sự kiện thiết bị.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}


} // namespace DAOs
} // namespace Integration
} // namespace ERP