// Modules/Config/Service/ConfigService.h
#ifndef MODULES_CONFIG_SERVICE_CONFIGSERVICE_H
#define MODULES_CONFIG_SERVICE_CONFIGSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Config.h"           // Đã rút gọn include
#include "ConfigDAO.h"        // Đã rút gọn include
#include "EncryptionService.h" // For encryption/decryption of config values
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

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
/**
 * @brief Default implementation of IConfigService.
 * This class uses ConfigDAO and ISecurityManager, and caches config values for performance.
 */
class ConfigService : public IConfigService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for ConfigService.
     * @param configDAO Shared pointer to ConfigDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    ConfigService(std::shared_ptr<DAOs::ConfigDAO> configDAO,
                  std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                  std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                  std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                  std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Config::DTO::ConfigDTO> getConfig(
        const std::string& configKey,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds,
        bool decrypt = true) override;
    std::vector<ERP::Config::DTO::ConfigDTO> getAllConfigs(
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds,
        bool decrypt = true) override;
    bool createConfig(
        const ERP::Config::DTO::ConfigDTO& configDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateConfig(
        const ERP::Config::DTO::ConfigDTO& configDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteConfig(
        const std::string& configId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    void reloadConfigCache() override;

private:
    std::shared_ptr<DAOs::ConfigDAO> configDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Cache for config values: configKey -> ConfigDTO
    static std::map<std::string, ERP::Config::DTO::ConfigDTO> s_configCache;
    static std::mutex s_cacheMutex; // Mutex for thread-safe access to cache

    /**
     * @brief Loads all active configuration settings into the cache from the database.
     */
    void loadAllConfigsToCache();
    
    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Config
} // namespace ERP
#endif // MODULES_CONFIG_SERVICE_CONFIGSERVICE_H