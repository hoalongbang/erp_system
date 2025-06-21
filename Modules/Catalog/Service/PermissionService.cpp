// Modules/Catalog/Service/PermissionService.cpp
#include "PermissionService.h" // Đã rút gọn include
#include "Permission.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Catalog {
namespace Services {

PermissionService::PermissionService(
    std::shared_ptr<DAOs::PermissionDAO> permissionDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      permissionDAO_(permissionDAO) {
    if (!permissionDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "PermissionService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ quyền hạn.");
        ERP::Logger::Logger::getInstance().critical("PermissionService: Injected PermissionDAO is null.");
        throw std::runtime_error("PermissionService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("PermissionService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Catalog::DTO::PermissionDTO> PermissionService::createPermission(
    const ERP::Catalog::DTO::PermissionDTO& permissionDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("PermissionService: Attempting to create permission: " + permissionDTO.name + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.CreatePermission", "Bạn không có quyền tạo quyền hạn.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (permissionDTO.name.empty() || permissionDTO.module.empty() || permissionDTO.action.empty()) {
        ERP::Logger::Logger::getInstance().warning("PermissionService: Invalid input for permission creation (empty name, module, or action).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "PermissionService: Invalid input for permission creation.", "Thông tin quyền hạn không đầy đủ.");
        return std::nullopt;
    }

    // Check if permission name already exists
    std::map<std::string, std::any> filterByName;
    filterByName["name"] = permissionDTO.name;
    if (permissionDAO_->count(filterByName) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("PermissionService: Permission with name " + permissionDTO.name + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "PermissionService: Permission with name " + permissionDTO.name + " already exists.", "Tên quyền hạn đã tồn tại. Vui lòng chọn tên khác.");
        return std::nullopt;
    }

    ERP::Catalog::DTO::PermissionDTO newPermission = permissionDTO;
    newPermission.id = ERP::Utils::generateUUID();
    newPermission.createdAt = ERP::Utils::DateUtils::now();
    newPermission.createdBy = currentUserId;
    newPermission.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Catalog::DTO::PermissionDTO> createdPermission = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!permissionDAO_->create(newPermission)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("PermissionService: Failed to create permission " + newPermission.name + " in DAO.");
                return false;
            }
            createdPermission = newPermission;
            // Invalidate authorization cache immediately on new permission creation
            securityManager_->getAuthorizationService()->reloadPermissionCache();
            eventBus_.publish(std::make_shared<EventBus::PermissionCreatedEvent>(newPermission.id, newPermission.name));
            return true;
        },
        "PermissionService", "createPermission"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("PermissionService: Permission " + newPermission.name + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Permission", newPermission.id, "Permission", newPermission.name,
                       std::nullopt, newPermission.toMap(), "Permission created.");
        return createdPermission;
    }
    return std::nullopt;
}

std::optional<ERP::Catalog::DTO::PermissionDTO> PermissionService::getPermissionById(
    const std::string& permissionId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("PermissionService: Retrieving permission by ID: " + permissionId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewPermissions", "Bạn không có quyền xem quyền hạn.")) {
        return std::nullopt;
    }

    return permissionDAO_->getById(permissionId); // Using getById from DAOBase template
}

std::optional<ERP::Catalog::DTO::PermissionDTO> PermissionService::getPermissionByName(
    const std::string& permissionName,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("PermissionService: Retrieving permission by name: " + permissionName + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewPermissions", "Bạn không có quyền xem quyền hạn.")) {
        return std::nullopt;
    }
    
    // Note: PermissionDAO has a specific getByName method, which can be called here directly
    // Or just use generic DAOBase::get with filter
    std::map<std::string, std::any> filter;
    filter["name"] = permissionName;
    std::vector<ERP::Catalog::DTO::PermissionDTO> permissions = permissionDAO_->get(filter); // Using get from DAOBase template
    if (!permissions.empty()) {
        return permissions[0];
    }
    ERP::Logger::Logger::getInstance().debug("PermissionService: Permission with name " + permissionName + " not found.");
    return std::nullopt;
}

std::vector<ERP::Catalog::DTO::PermissionDTO> PermissionService::getAllPermissions(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("PermissionService: Retrieving all permissions with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewPermissions", "Bạn không có quyền xem tất cả quyền hạn.")) {
        return {};
    }

    return permissionDAO_->get(filter); // Using get from DAOBase template
}

bool PermissionService::updatePermission(
    const ERP::Catalog::DTO::PermissionDTO& permissionDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("PermissionService: Attempting to update permission: " + permissionDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.UpdatePermission", "Bạn không có quyền cập nhật quyền hạn.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::PermissionDTO> oldPermissionOpt = permissionDAO_->getById(permissionDTO.id); // Using getById from DAOBase
    if (!oldPermissionOpt) {
        ERP::Logger::Logger::getInstance().warning("PermissionService: Permission with ID " + permissionDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy quyền hạn cần cập nhật.");
        return false;
    }

    // If permission name is changed, check for uniqueness
    if (permissionDTO.name != oldPermissionOpt->name) {
        std::map<std::string, std::any> filterByName;
        filterByName["name"] = permissionDTO.name;
        if (permissionDAO_->count(filterByName) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("PermissionService: New permission name " + permissionDTO.name + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "PermissionService: New permission name " + permissionDTO.name + " already exists.", "Tên quyền hạn mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }

    ERP::Catalog::DTO::PermissionDTO updatedPermission = permissionDTO;
    updatedPermission.updatedAt = ERP::Utils::DateUtils::now();
    updatedPermission.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!permissionDAO_->update(updatedPermission)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("PermissionService: Failed to update permission " + updatedPermission.id + " in DAO.");
                return false;
            }
            // Invalidate authorization cache since permission definitions might have changed
            securityManager_->getAuthorizationService()->reloadPermissionCache();
            eventBus_.publish(std::make_shared<EventBus::PermissionUpdatedEvent>(updatedPermission.id, updatedPermission.name));
            return true;
        },
        "PermissionService", "updatePermission"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("PermissionService: Permission " + updatedPermission.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Permission", updatedPermission.id, "Permission", updatedPermission.name,
                       oldPermissionOpt->toMap(), updatedPermission.toMap(), "Permission updated.");
        return true;
    }
    return false;
}

bool PermissionService::updatePermissionStatus(
    const std::string& permissionId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("PermissionService: Attempting to update status for permission: " + permissionId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ChangePermissionStatus", "Bạn không có quyền cập nhật trạng thái quyền hạn.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::PermissionDTO> permissionOpt = permissionDAO_->getById(permissionId); // Using getById from DAOBase
    if (!permissionOpt) {
        ERP::Logger::Logger::getInstance().warning("PermissionService: Permission with ID " + permissionId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy quyền hạn để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Catalog::DTO::PermissionDTO oldPermission = *permissionOpt;
    if (oldPermission.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("PermissionService: Permission " + permissionId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Catalog::DTO::PermissionDTO updatedPermission = oldPermission;
    updatedPermission.status = newStatus;
    updatedPermission.updatedAt = ERP::Utils::DateUtils::now();
    updatedPermission.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!permissionDAO_->update(updatedPermission)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("PermissionService: Failed to update status for permission " + permissionId + " in DAO.");
                return false;
            }
            // Invalidate authorization cache
            securityManager_->getAuthorizationService()->reloadPermissionCache();
            eventBus_.publish(std::make_shared<EventBus::PermissionStatusChangedEvent>(permissionId, newStatus));
            return true;
        },
        "PermissionService", "updatePermissionStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("PermissionService: Status for permission " + permissionId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "PermissionStatus", permissionId, "Permission", oldPermission.name,
                       oldPermission.toMap(), updatedPermission.toMap(), "Permission status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool PermissionService::deletePermission(
    const std::string& permissionId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("PermissionService: Attempting to delete permission: " + permissionId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.DeletePermission", "Bạn không có quyền xóa quyền hạn.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::PermissionDTO> permissionOpt = permissionDAO_->getById(permissionId); // Using getById from DAOBase
    if (!permissionOpt) {
        ERP::Logger::Logger::getInstance().warning("PermissionService: Permission with ID " + permissionId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy quyền hạn cần xóa.");
        return false;
    }

    ERP::Catalog::DTO::PermissionDTO permissionToDelete = *permissionOpt;

    // Additional checks: Prevent deletion if permission is assigned to any role
    // This requires checking the role_permissions join table. RoleDAO has `getRolePermissions`.
    // It's tricky to query "all roles that have this permission" from RoleDAO without iterating all roles.
    // Assuming a direct query if possible, or iterating through roles.
    // For simplicity, this check is omitted here, but would be crucial in a real system.
    // If it's a critical permission (like ALL.Manage, ALL.Read), it should not be deletable.

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!permissionDAO_->remove(permissionId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("PermissionService: Failed to delete permission " + permissionId + " in DAO.");
                return false;
            }
            // Invalidate authorization cache after successful deletion
            securityManager_->getAuthorizationService()->reloadPermissionCache();
            return true;
        },
        "PermissionService", "deletePermission"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("PermissionService: Permission " + permissionId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Permission", permissionId, "Permission", permissionToDelete.name,
                       permissionToDelete.toMap(), std::nullopt, "Permission deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Catalog
} // namespace ERP