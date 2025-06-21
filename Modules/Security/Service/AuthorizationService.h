// Modules/Security/Service/AuthorizationService.h
#ifndef MODULES_SECURITY_SERVICE_AUTHORIZATIONSERVICE_H
#define MODULES_SECURITY_SERVICE_AUTHORIZATIONSERVICE_H
#include <string>
#include <vector>
#include <set>      // For std::set<std::string> of permissions
#include <map>      // For std::map for caching roles/permissions
#include <memory>   // For std::shared_ptr
#include <mutex>    // For std::mutex for thread safety

#include "RoleDAO.h"        // Đã rút gọn include
#include "PermissionDAO.h"  // Đã rút gọn include
#include "UserDAO.h"        // Đã rút gọn include
#include "Logger.h"         // Đã rút gọn include
#include "ErrorHandler.h"   // Đã rút gọn include
#include "Common.h"         // Đã rút gọn include
#include "DateUtils.h"      // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include

namespace ERP {
namespace Security {
namespace Services {

// Forward declare if other services are used
// class IUserService; // No direct dependency, uses UserDAO directly

/**
 * @brief IAuthorizationService interface defines operations for user authorization (permission checking).
 */
class IAuthorizationService {
public:
    virtual ~IAuthorizationService() = default;
    /**
     * @brief Checks if a user has a specific permission.
     * This version receives user ID and role IDs directly.
     * @param userId The ID of the user.
     * @param userRoleIds The list of role IDs the user belongs to.
     * @param permissionName The name of the permission to check (e.g., "Sales.CreateOrder").
     * @return true if the user has the permission, false otherwise.
     */
    virtual bool hasPermission(
        const std::string& userId, // Pass userId for potential user-specific permissions/auditing
        const std::vector<std::string>& userRoleIds,
        const std::string& permissionName) = 0;
    /**
     * @brief Reloads the permission cache.
     * This should be called when roles or permissions are updated in the database.
     */
    virtual void reloadPermissionCache() = 0;
};
/**
 * @brief Default implementation of IAuthorizationService.
 * This class manages roles and permissions, including caching for performance.
 */
class AuthorizationService : public IAuthorizationService {
public:
    /**
     * @brief Constructor for AuthorizationService.
     * @param roleDAO Shared pointer to RoleDAO.
     * @param permissionDAO Shared pointer to PermissionDAO.
     * @param userDAO Shared pointer to UserDAO.
     * @param connectionPool Shared pointer to ConnectionPool.
     */
    AuthorizationService(std::shared_ptr<ERP::Catalog::DAOs::RoleDAO> roleDAO,
                         std::shared_ptr<ERP::Catalog::DAOs::PermissionDAO> permissionDAO,
                         std::shared_ptr<ERP::User::DAOs::UserDAO> userDAO, // To get user roles if not passed directly
                         std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);

    bool hasPermission(
        const std::string& userId, // Pass userId for potential user-specific permissions/auditing
        const std::vector<std::string>& userRoleIds,
        const std::string& permissionName) override;
    void reloadPermissionCache() override;

private:
    std::shared_ptr<ERP::Catalog::DAOs::RoleDAO> roleDAO_;
    std::shared_ptr<ERP::Catalog::DAOs::PermissionDAO> permissionDAO_;
    std::shared_ptr<ERP::User::DAOs::UserDAO> userDAO_; // Used to fetch user roles if not provided
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool_;

    // Cache for role permissions: roleId -> set of permission names
    static std::map<std::string, std::set<std::string>> s_rolePermissionsCache;
    static std::mutex s_cacheMutex; // Mutex for thread-safe access to cache

    /**
     * @brief Loads permissions for a specific role into the cache from the database.
     * @param roleId The ID of the role.
     * @return A set of permission names for the role.
     */
    std::set<std::string> loadPermissionsForRole(const std::string& roleId);
};
} // namespace Services
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_AUTHORIZATIONSERVICE_H