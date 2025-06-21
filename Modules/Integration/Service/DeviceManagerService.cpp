// Modules/Integration/Service/DeviceManagerService.cpp
#include "DeviceManagerService.h" // Đã rút gọn include
#include "DeviceConfig.h" // Đã rút gọn include
#include "DeviceEventLog.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Integration {
namespace Services {

DeviceManagerService::DeviceManagerService(
    std::shared_ptr<DAOs::DeviceConfigDAO> deviceConfigDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      deviceConfigDAO_(deviceConfigDAO) {
    if (!deviceConfigDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "DeviceManagerService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ quản lý thiết bị.");
        ERP::Logger::Logger::getInstance().critical("DeviceManagerService: Injected DeviceConfigDAO is null.");
        throw std::runtime_error("DeviceManagerService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("DeviceManagerService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Integration::DTO::DeviceConfigDTO> DeviceManagerService::registerDevice(
    const ERP::Integration::DTO::DeviceConfigDTO& deviceConfigDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DeviceManagerService: Attempting to register device: " + deviceConfigDTO.deviceName + " (" + deviceConfigDTO.deviceIdentifier + ") by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.RegisterDevice", "Bạn không có quyền đăng ký thiết bị.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (deviceConfigDTO.deviceName.empty() || deviceConfigDTO.deviceIdentifier.empty() || deviceConfigDTO.type == ERP::Integration::DTO::DeviceType::UNKNOWN) {
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Invalid input for device registration (empty name, identifier, or unknown type).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "DeviceManagerService: Invalid input for device registration.", "Thông tin thiết bị không đầy đủ.");
        return std::nullopt;
    }

    // Check if device identifier already exists
    std::map<std::string, std::any> filterByIdentifier;
    filterByIdentifier["device_identifier"] = deviceConfigDTO.deviceIdentifier;
    if (deviceConfigDAO_->count(filterByIdentifier) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Device with identifier " + deviceConfigDTO.deviceIdentifier + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "DeviceManagerService: Device with identifier " + deviceConfigDTO.deviceIdentifier + " already exists.", "Mã định danh thiết bị đã tồn tại. Vui lòng chọn mã khác.");
        return std::nullopt;
    }

    ERP::Integration::DTO::DeviceConfigDTO newDeviceConfig = deviceConfigDTO;
    newDeviceConfig.id = ERP::Utils::generateUUID();
    newDeviceConfig.createdAt = ERP::Utils::DateUtils::now();
    newDeviceConfig.createdBy = currentUserId;
    newDeviceConfig.status = ERP::Common::EntityStatus::ACTIVE; // Default status
    newDeviceConfig.connectionStatus = ERP::Integration::DTO::ConnectionStatus::DISCONNECTED; // Default disconnected

    std::optional<ERP::Integration::DTO::DeviceConfigDTO> registeredDevice = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!deviceConfigDAO_->create(newDeviceConfig)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("DeviceManagerService: Failed to register device " + newDeviceConfig.deviceIdentifier + " in DAO.");
                return false;
            }
            registeredDevice = newDeviceConfig;
            eventBus_.publish(std::make_shared<EventBus::DeviceRegisteredEvent>(newDeviceConfig.id, newDeviceConfig.deviceIdentifier, newDeviceConfig.type));
            return true;
        },
        "DeviceManagerService", "registerDevice"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("DeviceManagerService: Device " + newDeviceConfig.deviceIdentifier + " registered successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Integration", "DeviceRegistration", newDeviceConfig.id, "DeviceConfig", newDeviceConfig.deviceIdentifier,
                       std::nullopt, newDeviceConfig.toMap(), "Device registered.");
        return registeredDevice;
    }
    return std::nullopt;
}

std::optional<ERP::Integration::DTO::DeviceConfigDTO> DeviceManagerService::getDeviceConfigById(
    const std::string& deviceId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("DeviceManagerService: Retrieving device config by ID: " + deviceId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.ViewDeviceConfigs", "Bạn không có quyền xem cấu hình thiết bị.")) {
        return std::nullopt;
    }

    return deviceConfigDAO_->getById(deviceId); // Using getById from DAOBase template
}

std::optional<ERP::Integration::DTO::DeviceConfigDTO> DeviceManagerService::getDeviceConfigByIdentifier(
    const std::string& deviceIdentifier,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("DeviceManagerService: Retrieving device config by identifier: " + deviceIdentifier + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.ViewDeviceConfigs", "Bạn không có quyền xem cấu hình thiết bị.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["device_identifier"] = deviceIdentifier;
    std::vector<ERP::Integration::DTO::DeviceConfigDTO> configs = deviceConfigDAO_->get(filter); // Using get from DAOBase template
    if (!configs.empty()) {
        return configs[0];
    }
    ERP::Logger::Logger::getInstance().debug("DeviceManagerService: Device config with identifier " + deviceIdentifier + " not found.");
    return std::nullopt;
}

std::vector<ERP::Integration::DTO::DeviceConfigDTO> DeviceManagerService::getAllDeviceConfigs(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DeviceManagerService: Retrieving all device configs with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.ViewDeviceConfigs", "Bạn không có quyền xem tất cả cấu hình thiết bị.")) {
        return {};
    }

    return deviceConfigDAO_->get(filter); // Using get from DAOBase template
}

bool DeviceManagerService::updateDeviceConfig(
    const ERP::Integration::DTO::DeviceConfigDTO& deviceConfigDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DeviceManagerService: Attempting to update device config: " + deviceConfigDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.UpdateDeviceConfig", "Bạn không có quyền cập nhật cấu hình thiết bị.")) {
        return false;
    }

    std::optional<ERP::Integration::DTO::DeviceConfigDTO> oldDeviceConfigOpt = deviceConfigDAO_->getById(deviceConfigDTO.id); // Using getById from DAOBase
    if (!oldDeviceConfigOpt) {
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Device config with ID " + deviceConfigDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy cấu hình thiết bị cần cập nhật.");
        return false;
    }

    // If device identifier is changed, check for uniqueness
    if (deviceConfigDTO.deviceIdentifier != oldDeviceConfigOpt->deviceIdentifier) {
        std::map<std::string, std::any> filterByIdentifier;
        filterByIdentifier["device_identifier"] = deviceConfigDTO.deviceIdentifier;
        if (deviceConfigDAO_->count(filterByIdentifier) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("DeviceManagerService: New device identifier " + deviceConfigDTO.deviceIdentifier + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "DeviceManagerService: New device identifier " + deviceConfigDTO.deviceIdentifier + " already exists.", "Mã định danh thiết bị mới đã tồn tại. Vui lòng chọn mã khác.");
            return false;
        }
    }

    ERP::Integration::DTO::DeviceConfigDTO updatedDeviceConfig = deviceConfigDTO;
    updatedDeviceConfig.updatedAt = ERP::Utils::DateUtils::now();
    updatedDeviceConfig.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!deviceConfigDAO_->update(updatedDeviceConfig)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("DeviceManagerService: Failed to update device config " + updatedDeviceConfig.id + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::DeviceConfigUpdatedEvent>(updatedDeviceConfig.id, updatedDeviceConfig.deviceIdentifier));
            return true;
        },
        "DeviceManagerService", "updateDeviceConfig"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("DeviceManagerService: Device config " + updatedDeviceConfig.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Integration", "DeviceConfig", updatedDeviceConfig.id, "DeviceConfig", updatedDeviceConfig.deviceIdentifier,
                       oldDeviceConfigOpt->toMap(), updatedDeviceConfig.toMap(), "Device configuration updated.");
        return true;
    }
    return false;
}

bool DeviceManagerService::updateDeviceConnectionStatus(
    const std::string& deviceId,
    ERP::Integration::DTO::ConnectionStatus newStatus,
    const std::optional<std::string>& message,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DeviceManagerService: Attempting to update connection status for device: " + deviceId + " to " + ERP::Integration::DTO::DeviceConfigDTO().getConnectionStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.UpdateDeviceConnectionStatus", "Bạn không có quyền cập nhật trạng thái kết nối thiết bị.")) {
        return false;
    }

    std::optional<ERP::Integration::DTO::DeviceConfigDTO> deviceConfigOpt = deviceConfigDAO_->getById(deviceId); // Using getById from DAOBase
    if (!deviceConfigOpt) {
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Device config with ID " + deviceId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy cấu hình thiết bị để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Integration::DTO::DeviceConfigDTO oldDeviceConfig = *deviceConfigOpt;
    if (oldDeviceConfig.connectionStatus == newStatus) {
        ERP::Logger::Logger::getInstance().info("DeviceManagerService: Device " + deviceId + " is already in connection status " + oldDeviceConfig.getConnectionStatusString() + ".");
        return true; // Already in desired status
    }

    ERP::Integration::DTO::DeviceConfigDTO updatedDeviceConfig = oldDeviceConfig;
    updatedDeviceConfig.connectionStatus = newStatus;
    updatedDeviceConfig.updatedAt = ERP::Utils::DateUtils::now();
    updatedDeviceConfig.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!deviceConfigDAO_->update(updatedDeviceConfig)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("DeviceManagerService: Failed to update connection status for device " + deviceId + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::DeviceConnectionStatusChangedEvent>(deviceId, newStatus, message.value_or("")));
            return true;
        },
        "DeviceManagerService", "updateDeviceConnectionStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("DeviceManagerService: Connection status for device " + deviceId + " updated successfully to " + updatedDeviceConfig.getConnectionStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Integration", "DeviceConnectionStatus", deviceId, "DeviceConfig", oldDeviceConfig.deviceIdentifier,
                       oldDeviceConfig.toMap(), updatedDeviceConfig.toMap(), "Device connection status changed to " + updatedDeviceConfig.getConnectionStatusString() + ". Message: " + message.value_or("N/A") + ".");
        return true;
    }
    return false;
}

bool DeviceManagerService::deleteDeviceConfig(
    const std::string& deviceId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DeviceManagerService: Attempting to delete device config: " + deviceId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.DeleteDeviceConfig", "Bạn không có quyền xóa cấu hình thiết bị.")) {
        return false;
    }

    std::optional<ERP::Integration::DTO::DeviceConfigDTO> deviceConfigOpt = deviceConfigDAO_->getById(deviceId); // Using getById from DAOBase
    if (!deviceConfigOpt) {
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Device config with ID " + deviceId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy cấu hình thiết bị cần xóa.");
        return false;
    }

    ERP::Integration::DTO::DeviceConfigDTO deviceConfigToDelete = *deviceConfigOpt;

    // Additional checks: Prevent deletion if device is currently active or has active event logs.
    if (deviceConfigToDelete.connectionStatus == ERP::Integration::DTO::ConnectionStatus::CONNECTED) {
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Cannot delete device config " + deviceId + " as device is currently connected.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa cấu hình thiết bị đang kết nối.");
        return false;
    }
    // Check for active event logs (if they block deletion)
    std::map<std::string, std::any> eventLogFilter;
    eventLogFilter["device_id"] = deviceId;
    if (deviceConfigDAO_->countDeviceEventLogs(eventLogFilter) > 0) { // Specific DAO method or check if device events are standalone
        // Assuming DeviceConfigDAO also handles logs implicitly or has a method to check.
        // If not, would need a separate DAO/Service for DeviceEventLog.
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Cannot delete device config " + deviceId + " as it has associated event logs.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa cấu hình thiết bị có nhật ký sự kiện liên quan.");
        return false;
    }


    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated event logs first if they are children and cascade delete is not set
            if (!deviceConfigDAO_->removeDeviceEventLogsByDeviceId(deviceId)) { // Specific DAO method (assuming it exists or can be added)
                ERP::Logger::Logger::getInstance().error("DeviceManagerService: Failed to remove associated device event logs for device " + deviceId + ".");
                return false;
            }
            if (!deviceConfigDAO_->remove(deviceId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("DeviceManagerService: Failed to delete device config " + deviceId + " in DAO.");
                return false;
            }
            return true;
        },
        "DeviceManagerService", "deleteDeviceConfig"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("DeviceManagerService: Device config " + deviceId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Integration", "DeviceConfig", deviceId, "DeviceConfig", deviceConfigToDelete.deviceIdentifier,
                       deviceConfigToDelete.toMap(), std::nullopt, "Device configuration deleted.");
        return true;
    }
    return false;
}

bool DeviceManagerService::recordDeviceEvent(
    const ERP::Integration::DTO::DeviceEventLogDTO& eventLogDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DeviceManagerService: Attempting to record device event for device: " + eventLogDTO.deviceId + " type: " + ERP::Integration::DTO::DeviceEventLogDTO().getEventTypeString() + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.RecordDeviceEvent", "Bạn không có quyền ghi nhật ký sự kiện thiết bị.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (eventLogDTO.deviceId.empty() || eventLogDTO.eventDescription.empty()) {
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Invalid input for device event recording (missing deviceId or description).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin nhật ký sự kiện thiết bị không đầy đủ.");
        return std::nullopt;
    }

    // Validate Device existence
    if (!getDeviceConfigById(eventLogDTO.deviceId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Device " + eventLogDTO.deviceId + " not found for event logging.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Thiết bị không tồn tại.");
        return false;
    }

    ERP::Integration::DTO::DeviceEventLogDTO newEventLog = eventLogDTO;
    newEventLog.id = ERP::Utils::generateUUID();
    newEventLog.createdAt = ERP::Utils::DateUtils::now();
    newEventLog.createdBy = currentUserId; // User who recorded, or system
    newEventLog.eventTime = newEventLog.createdAt; // Set event time
    newEventLog.status = ERP::Common::EntityStatus::ACTIVE;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!deviceConfigDAO_->createDeviceEventLog(newEventLog)) { // Specific DAO method (assuming it exists or can be added)
                ERP::Logger::Logger::getInstance().error("DeviceManagerService: Failed to create device event log for device " + newEventLog.deviceId + " in DAO.");
                return false;
            }
            // Optionally, publish event (e.g., for real-time monitoring)
            // eventBus_.publish(std::make_shared<EventBus::DeviceEventRecordedEvent>(newEventLog));
            return true;
        },
        "DeviceManagerService", "recordDeviceEvent"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("DeviceManagerService: Device event recorded successfully for device: " + newEventLog.deviceId + " (Type: " + newEventLog.getEventTypeString() + ").");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::PROCESS_END, ERP::Common::LogSeverity::INFO, // Could be specialized AuditActionType for device event
                       "Integration", "DeviceEventLog", newEventLog.id, "DeviceEventLog", newEventLog.deviceId,
                       std::nullopt, newEventLog.toMap(), "Device event recorded: " + newEventLog.getEventTypeString() + ".");
        return true;
    }
    return false;
}

std::vector<ERP::Integration::DTO::DeviceEventLogDTO> DeviceManagerService::getDeviceEventLogsByDevice(
    const std::string& deviceId,
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DeviceManagerService: Retrieving device event logs for device ID: " + deviceId + " with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.ViewDeviceEventLogs", "Bạn không có quyền xem nhật ký sự kiện thiết bị.")) {
        return {};
    }

    // Validate Device existence
    if (!getDeviceConfigById(deviceId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("DeviceManagerService: Device " + deviceId + " not found when getting event logs.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Thiết bị không tồn tại.");
        return {};
    }

    std::map<std::string, std::any> finalFilter = filter;
    finalFilter["device_id"] = deviceId; // Always filter by deviceId

    return deviceConfigDAO_->getDeviceEventLogs(finalFilter); // Specific DAO method (assuming it exists or can be added)
}

} // namespace Services
} // namespace Integration
} // namespace ERP