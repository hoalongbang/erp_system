// Modules/Integration/Service/ExternalSystemService.h
#ifndef MODULES_INTEGRATION_SERVICE_EXTERNALSYSTEMSERVICE_H
#define MODULES_INTEGRATION_SERVICE_EXTERNALSYSTEMSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "IntegrationConfig.h"  // Đã rút gọn include
#include "APIEndpoint.h"        // Đã rút gọn include
#include "IntegrationConfigDAO.h" // Đã rút gọn include
#include "ISecurityManager.h"   // Đã rút gọn include
#include "EventBus.h"           // Đã rút gọn include
#include "Logger.h"             // Đã rút gọn include
#include "ErrorHandler.h"       // Đã rút gọn include
#include "Common.h"             // Đã rút gọn include
#include "Utils.h"              // Đã rút gọn include
#include "DateUtils.h"          // Đã rút gọn include

namespace ERP {
namespace Integration {
namespace Services {

/**
 * @brief IExternalSystemService interface defines operations for integrating with external systems.
 * This service manages configurations and orchestrates data exchange with external APIs/systems.
 */
class IExternalSystemService {
public:
    virtual ~IExternalSystemService() = default;
    /**
     * @brief Creates a new external system integration configuration.
     * @param configDTO DTO containing new configuration information.
     * @param apiEndpoints Vector of APIEndpointDTOs associated with this system.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional IntegrationConfigDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Integration::DTO::IntegrationConfigDTO> createIntegrationConfig(
        const ERP::Integration::DTO::IntegrationConfigDTO& configDTO,
        const std::vector<ERP::Integration::DTO::APIEndpointDTO>& apiEndpoints,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves external system integration configuration by ID.
     * @param configId ID of the configuration to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional IntegrationConfigDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Integration::DTO::IntegrationConfigDTO> getIntegrationConfigById(
        const std::string& configId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves external system integration configuration by system code.
     * @param systemCode System code to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional IntegrationConfigDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Integration::DTO::IntegrationConfigDTO> getIntegrationConfigBySystemCode(
        const std::string& systemCode,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all external system integration configurations or configs matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching IntegrationConfigDTOs.
     */
    virtual std::vector<ERP::Integration::DTO::IntegrationConfigDTO> getAllIntegrationConfigs(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates external system integration configuration.
     * @param configDTO DTO containing updated configuration information (must have ID).
     * @param apiEndpoints Vector of APIEndpointDTOs associated with this system (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateIntegrationConfig(
        const ERP::Integration::DTO::IntegrationConfigDTO& configDTO,
        const std::vector<ERP::Integration::DTO::APIEndpointDTO>& apiEndpoints,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of an external system integration.
     * @param configId ID of the configuration.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateIntegrationConfigStatus(
        const std::string& configId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes an external system integration configuration record by ID (soft delete).
     * @param configId ID of the configuration to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteIntegrationConfig(
        const std::string& configId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all API endpoints for a specific external system.
     * @param integrationConfigId ID of the integration configuration.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of APIEndpointDTOs.
     */
    virtual std::vector<ERP::Integration::DTO::APIEndpointDTO> getAPIEndpointsByIntegrationConfig(
        const std::string& integrationConfigId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Sends data to an external system through a specified endpoint.
     * This is a generic method; actual implementation depends on endpoint type and data format.
     * @param endpointCode Code of the API endpoint to use.
     * @param dataToSend Data to send (as a map).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if data was sent successfully, false otherwise.
     */
    virtual bool sendDataToExternalSystem(
        const std::string& endpointCode,
        const std::map<std::string, std::any>& dataToSend,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of IExternalSystemService.
 * This class uses IntegrationConfigDAO and ISecurityManager.
 */
class ExternalSystemService : public IExternalSystemService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for ExternalSystemService.
     * @param integrationConfigDAO Shared pointer to IntegrationConfigDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    ExternalSystemService(std::shared_ptr<DAOs::IntegrationConfigDAO> integrationConfigDAO,
                          std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                          std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                          std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                          std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> createIntegrationConfig(
        const ERP::Integration::DTO::IntegrationConfigDTO& configDTO,
        const std::vector<ERP::Integration::DTO::APIEndpointDTO>& apiEndpoints,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> getIntegrationConfigById(
        const std::string& configId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> getIntegrationConfigBySystemCode(
        const std::string& systemCode,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Integration::DTO::IntegrationConfigDTO> getAllIntegrationConfigs(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateIntegrationConfig(
        const ERP::Integration::DTO::IntegrationConfigDTO& configDTO,
        const std::vector<ERP::Integration::DTO::APIEndpointDTO>& apiEndpoints,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateIntegrationConfigStatus(
        const std::string& configId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteIntegrationConfig(
        const std::string& configId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Integration::DTO::APIEndpointDTO> getAPIEndpointsByIntegrationConfig(
        const std::string& integrationConfigId,
        const std::vector<std::string>& userRoleIds = {}) override;
    bool sendDataToExternalSystem(
        const std::string& endpointCode,
        const std::map<std::string, std::any>& dataToSend,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::IntegrationConfigDAO> integrationConfigDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();

    // Helper to perform actual data sending (e.g., HTTP requests)
    // This would likely involve a dedicated HTTPClient or similar utility.
    bool performExternalCall(const ERP::Integration::DTO::APIEndpointDTO& endpoint, const std::map<std::string, std::any>& data);
};
} // namespace Services
} // namespace Integration
} // namespace ERP
#endif // MODULES_INTEGRATION_SERVICE_EXTERNALSYSTEMSERVICE_H