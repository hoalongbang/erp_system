// Modules/Catalog/Service/PermissionService.h
#ifndef MODULES_CATALOG_SERVICE_PERMISSIONSERVICE_H
#define MODULES_CATALOG_SERVICE_PERMISSIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Permission.h"       // Đã rút gọn include
#include "PermissionDAO.h"    // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

namespace ERP {
namespace Catalog {
namespace Services {

/**
 * @brief IPermissionService interface defines operations for managing system permissions.
 */
class IPermissionService {
public:
    virtual ~IPermissionService() = default;
    /**
     * @brief Creates a new permission.
     * @param permissionDTO DTO containing new permission information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional PermissionDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::PermissionDTO> createPermission(
        const ERP::Catalog::DTO::PermissionDTO& permissionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves permission information by ID.
     * @param permissionId ID of the permission to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional PermissionDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::PermissionDTO> getPermissionById(
        const std::string& permissionId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves permission information by name.
     * @param permissionName Name of the permission to retrieve (e.g., "Sales.CreateOrder").
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional PermissionDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::PermissionDTO> getPermissionByName(
        const std::string& permissionName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all permissions or permissions matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching PermissionDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::PermissionDTO> getAllPermissions(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates permission information.
     * @param permissionDTO DTO containing updated permission information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updatePermission(
        const ERP::Catalog::DTO::PermissionDTO& permissionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a permission.
     * @param permissionId ID of the permission.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updatePermissionStatus(
        const std::string& permissionId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a permission record by ID (soft delete).
     * @param permissionId ID of the permission to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deletePermission(
        const std::string& permissionId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of IPermissionService.
 * This class uses PermissionDAO and ISecurityManager.
 */
class PermissionService : public IPermissionService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for PermissionService.
     * @param permissionDAO Shared pointer to PermissionDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    PermissionService(std::shared_ptr<DAOs::PermissionDAO> permissionDAO,
                      std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                      std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                      std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                      std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Catalog::DTO::PermissionDTO> createPermission(
        const ERP::Catalog::DTO::PermissionDTO& permissionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Catalog::DTO::PermissionDTO> getPermissionById(
        const std::string& permissionId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Catalog::DTO::PermissionDTO> getPermissionByName(
        const std::string& permissionName,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Catalog::DTO::PermissionDTO> getAllPermissions(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updatePermission(
        const ERP::Catalog::DTO::PermissionDTO& permissionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updatePermissionStatus(
        const std::string& permissionId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deletePermission(
        const std::string& permissionId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::PermissionDAO> permissionDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_
    
    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_PERMISSIONSERVICE_H