// Modules/Integration/Service/IDeviceManagerService.h
#ifndef MODULES_INTEGRATION_SERVICE_IDeviceManagerService_H
#define MODULES_INTEGRATION_SERVICE_IDeviceManagerService_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "DeviceConfig.h"   // DTO
#include "DeviceEventLog.h" // DTO
#include "Common.h"         // Enum Common
#include "BaseService.h"    // Base Service

namespace ERP {
namespace Integration {
namespace Services {

/**
 * @brief IDeviceManagerService interface defines operations for managing connected devices.
 */
class IDeviceManagerService {
public:
    virtual ~IDeviceManagerService() = default;
    /**
     * @brief Registers a new device configuration.
     * @param deviceConfigDTO DTO containing new device configuration information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional DeviceConfigDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Integration::DTO::DeviceConfigDTO> registerDevice(
        const ERP::Integration::DTO::DeviceConfigDTO& deviceConfigDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves device configuration by ID.
     * @param deviceId ID of the device to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional DeviceConfigDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Integration::DTO::DeviceConfigDTO> getDeviceConfigById(
        const std::string& deviceId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves device configuration by device identifier (e.g., serial number).
     * @param deviceIdentifier The unique identifier of the device.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional DeviceConfigDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Integration::DTO::DeviceConfigDTO> getDeviceConfigByIdentifier(
        const std::string& deviceIdentifier,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all device configurations or configs matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching DeviceConfigDTOs.
     */
    virtual std::vector<ERP::Integration::DTO::DeviceConfigDTO> getAllDeviceConfigs(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates device configuration.
     * @param deviceConfigDTO DTO containing updated configuration information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateDeviceConfig(
        const ERP::Integration::DTO::DeviceConfigDTO& deviceConfigDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the connection status of a device.
     * @param deviceId ID of the device.
     * @param newStatus New connection status.
     * @param message Optional message about the status change.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateDeviceConnectionStatus(
        const std::string& deviceId,
        ERP::Integration::DTO::ConnectionStatus newStatus,
        const std::optional<std::string>& message,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a device configuration record by ID (soft delete).
     * @param deviceId ID of the device to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteDeviceConfig(
        const std::string& deviceId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records a device event log.
     * This method is called by device integrations to log events.
     * @param eventLogDTO DTO containing new event log information.
     * @param currentUserId ID of the user performing the operation (or system user).
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if successful, false otherwise.
     */
    virtual bool recordDeviceEvent(
        const ERP::Integration::DTO::DeviceEventLogDTO& eventLogDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves device event logs for a specific device.
     * @param deviceId ID of the device.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching DeviceEventLogDTOs.
     */
    virtual std::vector<ERP::Integration::DTO::DeviceEventLogDTO> getDeviceEventLogsByDevice(
        const std::string& deviceId,
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
};

} // namespace Services
} // namespace Integration
} // namespace ERP
#endif // MODULES_INTEGRATION_SERVICE_IDeviceManagerService_H