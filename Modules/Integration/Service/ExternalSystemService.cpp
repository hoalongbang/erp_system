// Modules/Integration/Service/ExternalSystemService.cpp
#include "ExternalSystemService.h" // Standard includes
#include "IntegrationConfig.h"
#include "APIEndpoint.h"
#include "Event.h"
#include "ConnectionPool.h"
#include "DBConnection.h"
#include "Common.h"
#include "Utils.h"
#include "DateUtils.h"
#include "AutoRelease.h"
#include "ISecurityManager.h"
#include "UserService.h"
#include "DTOUtils.h" // For mapToJsonString
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

// For HTTP client (replacing Qt Network)
#include <cpr/cpr.h> // NEW: Include cpr library
#include <nlohmann/json.hpp> // NEW: For JSON serialization using nlohmann/json

// Removed Qt Network includes
// #include <QNetworkAccessManager>
// #include <QNetworkRequest>
// #include <QUrl>
// #include <QEventLoop>
// #include <QJsonDocument>

namespace ERP {
namespace Integration {
namespace Services {

// Use nlohmann json namespace
using json = nlohmann::json;

ExternalSystemService::ExternalSystemService(
    std::shared_ptr<DAOs::IntegrationConfigDAO> integrationConfigDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
      integrationConfigDAO_(integrationConfigDAO) {
    if (!integrationConfigDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ExternalSystemService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ hệ thống bên ngoài.");
        ERP::Logger::Logger::getInstance().critical("ExternalSystemService: Injected IntegrationConfigDAO is null.");
        throw std::runtime_error("ExternalSystemService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("ExternalSystemService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

// Helper to perform actual external API call (now using cpr)
bool ExternalSystemService::performExternalCall(const ERP::Integration::DTO::APIEndpointDTO& endpoint, const std::map<std::string, std::any>& data) {
    ERP::Logger::Logger::getInstance().info("ExternalSystemService: Performing external call to endpoint " + endpoint.endpointCode + " at URL: " + endpoint.url);

    try {
        std::string payloadString = ERP::Utils::DTOUtils::mapToJsonString(data);
        cpr::Header headers;
        // Add common headers, e.g., Content-Type
        headers["Content-Type"] = "application/json";

        // Add API key or other authentication headers if needed from endpoint.metadata
        if (endpoint.metadata.count("api_key") && endpoint.metadata.at("api_key").type() == typeid(std::string)) {
            headers["X-API-Key"] = std::any_cast<std::string>(endpoint.metadata.at("api_key"));
        }
        // Add Authorization header if endpoint.auth_type is e.g., "Bearer"
        if (endpoint.metadata.count("auth_token") && endpoint.metadata.at("auth_token").type() == typeid(std::string)) {
            headers["Authorization"] = "Bearer " + std::any_cast<std::string>(endpoint.metadata.at("auth_token"));
        }

        cpr::Response r;
        if (endpoint.method == ERP::Integration::DTO::HTTPMethod::POST) {
            r = cpr::Post(cpr::Url{endpoint.url}, cpr::Body{payloadString}, headers);
        } else if (endpoint.method == ERP::Integration::DTO::HTTPMethod::GET) {
            // For GET, you might need to convert data map to URL parameters
            std::string paramsString;
            bool firstParam = true;
            for (const auto& pair : data) {
                if (!firstParam) paramsString += "&";
                paramsString += pair.first + "=";
                // URL-encode value if necessary
                if (pair.second.type() == typeid(std::string)) {
                    paramsString += std::any_cast<std::string>(pair.second); // In reality, use cpr::Parameters for proper encoding
                }
                firstParam = false;
            }
            if (!paramsString.empty()) {
                r = cpr::Get(cpr::Url{endpoint.url + "?" + paramsString}, headers);
            } else {
                r = cpr::Get(cpr::Url{endpoint.url}, headers);
            }
        }
        // Add other HTTP methods (PUT, DELETE, etc.) as needed

        ERP::Logger::Logger::getInstance().info("ExternalSystemService: HTTP Status Code: " + std::to_string(r.status_code) + ", Response: " + r.text);

        if (r.status_code >= 200 && r.status_code < 300) { // Success status codes
            ERP::Logger::Logger::getInstance().info("ExternalSystemService: Data successfully sent to " + endpoint.endpointCode + ".");
            return true;
        } else {
            ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to send data to " + endpoint.endpointCode + ". HTTP Status: " + std::to_string(r.status_code) + ", Error: " + r.error.message);
            ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ExternalSystemService: Failed to send data to external system: " + r.error.message);
            return false;
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ExternalSystemService: Exception during external call to " + endpoint.endpointCode + ": " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ExternalSystemService: Exception during external call: " + std::string(e.what()));
        return false;
    }
}


std::optional<ERP::Integration::DTO::IntegrationConfigDTO> ExternalSystemService::createIntegrationConfig(
    const ERP::Integration::DTO::IntegrationConfigDTO& configDTO,
    const std::vector<ERP::Integration::DTO::APIEndpointDTO>& apiEndpoints,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ExternalSystemService: Attempting to create integration config: " + configDTO.systemName + " (" + configDTO.systemCode + ") by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.CreateIntegrationConfig", "Bạn không có quyền tạo cấu hình tích hợp hệ thống bên ngoài.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (configDTO.systemName.empty() || configDTO.systemCode.empty() || configDTO.type == ERP::Integration::DTO::IntegrationType::UNKNOWN) {
        ERP::Logger::Logger::getInstance().warning("ExternalSystemService: Invalid input for integration config creation (empty name, code, or unknown type).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin cấu hình tích hợp không đầy đủ.");
        return std::nullopt;
    }

    // Check if system code already exists
    std::map<std::string, std::any> filterByCode;
    filterByCode["system_code"] = configDTO.systemCode;
    if (integrationConfigDAO_->count(filterByCode) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("ExternalSystemService: Integration config with code " + configDTO.systemCode + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Mã hệ thống tích hợp đã tồn tại. Vui lòng chọn mã khác.");
        return std::nullopt;
    }

    ERP::Integration::DTO::IntegrationConfigDTO newConfig = configDTO;
    newConfig.id = ERP::Utils::generateUUID();
    newConfig.createdAt = ERP::Utils::DateUtils::now();
    newConfig.createdBy = currentUserId;
    newConfig.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> createdConfig = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!integrationConfigDAO_->create(newConfig)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to create integration config " + newConfig.systemCode + " in DAO.");
                return false;
            }
            // Create API Endpoints
            for (auto endpoint : apiEndpoints) {
                endpoint.id = ERP::Utils::generateUUID();
                if (!integrationConfigDAO_->createAPIEndpoint(endpoint, newConfig.id)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to create API endpoint " + endpoint.endpointCode + " for integration " + newConfig.id + ".");
                    return false;
                }
            }
            createdConfig = newConfig;
            eventBus_.publish(std::make_shared<EventBus::IntegrationConfigCreatedEvent>(newConfig.id, newConfig.systemCode, newConfig.systemName));
            return true;
        },
        "ExternalSystemService", "createIntegrationConfig"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ExternalSystemService: Integration config " + newConfig.systemCode + " created successfully with " + std::to_string(apiEndpoints.size()) + " endpoints.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Integration", "IntegrationConfig", newConfig.id, "IntegrationConfig", newConfig.systemCode,
                       std::nullopt, newConfig.toMap(), "Integration config created.");
        return createdConfig;
    }
    return std::nullopt;
}

std::optional<ERP::Integration::DTO::IntegrationConfigDTO> ExternalSystemService::getIntegrationConfigById(
    const std::string& configId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ExternalSystemService: Retrieving integration config by ID: " + configId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.ViewIntegrationConfigs", "Bạn không có quyền xem cấu hình tích hợp.")) {
        return std::nullopt;
    }

    return integrationConfigDAO_->getById(configId); // Using getById from DAOBase template
}

std::optional<ERP::Integration::DTO::IntegrationConfigDTO> ExternalSystemService::getIntegrationConfigBySystemCode(
    const std::string& systemCode,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ExternalSystemService: Retrieving integration config by system code: " + systemCode + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.ViewIntegrationConfigs", "Bạn không có quyền xem cấu hình tích hợp.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["system_code"] = systemCode;
    std::vector<ERP::Integration::DTO::IntegrationConfigDTO> configs = integrationConfigDAO_->get(filter); // Using get from DAOBase template
    if (!configs.empty()) {
        return configs[0];
    }
    ERP::Logger::Logger::getInstance().debug("ExternalSystemService: Integration config with system code " + systemCode + " not found.");
    return std::nullopt;
}

std::vector<ERP::Integration::DTO::IntegrationConfigDTO> ExternalSystemService::getAllIntegrationConfigs(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ExternalSystemService: Retrieving all integration configs with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.ViewIntegrationConfigs", "Bạn không có quyền xem tất cả cấu hình tích hợp.")) {
        return {};
    }

    return integrationConfigDAO_->get(filter); // Using get from DAOBase template
}

bool ExternalSystemService::updateIntegrationConfig(
    const ERP::Integration::DTO::IntegrationConfigDTO& configDTO,
    const std::vector<ERP::Integration::DTO::APIEndpointDTO>& apiEndpoints,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ExternalSystemService: Attempting to update integration config: " + configDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.UpdateIntegrationConfig", "Bạn không có quyền cập nhật cấu hình tích hợp hệ thống bên ngoài.")) {
        return false;
    }

    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> oldConfigOpt = integrationConfigDAO_->getById(configDTO.id); // Using getById from DAOBase
    if (!oldConfigOpt) {
        ERP::Logger::Logger::getInstance().warning("ExternalSystemService: Integration config with ID " + configDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy cấu hình tích hợp cần cập nhật.");
        return false;
    }

    // If system code is changed, check for uniqueness
    if (configDTO.systemCode != oldConfigOpt->systemCode) {
        std::map<std::string, std::any> filterByCode;
        filterByCode["system_code"] = configDTO.systemCode;
        if (integrationConfigDAO_->count(filterByCode) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("ExternalSystemService: New system code " + configDTO.systemCode + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Mã hệ thống tích hợp mới đã tồn tại. Vui lòng chọn mã khác.");
            return false;
        }
    }

    ERP::Integration::DTO::IntegrationConfigDTO updatedConfig = configDTO;
    updatedConfig.updatedAt = ERP::Utils::DateUtils::now();
    updatedConfig.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!integrationConfigDAO_->update(updatedConfig)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to update integration config " + updatedConfig.id + " in DAO.");
                return false;
            }
            // Update API Endpoints (full replacement strategy)
            if (!integrationConfigDAO_->removeAPIEndpointsByIntegrationConfigId(updatedConfig.id)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to remove old API endpoints for integration " + updatedConfig.id + ".");
                return false;
            }
            for (auto endpoint : apiEndpoints) {
                endpoint.id = ERP::Utils::generateUUID(); // New UUID for new endpoints
                if (!integrationConfigDAO_->createAPIEndpoint(endpoint, updatedConfig.id)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to create new API endpoint " + endpoint.endpointCode + " for integration " + updatedConfig.id + ".");
                    return false;
                }
            }
            eventBus_.publish(std::make_shared<EventBus::IntegrationConfigUpdatedEvent>(updatedConfig.id, updatedConfig.systemCode, updatedConfig.systemName));
            return true;
        },
        "ExternalSystemService", "updateIntegrationConfig"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ExternalSystemService: Integration config " + updatedConfig.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Integration", "IntegrationConfig", updatedConfig.id, "IntegrationConfig", updatedConfig.systemCode,
                       oldConfigOpt->toMap(), updatedConfig.toMap(), "Integration configuration updated.");
        return true;
    }
    return false;
}

bool ExternalSystemService::updateIntegrationConfigStatus(
    const std::string& configId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ExternalSystemService: Attempting to update status for integration config: " + configId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.UpdateIntegrationConfigStatus", "Bạn không có quyền cập nhật trạng thái cấu hình tích hợp.")) {
        return false;
    }

    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> configOpt = integrationConfigDAO_->getById(configId); // Using getById from DAOBase
    if (!configOpt) {
        ERP::Logger::Logger::getInstance().warning("ExternalSystemService: Integration config with ID " + configId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy cấu hình tích hợp để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Integration::DTO::IntegrationConfigDTO oldConfig = *configOpt;
    if (oldConfig.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("ExternalSystemService: Integration config " + configId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Integration::DTO::IntegrationConfigDTO updatedConfig = oldConfig;
    updatedConfig.status = newStatus;
    updatedConfig.updatedAt = ERP::Utils::DateUtils::now();
    updatedConfig.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!integrationConfigDAO_->update(updatedConfig)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to update status for integration config " + configId + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::IntegrationConfigStatusChangedEvent>(configId, newStatus));
            return true;
        },
        "ExternalSystemService", "updateIntegrationConfigStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ExternalSystemService: Status for integration config " + configId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Integration", "IntegrationConfigStatus", configId, "IntegrationConfig", oldConfig.systemCode,
                       oldConfig.toMap(), updatedConfig.toMap(), "Integration config status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool ExternalSystemService::deleteIntegrationConfig(
    const std::string& configId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ExternalSystemService: Attempting to delete integration config: " + configId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.DeleteIntegrationConfig", "Bạn không có quyền xóa cấu hình tích hợp hệ thống bên ngoài.")) {
        return false;
    }

    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> configOpt = integrationConfigDAO_->getById(configId); // Using getById from DAOBase
    if (!configOpt) {
        ERP::Logger::Logger::getInstance().warning("ExternalSystemService: Integration config with ID " + configId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy cấu hình tích hợp cần xóa.");
        return false;
    }

    ERP::Integration::DTO::IntegrationConfigDTO configToDelete = *configOpt;

    // Additional checks: Prevent deletion if integration is currently active or has active endpoints being used.
    if (configToDelete.status == ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("ExternalSystemService: Cannot delete integration config " + configId + " as it is currently active.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa cấu hình tích hợp đang hoạt động.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated API Endpoints first
            if (!integrationConfigDAO_->removeAPIEndpointsByIntegrationConfigId(configId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to remove associated API endpoints for integration " + configId + ".");
                return false;
            }
            if (!integrationConfigDAO_->remove(configId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to delete integration config " + configId + " in DAO.");
                return false;
            }
            return true;
        },
        "ExternalSystemService", "deleteIntegrationConfig"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ExternalSystemService: Integration config " + configId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Integration", "IntegrationConfig", configId, "IntegrationConfig", configToDelete.systemCode,
                       configToDelete.toMap(), std::nullopt, "Integration configuration deleted.");
        return true;
    }
    return false;
}

std::vector<ERP::Integration::DTO::APIEndpointDTO> ExternalSystemService::getAPIEndpointsByIntegrationConfig(
    const std::string& integrationConfigId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ExternalSystemService: Retrieving API endpoints for integration config ID: " + integrationConfigId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.ViewIntegrationConfigs", "Bạn không có quyền xem điểm cuối API tích hợp.")) {
        return {};
    }

    // Check if parent Integration Config exists
    if (!integrationConfigDAO_->getById(integrationConfigId)) {
        ERP::Logger::Logger::getInstance().warning("ExternalSystemService: Parent Integration Config " + integrationConfigId + " not found when getting API endpoints.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Cấu hình tích hợp cha không tồn tại.");
        return {};
    }

    return integrationConfigDAO_->getAPIEndpointsByIntegrationConfigId(integrationConfigId); // Specific DAO method
}

bool ExternalSystemService::sendDataToExternalSystem(
    const std::string& endpointCode,
    const std::map<std::string, std::any>& dataToSend,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ExternalSystemService: Attempting to send data to external system via endpoint: " + endpointCode + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Integration.SendData", "Bạn không có quyền gửi dữ liệu đến hệ thống bên ngoài.")) {
        return false;
    }

    // Retrieve endpoint configuration by code
    std::map<std::string, std::any> filter;
    filter["endpoint_code"] = endpointCode;
    std::vector<ERP::Integration::DTO::APIEndpointDTO> endpoints = integrationConfigDAO_->getAPIEndpoints(filter); // Assuming this method exists
    if (endpoints.empty()) {
        ERP::Logger::Logger::getInstance().warning("ExternalSystemService: API Endpoint with code " + endpointCode + " not found.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Điểm cuối API không tồn tại.");
        return false;
    }
    ERP::Integration::DTO::APIEndpointDTO endpoint = endpoints[0];

    // Validate if endpoint is active
    if (endpoint.status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("ExternalSystemService: API Endpoint " + endpointCode + " is not active.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Điểm cuối API không hoạt động. Không thể gửi dữ liệu.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Perform the actual external call
            if (!performExternalCall(endpoint, dataToSend)) { // Helper method
                ERP::Logger::Logger::getInstance().error("ExternalSystemService: Failed to send data via external call for endpoint " + endpointCode + ".");
                return false;
            }
            // Optionally, log the transaction or status in DB (not just audit log)
            return true;
        },
        "ExternalSystemService", "sendDataToExternalSystem"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ExternalSystemService: Data sent successfully via endpoint: " + endpointCode + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DATA_EXPORT, ERP::Common::LogSeverity::INFO,
                       "Integration", "ExternalSystemDataExchange", endpoint.id, "APIEndpoint", endpoint.endpointCode,
                       std::nullopt, dataToSend, "Data sent to external system via endpoint: " + endpointCode + ".");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Integration
} // namespace ERP