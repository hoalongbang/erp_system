// Modules/Catalog/Service/RoleService.cpp
#include "RoleService.h" // Đã rút gọn include
#include "Role.h" // Đã rút gọn include
#include "Permission.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "PermissionService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Catalog {
namespace Services {

RoleService::RoleService(
    std::shared_ptr<DAOs::RoleDAO> roleDAO,
    std::shared_ptr<IPermissionService> permissionService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      roleDAO_(roleDAO), permissionService_(permissionService) {
    if (!roleDAO_ || !permissionService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "RoleService: Initialized with null DAO or PermissionService.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ vai trò.");
        ERP::Logger::Logger::getInstance().critical("RoleService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("RoleService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("RoleService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Catalog::DTO::RoleDTO> RoleService::createRole(
    const ERP::Catalog::DTO::RoleDTO& roleDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("RoleService: Attempting to create role: " + roleDTO.name + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.CreateRole", "Bạn không có quyền tạo vai trò.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (roleDTO.name.empty()) {
        ERP::Logger::Logger::getInstance().warning("RoleService: Invalid input for role creation (empty name).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "RoleService: Invalid input for role creation.", "Tên vai trò không được để trống.");
        return std::nullopt;
    }

    // Check if role name already exists
    std::map<std::string, std::any> filterByName;
    filterByName["name"] = roleDTO.name;
    if (roleDAO_->count(filterByName) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("RoleService: Role with name " + roleDTO.name + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "RoleService: Role with name " + roleDTO.name + " already exists.", "Tên vai trò đã tồn tại. Vui lòng chọn tên khác.");
        return std::nullopt;
    }

    ERP::Catalog::DTO::RoleDTO newRole = roleDTO;
    newRole.id = ERP::Utils::generateUUID();
    newRole.createdAt = ERP::Utils::DateUtils::now();
    newRole.createdBy = currentUserId;
    newRole.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Catalog::DTO::RoleDTO> createdRole = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!roleDAO_->create(newRole)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("RoleService: Failed to create role " + newRole.name + " in DAO.");
                return false;
            }
            createdRole = newRole;
            eventBus_.publish(std::make_shared<EventBus::RoleCreatedEvent>(newRole.id, newRole.name));
            return true;
        },
        "RoleService", "createRole"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("RoleService: Role " + newRole.name + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Role", newRole.id, "Role", newRole.name,
                       std::nullopt, newRole.toMap(), "Role created.");
        return createdRole;
    }
    return std::nullopt;
}

std::optional<ERP::Catalog::DTO::RoleDTO> RoleService::getRoleById(
    const std::string& roleId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("RoleService: Retrieving role by ID: " + roleId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewRoles", "Bạn không có quyền xem vai trò.")) {
        return std::nullopt;
    }

    return roleDAO_->getById(roleId); // Using getById from DAOBase template
}

std::optional<ERP::Catalog::DTO::RoleDTO> RoleService::getRoleByName(
    const std::string& roleName,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("RoleService: Retrieving role by name: " + roleName + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewRoles", "Bạn không có quyền xem vai trò.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["name"] = roleName;
    std::vector<ERP::Catalog::DTO::RoleDTO> roles = roleDAO_->get(filter); // Using get from DAOBase template
    if (!roles.empty()) {
        return roles[0];
    }
    ERP::Logger::Logger::getInstance().debug("RoleService: Role with name " + roleName + " not found.");
    return std::nullopt;
}

std::vector<ERP::Catalog::DTO::RoleDTO> RoleService::getAllRoles(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("RoleService: Retrieving all roles with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewRoles", "Bạn không có quyền xem tất cả vai trò.")) {
        return {};
    }

    return roleDAO_->get(filter); // Using get from DAOBase template
}

bool RoleService::updateRole(
    const ERP::Catalog::DTO::RoleDTO& roleDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("RoleService: Attempting to update role: " + roleDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.UpdateRole", "Bạn không có quyền cập nhật vai trò.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::RoleDTO> oldRoleOpt = roleDAO_->getById(roleDTO.id); // Using getById from DAOBase
    if (!oldRoleOpt) {
        ERP::Logger::Logger::getInstance().warning("RoleService: Role with ID " + roleDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy vai trò cần cập nhật.");
        return false;
    }

    // If role name is changed, check for uniqueness
    if (roleDTO.name != oldRoleOpt->name) {
        std::map<std::string, std::any> filterByName;
        filterByName["name"] = roleDTO.name;
        if (roleDAO_->count(filterByName) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("RoleService: New role name " + roleDTO.name + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "RoleService: New role name " + roleDTO.name + " already exists.", "Tên vai trò mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }

    ERP::Catalog::DTO::RoleDTO updatedRole = roleDTO;
    updatedRole.updatedAt = ERP::Utils::DateUtils::now();
    updatedRole.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!roleDAO_->update(updatedRole)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("RoleService: Failed to update role " + updatedRole.id + " in DAO.");
                return false;
            }
            // Invalidate authorization cache since role permissions might have changed
            securityManager_->getAuthorizationService()->reloadPermissionCache();
            eventBus_.publish(std::make_shared<EventBus::RoleUpdatedEvent>(updatedRole.id, updatedRole.name));
            return true;
        },
        "RoleService", "updateRole"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("RoleService: Role " + updatedRole.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Role", updatedRole.id, "Role", updatedRole.name,
                       oldRoleOpt->toMap(), updatedRole.toMap(), "Role updated.");
        return true;
    }
    return false;
}

bool RoleService::updateRoleStatus(
    const std::string& roleId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("RoleService: Attempting to update status for role: " + roleId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ChangeRoleStatus", "Bạn không có quyền cập nhật trạng thái vai trò.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::RoleDTO> roleOpt = roleDAO_->getById(roleId); // Using getById from DAOBase
    if (!roleOpt) {
        ERP::Logger::Logger::getInstance().warning("RoleService: Role with ID " + roleId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy vai trò để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Catalog::DTO::RoleDTO oldRole = *roleOpt;
    if (oldRole.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("RoleService: Role " + roleId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Catalog::DTO::RoleDTO updatedRole = oldRole;
    updatedRole.status = newStatus;
    updatedRole.updatedAt = ERP::Utils::DateUtils::now();
    updatedRole.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!roleDAO_->update(updatedRole)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("RoleService: Failed to update status for role " + roleId + " in DAO.");
                return false;
            }
            // Invalidate authorization cache
            securityManager_->getAuthorizationService()->reloadPermissionCache();
            eventBus_.publish(std::make_shared<EventBus::RoleStatusChangedEvent>(roleId, newStatus));
            return true;
        },
        "RoleService", "updateRoleStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("RoleService: Status for role " + roleId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "RoleStatus", roleId, "Role", oldRole.name,
                       oldRole.toMap(), updatedRole.toMap(), "Role status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool RoleService::deleteRole(
    const std::string& roleId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("RoleService: Attempting to delete role: " + roleId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.DeleteRole", "Bạn không có quyền xóa vai trò.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::RoleDTO> roleOpt = roleDAO_->getById(roleId); // Using getById from DAOBase
    if (!roleOpt) {
        ERP::Logger::Logger::getInstance().warning("RoleService: Role with ID " + roleId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy vai trò cần xóa.");
        return false;
    }

    ERP::Catalog::DTO::RoleDTO roleToDelete = *roleOpt;

    // Additional checks: Prevent deletion if role is assigned to any user
    std::map<std::string, std::any> userFilter;
    userFilter["role_id"] = roleId; // Assuming role_id is a direct column in users table or can filter join table
    if (securityManager_->getUserService()->getAllUsers(userFilter).size() > 0) { // Assuming UserService can query users by role
        ERP::Logger::Logger::getInstance().warning("RoleService: Cannot delete role " + roleId + " as it is assigned to users.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa vai trò đang được gán cho người dùng.");
        return false;
    }
    // Also, roles like 'Admin' or 'User' should probably not be deletable for system integrity.

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove all associated permissions first
            if (!roleDAO_->removeRolePermission(roleId, "")) { // Specific DAO method, empty string to remove all
                ERP::Logger::Logger::getInstance().error("RoleService: Failed to remove associated permissions for role " + roleId + ".");
                return false;
            }
            if (!roleDAO_->remove(roleId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("RoleService: Failed to delete role " + roleId + " in DAO.");
                return false;
            }
            // Invalidate authorization cache after successful deletion
            securityManager_->getAuthorizationService()->reloadPermissionCache();
            return true;
        },
        "RoleService", "deleteRole"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("RoleService: Role " + roleId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Role", roleId, "Role", roleToDelete.name,
                       roleToDelete.toMap(), std::nullopt, "Role deleted.");
        return true;
    }
    return false;
}

std::set<std::string> RoleService::getRolePermissions(
    const std::string& roleId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("RoleService: Getting permissions for role ID: " + roleId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewRolePermissions", "Bạn không có quyền xem quyền hạn của vai trò.")) {
        return {};
    }

    // Check if role exists
    if (!roleDAO_->getById(roleId)) { // Use DAOBase's getById
        ERP::Logger::Logger::getInstance().warning("RoleService: Role " + roleId + " not found when getting permissions.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Vai trò không tồn tại.");
        return {};
    }

    // RoleDAO::getRolePermissions is a specific method
    std::vector<std::map<std::string, std::any>> rawPermissions = roleDAO_->getRolePermissions(roleId);
    std::set<std::string> permissions;
    for (const auto& row : rawPermissions) {
        if (row.count("permission_name") && row.at("permission_name").type() == typeid(std::string)) {
            permissions.insert(std::any_cast<std::string>(row.at("permission_name")));
        }
    }
    return permissions;
}

bool RoleService::assignPermissionToRole(
    const std::string& roleId,
    const std::string& permissionName,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("RoleService: Attempting to assign permission " + permissionName + " to role " + roleId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ManageRolePermissions", "Bạn không có quyền quản lý quyền hạn của vai trò.")) {
        return false;
    }

    // Validate role and permission existence
    if (!roleDAO_->getById(roleId)) { // Use DAOBase's getById
        ERP::Logger::Logger::getInstance().warning("RoleService: Role " + roleId + " not found for permission assignment.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Vai trò không tồn tại để gán quyền.");
        return false;
    }
    if (!permissionService_->getPermissionByName(permissionName, userRoleIds)) { // Assuming PermissionService can retrieve by name
        ERP::Logger::Logger::getInstance().warning("RoleService: Permission " + permissionName + " not found for assignment.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Quyền hạn không tồn tại.");
        return false;
    }

    // Check if permission is already assigned
    std::set<std::string> existingPermissions = getRolePermissions(roleId, userRoleIds);
    if (existingPermissions.count(permissionName)) {
        ERP::Logger::Logger::getInstance().warning("RoleService: Permission " + permissionName + " is already assigned to role " + roleId + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Quyền hạn đã được gán cho vai trò này.");
        return true; // Consider it successful as desired state is met
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!roleDAO_->addRolePermission(roleId, permissionName)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("RoleService: Failed to assign permission " + permissionName + " to role " + roleId + " in DAO.");
                return false;
            }
            // Invalidate authorization cache
            securityManager_->getAuthorizationService()->reloadPermissionCache();
            return true;
        },
        "RoleService", "assignPermissionToRole"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("RoleService: Permission " + permissionName + " assigned to role " + roleId + " successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::PERMISSION_CHANGE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "RolePermission", roleId, "Role", roleId, // entityName could be role name
                       std::nullopt, std::nullopt, "Assigned permission: " + permissionName + " to role.");
        return true;
    }
    return false;
}

bool RoleService::removePermissionFromRole(
    const std::string& roleId,
    const std::string& permissionName,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("RoleService: Attempting to remove permission " + permissionName + " from role " + roleId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ManageRolePermissions", "Bạn không có quyền quản lý quyền hạn của vai trò.")) {
        return false;
    }

    // Validate role existence
    if (!roleDAO_->getById(roleId)) { // Use DAOBase's getById
        ERP::Logger::Logger::getInstance().warning("RoleService: Role " + roleId + " not found for permission removal.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Vai trò không tồn tại để gỡ bỏ quyền.");
        return false;
    }
    // Permission validation is optional here, as we are removing.

    // Check if permission is actually assigned
    std::set<std::string> existingPermissions = getRolePermissions(roleId, userRoleIds);
    if (!existingPermissions.count(permissionName)) {
        ERP::Logger::Logger::getInstance().warning("RoleService: Permission " + permissionName + " is not assigned to role " + roleId + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Quyền hạn không được gán cho vai trò này.");
        return true; // Consider it successful as desired state is met
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!roleDAO_->removeRolePermission(roleId, permissionName)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("RoleService: Failed to remove permission " + permissionName + " from role " + roleId + " in DAO.");
                return false;
            }
            // Invalidate authorization cache
            securityManager_->getAuthorizationService()->reloadPermissionCache();
            return true;
        },
        "RoleService", "removePermissionFromRole"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("RoleService: Permission " + permissionName + " removed from role " + roleId + " successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::PERMISSION_CHANGE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "RolePermission", roleId, "Role", roleId, // entityName could be role name
                       std::nullopt, std::nullopt, "Removed permission: " + permissionName + " from role.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Catalog
} // namespace ERP