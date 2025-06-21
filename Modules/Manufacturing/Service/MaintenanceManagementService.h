// Modules/Manufacturing/Service/MaintenanceManagementService.h
#ifndef MODULES_MANUFACTURING_SERVICE_MAINTENANCEMANAGEMENTSERVICE_H
#define MODULES_MANUFACTURING_SERVICE_MAINTENANCEMANAGEMENTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "MaintenanceManagement.h" // Đã rút gọn include
#include "MaintenanceManagementDAO.h" // Đã rút gọn include
#include "AssetManagementService.h" // For Asset validation
#include "ISecurityManager.h"   // Đã rút gọn include
#include "EventBus.h"           // Đã rút gọn include
#include "Logger.h"             // Đã rút gọn include
#include "ErrorHandler.h"       // Đã rút gọn include
#include "Common.h"             // Đã rút gọn include
#include "Utils.h"              // Đã rút gọn include
#include "DateUtils.h"          // Đã rút gọn include

namespace ERP {
namespace Manufacturing {
namespace Services {

// Forward declare if AssetManagementService is only used via pointer/reference
// class IAssetManagementService;

/**
 * @brief IMaintenanceManagementService interface defines operations for managing maintenance requests and activities.
 */
class IMaintenanceManagementService {
public:
    virtual ~IMaintenanceManagementService() = default;
    /**
     * @brief Creates a new maintenance request.
     * @param requestDTO DTO containing new request information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaintenanceRequestDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> createMaintenanceRequest(
        const ERP::Manufacturing::DTO::MaintenanceRequestDTO& requestDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves maintenance request information by ID.
     * @param requestId ID of the request to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaintenanceRequestDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> getMaintenanceRequestById(
        const std::string& requestId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all maintenance requests or requests matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching MaintenanceRequestDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::MaintenanceRequestDTO> getAllMaintenanceRequests(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates maintenance request information.
     * @param requestDTO DTO containing updated request information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateMaintenanceRequest(
        const ERP::Manufacturing::DTO::MaintenanceRequestDTO& requestDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a maintenance request.
     * @param requestId ID of the request.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateMaintenanceRequestStatus(
        const std::string& requestId,
        ERP::Manufacturing::DTO::MaintenanceRequestStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a maintenance request record by ID (soft delete).
     * @param requestId ID of the request to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteMaintenanceRequest(
        const std::string& requestId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records a maintenance activity for a request.
     * @param activityDTO DTO containing new activity information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaintenanceActivityDTO if successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::MaintenanceActivityDTO> recordMaintenanceActivity(
        const ERP::Manufacturing::DTO::MaintenanceActivityDTO& activityDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all maintenance activities for a specific request.
     * @param requestId ID of the maintenance request.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching MaintenanceActivityDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::MaintenanceActivityDTO> getMaintenanceActivitiesByRequest(
        const std::string& requestId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};
/**
 * @brief Default implementation of IMaintenanceManagementService.
 * This class uses MaintenanceManagementDAO and ISecurityManager.
 */
class MaintenanceManagementService : public IMaintenanceManagementService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for MaintenanceManagementService.
     * @param maintenanceDAO Shared pointer to MaintenanceManagementDAO.
     * @param assetManagementService Shared pointer to IAssetManagementService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    MaintenanceManagementService(std::shared_ptr<DAOs::MaintenanceManagementDAO> maintenanceDAO,
                                 std::shared_ptr<ERP::Asset::Services::IAssetManagementService> assetManagementService,
                                 std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                                 std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                                 std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                                 std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> createMaintenanceRequest(
        const ERP::Manufacturing::DTO::MaintenanceRequestDTO& requestDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> getMaintenanceRequestById(
        const std::string& requestId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Manufacturing::DTO::MaintenanceRequestDTO> getAllMaintenanceRequests(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateMaintenanceRequest(
        const ERP::Manufacturing::DTO::MaintenanceRequestDTO& requestDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateMaintenanceRequestStatus(
        const std::string& requestId,
        ERP::Manufacturing::DTO::MaintenanceRequestStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteMaintenanceRequest(
        const std::string& requestId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Manufacturing::DTO::MaintenanceActivityDTO> recordMaintenanceActivity(
        const ERP::Manufacturing::DTO::MaintenanceActivityDTO& activityDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Manufacturing::DTO::MaintenanceActivityDTO> getMaintenanceActivitiesByRequest(
        const std::string& requestId,
        const std::vector<std::string>& userRoleIds = {}) override;

private:
    std::shared_ptr<DAOs::MaintenanceManagementDAO> maintenanceDAO_;
    std::shared_ptr<ERP::Asset::Services::IAssetManagementService> assetManagementService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_SERVICE_MAINTENANCEMANAGEMENTSERVICE_H