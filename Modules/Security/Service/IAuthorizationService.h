// Modules/Security/Service/IAuthorizationService.h
#ifndef MODULES_SECURITY_SERVICE_IAUTHORIZATIONSERVICE_H
#define MODULES_SECURITY_SERVICE_IAUTHORIZATIONSERVICE_H
#include <string>
#include <vector>
#include <set>      // For std::set<std::string> of permissions
#include <map>      // For std::map for caching roles/permissions

// Rút gọn các include paths (không cần thêm các include DAO, Logger, ErrorHandler, Common, DateUtils, ConnectionPool vào đây)
// Các classes này sẽ được include trong file .cpp triển khai của service này nếu cần.

namespace ERP {
namespace Security {
namespace Services {

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

} // namespace Services
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_IAUTHORIZATIONSERVICE_H