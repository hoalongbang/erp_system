// Modules/Config/Service/ConfigService.cpp
#include "ConfigService.h" // Đã rút gọn include
#include "Config.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "EncryptionService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Config {
namespace Services {

// Initialize static members
std::map<std::string, ERP::Config::DTO::ConfigDTO> ConfigService::s_configCache;
std::mutex ConfigService::s_cacheMutex;

ConfigService::ConfigService(
    std::shared_ptr<DAOs::ConfigDAO> configDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      configDAO_(configDAO) {
    if (!configDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ConfigService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ cấu hình.");
        ERP::Logger::Logger::getInstance().critical("ConfigService: Injected ConfigDAO is null.");
        throw std::runtime_error("ConfigService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("ConfigService: Initialized. Loading configs to cache...");
    loadAllConfigsToCache(); // Load configs on startup
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

void ConfigService::loadAllConfigsToCache() {
    std::lock_guard<std::mutex> lock(s_cacheMutex); // Protect cache access
    s_configCache.clear();
    ERP::Logger::Logger::getInstance().info("ConfigService: Loading all active configurations to cache.");

    std::map<std::string, std::any> filter;
    filter["status"] = static_cast<int>(ERP::Common::EntityStatus::ACTIVE); // Only load active configs
    std::vector<ERP::Config::DTO::ConfigDTO> allConfigs = configDAO_->get(filter); // Using get from DAOBase

    for (auto& config : allConfigs) {
        // Decrypt values during loading for cache. This assumes the cache will hold decrypted values.
        // If sensitive, store encrypted in cache and decrypt on getConfig() call.
        // For simplicity, decrypting during loading.
        if (config.isEncrypted && !config.configValue.empty()) {
            try {
                config.configValue = securityManager_->getEncryptionService().decrypt(config.configValue);
            } catch (const std::exception& e) {
                ERP::Logger::Logger::getInstance().error("ConfigService: Failed to decrypt config '" + config.configKey + "' during cache load: " + e.what());
                config.configValue = ""; // Clear value if decryption fails
            }
        }
        s_configCache[config.configKey] = config;
    }
    ERP::Logger::Logger::getInstance().info("ConfigService: Loaded " + std::to_string(s_configCache.size()) + " active configurations into cache.");
}

std::optional<ERP::Config::DTO::ConfigDTO> ConfigService::getConfig(
    const std::string& configKey,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds,
    bool decrypt) {
    ERP::Logger::Logger::getInstance().debug("ConfigService: Retrieving config by key: " + configKey + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Config.ViewConfig", "Bạn không có quyền xem cấu hình.")) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(s_cacheMutex); // Protect cache access
    if (s_configCache.count(configKey)) {
        ERP::Config::DTO::ConfigDTO config = s_configCache.at(configKey);
        // If config is sensitive and stored encrypted in cache, decrypt here
        // If already decrypted during loadAllConfigsToCache, no further decryption needed.
        ERP::Logger::Logger::getInstance().debug("ConfigService: Config '" + configKey + "' found in cache.");
        return config;
    }

    ERP::Logger::Logger::getInstance().warning("ConfigService: Config with key " + configKey + " not found in cache. Attempting DB lookup.");
    std::map<std::string, std::any> filter;
    filter["config_key"] = configKey;
    std::vector<ERP::Config::DTO::ConfigDTO> configs = configDAO_->get(filter); // Using get from DAOBase
    if (!configs.empty()) {
        ERP::Config::DTO::ConfigDTO config = configs[0];
        // Decrypt if requested and encrypted
        if (decrypt && config.isEncrypted && !config.configValue.empty()) {
            try {
                config.configValue = securityManager_->getEncryptionService().decrypt(config.configValue);
            } catch (const std::exception& e) {
                ERP::Logger::Logger::getInstance().error("ConfigService: Failed to decrypt config '" + config.configKey + "' from DB: " + e.what());
                config.configValue = ""; // Clear value if decryption fails
            }
        }
        // Add to cache
        s_configCache[config.configKey] = config;
        return config;
    }
    ERP::Logger::Logger::getInstance().warning("ConfigService: Config with key " + configKey + " not found in DB.");
    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Cấu hình không tồn tại.");
    return std::nullopt;
}

std::vector<ERP::Config::DTO::ConfigDTO> ConfigService::getAllConfigs(
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds,
    bool decrypt) {
    ERP::Logger::Logger::getInstance().info("ConfigService: Retrieving all configurations.");

    if (!checkPermission(currentUserId, userRoleIds, "Config.ViewConfig", "Bạn không có quyền xem tất cả cấu hình.")) {
        return {};
    }

    std::lock_guard<std::mutex> lock(s_cacheMutex); // Protect cache access
    std::vector<ERP::Config::DTO::ConfigDTO> allCachedConfigs;
    for (const auto& pair : s_configCache) {
        allCachedConfigs.push_back(pair.second);
    }
    // Note: If 'decrypt' is false, but values are already decrypted in cache, this will return decrypted values.
    // If you need truly encrypted values from this call when decrypt=false, cache should store encrypted values.
    ERP::Logger::Logger::getInstance().info("ConfigService: Retrieved " + std::to_string(allCachedConfigs.size()) + " configurations from cache.");
    return allCachedConfigs;
}

bool ConfigService::createConfig(
    const ERP::Config::DTO::ConfigDTO& configDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ConfigService: Attempting to create config: " + configDTO.configKey + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Config.CreateConfig", "Bạn không có quyền tạo cấu hình.")) {
        return false;
    }

    // 1. Validate input DTO
    if (configDTO.configKey.empty() || configDTO.configValue.empty()) {
        ERP::Logger::Logger::getInstance().warning("ConfigService: Invalid input for config creation (empty key or value).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ConfigService: Invalid input for config creation.", "Khóa hoặc giá trị cấu hình không được để trống.");
        return false;
    }

    // Check if config key already exists
    std::map<std::string, std::any> filterByKey;
    filterByKey["config_key"] = configDTO.configKey;
    if (configDAO_->count(filterByKey) > 0) { // Using count from DAOBase
        ERP::Logger::Logger::getInstance().warning("ConfigService: Config with key " + configDTO.configKey + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ConfigService: Config with key " + configDTO.configKey + " already exists.", "Khóa cấu hình đã tồn tại. Vui lòng chọn khóa khác.");
        return false;
    }

    ERP::Config::DTO::ConfigDTO newConfig = configDTO;
    newConfig.id = ERP::Utils::generateUUID();
    newConfig.createdAt = ERP::Utils::DateUtils::now();
    newConfig.createdBy = currentUserId;
    newConfig.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    // Encrypt sensitive config values before saving
    if (newConfig.isEncrypted && !newConfig.configValue.empty()) {
        try {
            newConfig.configValue = securityManager_->getEncryptionService().encrypt(newConfig.configValue);
        } catch (const std::exception& e) {
            ERP::Logger::Logger::getInstance().error("ConfigService: Failed to encrypt config value for " + newConfig.configKey + ": " + e.what());
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::EncryptionError, "Không thể mã hóa giá trị cấu hình.");
            return false;
        }
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!configDAO_->create(newConfig)) { // Using create from DAOBase
                ERP::Logger::Logger::getInstance().error("ConfigService: Failed to create config " + newConfig.configKey + " in DAO.");
                return false;
            }
            return true;
        },
        "ConfigService", "createConfig"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ConfigService: Config " + newConfig.configKey + " created successfully.");
        // Reload cache after successful creation
        reloadConfigCache();
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CONFIGURATION_CHANGE, ERP::Common::LogSeverity::INFO,
                       "Config", "Config", newConfig.id, "Config", newConfig.configKey,
                       std::nullopt, newConfig.toMap(), "Configuration created.");
        return true;
    }
    return false;
}

bool ConfigService::updateConfig(
    const ERP::Config::DTO::ConfigDTO& configDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ConfigService: Attempting to update config: " + configDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Config.UpdateConfig", "Bạn không có quyền cập nhật cấu hình.")) {
        return false;
    }

    std::optional<ERP::Config::DTO::ConfigDTO> oldConfigOpt = configDAO_->getById(configDTO.id); // Using getById from DAOBase
    if (!oldConfigOpt) {
        ERP::Logger::Logger::getInstance().warning("ConfigService: Config with ID " + configDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy cấu hình cần cập nhật.");
        return false;
    }

    // If config key is changed, check for uniqueness
    if (configDTO.configKey != oldConfigOpt->configKey) {
        std::map<std::string, std::any> filterByKey;
        filterByKey["config_key"] = configDTO.configKey;
        if (configDAO_->count(filterByKey) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("ConfigService: New config key " + configDTO.configKey + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ConfigService: New config key " + configDTO.configKey + " already exists.", "Khóa cấu hình mới đã tồn tại. Vui lòng chọn khóa khác.");
            return false;
        }
    }

    ERP::Config::DTO::ConfigDTO updatedConfig = configDTO;
    updatedConfig.updatedAt = ERP::Utils::DateUtils::now();
    updatedConfig.updatedBy = currentUserId;

    // Encrypt sensitive config values before saving, if they are new/changed and marked encrypted
    if (updatedConfig.isEncrypted && updatedConfig.configValue != oldConfigOpt->configValue && !updatedConfig.configValue.empty()) {
        try {
            updatedConfig.configValue = securityManager_->getEncryptionService().encrypt(updatedConfig.configValue);
        } catch (const std::exception& e) {
            ERP::Logger::Logger::getInstance().error("ConfigService: Failed to encrypt updated config value for " + updatedConfig.configKey + ": " + e.what());
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::EncryptionError, "Không thể mã hóa giá trị cấu hình.");
            return false;
        }
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!configDAO_->update(updatedConfig)) { // Using update from DAOBase
                ERP::Logger::Logger::getInstance().error("ConfigService: Failed to update config " + updatedConfig.id + " in DAO.");
                return false;
            }
            return true;
        },
        "ConfigService", "updateConfig"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ConfigService: Config " + updatedConfig.id + " updated successfully.");
        // Reload cache after successful update
        reloadConfigCache();
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CONFIGURATION_CHANGE, ERP::Common::LogSeverity::INFO,
                       "Config", "Config", updatedConfig.id, "Config", updatedConfig.configKey,
                       oldConfigOpt->toMap(), updatedConfig.toMap(), "Configuration updated.");
        return true;
    }
    return false;
}

bool ConfigService::deleteConfig(
    const std::string& configId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ConfigService: Attempting to delete config: " + configId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Config.DeleteConfig", "Bạn không có quyền xóa cấu hình.")) {
        return false;
    }

    std::optional<ERP::Config::DTO::ConfigDTO> configOpt = configDAO_->getById(configId); // Using getById from DAOBase
    if (!configOpt) {
        ERP::Logger::Logger::getInstance().warning("ConfigService: Config with ID " + configId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy cấu hình cần xóa.");
        return false;
    }

    ERP::Config::DTO::ConfigDTO configToDelete = *configOpt;

    // Additional checks: Prevent deletion of critical system configs (e.g., database connection strings)
    // This requires a list of protected config keys or a flag in ConfigDTO.
    // For demo, assume all are deletable.

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!configDAO_->remove(configId)) { // Using remove from DAOBase
                ERP::Logger::Logger::getInstance().error("ConfigService: Failed to delete config " + configId + " in DAO.");
                return false;
            }
            return true;
        },
        "ConfigService", "deleteConfig"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ConfigService: Config " + configId + " deleted successfully.");
        // Reload cache after successful deletion
        reloadConfigCache();
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CONFIGURATION_CHANGE, ERP::Common::LogSeverity::INFO,
                       "Config", "Config", configId, "Config", configToDelete.configKey,
                       configToDelete.toMap(), std::nullopt, "Configuration deleted.");
        return true;
    }
    return false;
}

void ConfigService::reloadConfigCache() {
    ERP::Logger::Logger::getInstance().info("ConfigService: Reloading config cache.");
    loadAllConfigsToCache();
    // Invalidate any services that might be holding old config values
    eventBus_.publish(std::make_shared<EventBus::ConfigReloadedEvent>());
}

} // namespace Services
} // namespace Config
} // namespace ERP