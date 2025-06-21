// Modules/Config/Service/IConfigService.h
#ifndef MODULES_CONFIG_SERVICE_ICONFIGSERVICE_H
#define MODULES_CONFIG_SERVICE_ICONFIGSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "Config.h"        // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Config {
namespace Services {

/**
 * @brief IConfigService interface defines operations for managing application configurations.
 */
class IConfigService {
public:
    virtual ~IConfigService() = default;
    /**
     * @brief Gets a configuration value by its key.
     * @param configKey The unique key of the configuration setting.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @param decrypt If true, decrypts the value if it's marked as encrypted.
     * @return An optional ConfigDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Config::DTO::ConfigDTO> getConfig(
        const std::string& configKey,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds,
        bool decrypt = true) = 0;
    /**
     * @brief Gets all configuration settings.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @param decrypt If true, decrypts values marked as encrypted.
     * @return Vector of ConfigDTOs.
     */
    virtual std::vector<ERP::Config::DTO::ConfigDTO> getAllConfigs(
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds,
        bool decrypt = true) = 0;
    /**
     * @brief Creates a new configuration setting.
     * @param configDTO DTO containing new configuration information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if creation is successful, false otherwise.
     */
    virtual bool createConfig(
        const ERP::Config::DTO::ConfigDTO& configDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates an existing configuration setting.
     * @param configDTO DTO containing updated configuration information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateConfig(
        const ERP::Config::DTO::ConfigDTO& configDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a configuration setting by ID.
     * @param configId ID of the configuration to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteConfig(
        const std::string& configId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Reloads the configuration cache.
     * This should be called when config values are updated directly in DB or by other means.
     */
    virtual void reloadConfigCache() = 0;
};

} // namespace Services
} // namespace Config
} // namespace ERP
#endif // MODULES_CONFIG_SERVICE_ICONFIGSERVICE_H