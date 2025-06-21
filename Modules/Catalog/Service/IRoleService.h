// Modules/Catalog/Service/IRoleService.h
#ifndef MODULES_CATALOG_SERVICE_IROLESERVICE_H
#define MODULES_CATALOG_SERVICE_IROLESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <set>    // For std::set<std::string>

// Rút gọn các include paths
#include "Role.h"          // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Catalog {
namespace Services {

/**
 * @brief IRoleService interface defines operations for managing user roles.
 */
class IRoleService {
public:
    virtual ~IRoleService() = default;
    /**
     * @brief Creates a new role.
     * @param roleDTO DTO containing new role information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional RoleDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::RoleDTO> createRole(
        const ERP::Catalog::DTO::RoleDTO& roleDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves role information by ID.
     * @param roleId ID of the role to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional RoleDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::RoleDTO> getRoleById(
        const std::string& roleId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves role information by name.
     * @param roleName Name of the role to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional RoleDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::RoleDTO> getRoleByName(
        const std::string& roleName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all roles or roles matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching RoleDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::RoleDTO> getAllRoles(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates role information.
     * @param roleDTO DTO containing updated role information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateRole(
        const ERP::Catalog::DTO::RoleDTO& roleDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a role.
     * @param roleId ID of the role.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateRoleStatus(
        const std::string& roleId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a role record by ID (soft delete).
     * @param roleId ID of the role to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteRole(
        const std::string& roleId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves permissions assigned to a specific role.
     * @param roleId The ID of the role.
     * @param userRoleIds Roles of the user performing the operation.
     * @return A set of permission names assigned to the role.
     */
    virtual std::set<std::string> getRolePermissions(
        const std::string& roleId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Assigns a permission to a role.
     * @param roleId The ID of the role.
     * @param permissionName The name of the permission to assign.
     * @param currentUserId The ID of the user performing the operation.
     * @param userRoleIds The roles of the user performing the operation.
     * @return True if assignment is successful, false otherwise.
     */
    virtual bool assignPermissionToRole(
        const std::string& roleId,
        const std::string& permissionName,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Removes a permission from a role.
     * @param roleId The ID of the role.
     * @param permissionName The name of the permission to remove.
     * @param currentUserId The ID of the user performing the operation.
     * @param userRoleIds The roles of the user performing the operation.
     * @return True if removal is successful, false otherwise.
     */
    virtual bool removePermissionFromRole(
        const std::string& roleId,
        const std::string& permissionName,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_IROLESERVICE_H