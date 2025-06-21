// Modules/Catalog/Service/IPermissionService.h
#ifndef MODULES_CATALOG_SERVICE_IPERMISSIONSERVICE_H
#define MODULES_CATALOG_SERVICE_IPERMISSIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <set>    // For std::set<std::string>

// Rút gọn các include paths
#include "Permission.h"    // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

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

} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_IPERMISSIONSERVICE_H