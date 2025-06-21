// Modules/Security/Service/AuthorizationService.cpp
#include "AuthorizationService.h" // Đã rút gọn include
#include "Role.h" // Đã rút gọn include
#include "Permission.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "User.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::any_of, std::all_of

namespace ERP {
namespace Security {
namespace Services {

// Initialize static members
std::map<std::string, std::set<std::string>> AuthorizationService::s_rolePermissionsCache;
std::mutex AuthorizationService::s_cacheMutex;

AuthorizationService::AuthorizationService(
    std::shared_ptr<ERP::Catalog::DAOs::RoleDAO> roleDAO,
    std::shared_ptr<ERP::Catalog::DAOs::PermissionDAO> permissionDAO,
    std::shared_ptr<ERP::User::DAOs::UserDAO> userDAO,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : roleDAO_(roleDAO), permissionDAO_(permissionDAO), userDAO_(userDAO), connectionPool_(connectionPool) {
    if (!roleDAO_ || !permissionDAO_ || !userDAO_ || !connectionPool_) {
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "AuthorizationService: Initialized with null dependencies.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ ủy quyền.");
        ERP::Logger::Logger::getInstance().critical("AuthorizationService: One or more injected DAOs are null.");
        throw std::runtime_error("AuthorizationService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("AuthorizationService: Initialized. Loading permissions to cache...");
    // Initial load of permissions into cache
    reloadPermissionCache();
}

bool AuthorizationService::hasPermission(
    const std::string& userId,
    const std::vector<std::string>& userRoleIds,
    const std::string& permissionName) {
    // If the permission is a generic "ALL.Manage" or "ALL.Read" and user has it, grant access
    // This allows for super-admin roles without listing every single permission
    if (userRoleIds.empty()) {
        ERP::Logger::Logger::getInstance().warning("AuthorizationService: User " + userId + " has no roles assigned.");
        return false;
    }

    std::lock_guard<std::mutex> lock(s_cacheMutex); // Protect cache access

    for (const std::string& roleId : userRoleIds) {
        // Check if role is in cache, if not, load it
        if (s_rolePermissionsCache.find(roleId) == s_rolePermissionsCache.end()) {
            s_rolePermissionsCache[roleId] = loadPermissionsForRole(roleId);
        }

        // Check for specific permission
        if (s_rolePermissionsCache[roleId].count(permissionName)) {
            ERP::Logger::Logger::getInstance().debug("AuthorizationService: User " + userId + " has permission " + permissionName + " via role " + roleId + ".");
            return true;
        }
        // Check for "ALL.Manage" or "ALL.Read" permissions for this role
        if (s_rolePermissionsCache[roleId].count("ALL.Manage")) {
            ERP::Logger::Logger::getInstance().debug("AuthorizationService: User " + userId + " has ALL.Manage permission via role " + roleId + ".");
            return true; // Has full access
        }
        if (permissionName.rfind(".View") != std::string::npos && s_rolePermissionsCache[roleId].count("ALL.Read")) {
            ERP::Logger::Logger::getInstance().debug("AuthorizationService: User " + userId + " has ALL.Read permission via role " + roleId + " for view operation.");
            return true; // Has read access for view operations
        }
    }
    ERP::Logger::Logger::getInstance().info("AuthorizationService: User " + userId + " denied permission: " + permissionName);
    return false;
}

void AuthorizationService::reloadPermissionCache() {
    std::lock_guard<std::mutex> lock(s_cacheMutex); // Protect cache access
    s_rolePermissionsCache.clear();
    ERP::Logger::Logger::getInstance().info("AuthorizationService: Permission cache cleared. Will reload on demand.");

    // Optionally, pre-load all active roles' permissions here
    // std::map<std::string, std::any> activeRoleFilter;
    // activeRoleFilter["status"] = static_cast<int>(ERP::Common::EntityStatus::ACTIVE);
    // std::vector<ERP::Catalog::DTO::RoleDTO> activeRoles = roleDAO_->get(activeRoleFilter);
    // for (const auto& role : activeRoles) {
    //     loadPermissionsForRole(role.id);
    // }
    // ERP::Logger::Logger::getInstance().info("AuthorizationService: Pre-loaded permissions for " + std::to_string(activeRoles.size()) + " active roles.");
}

std::set<std::string> AuthorizationService::loadPermissionsForRole(const std::string& roleId) {
    ERP::Logger::Logger::getInstance().debug("AuthorizationService: Loading permissions for role '" + roleId + "' from database.");
    std::set<std::string> permissions;

    // Check if role exists and is active first
    std::optional<ERP::Catalog::DTO::RoleDTO> roleOpt = roleDAO_->getById(roleId);
    if (!roleOpt || roleOpt->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("AuthorizationService: Role " + roleId + " not found or not active. No permissions.");
        return {}; // Return empty set if role not found or not active
    }

    // RoleDAO's getRolePermissions requires its own connection management
    // Re-instantiate DAO (it uses ConnectionPool internally) or pass connection
    // For simplicity, let RoleDAO manage its connection from the pool.
    std::vector<std::map<std::string, std::any>> results = roleDAO_->getRolePermissions(roleId);
    
    for (const auto& row : results) {
        if (row.count("permission_name") && row.at("permission_name").type() == typeid(std::string)) {
            permissions.insert(std::any_cast<std::string>(row.at("permission_name")));
        }
    }
    // Cache the loaded permissions (this is done in hasPermission for specific roles already)
    // s_rolePermissionsCache[roleId] = permissions; // Avoid recursive locking if loadPermissionsForRole calls itself
    ERP::Logger::Logger::getInstance().info("AuthorizationService: Loaded " + std::to_string(permissions.size()) + " permissions for role '" + roleId + "'.");
    return permissions;
}

} // namespace Services
} // namespace Security
} // namespace ERP