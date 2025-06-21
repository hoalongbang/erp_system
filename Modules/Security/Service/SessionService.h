// Modules/Security/Service/SessionService.h
#ifndef MODULES_SECURITY_SERVICE_SESSIONSERVICE_H
#define MODULES_SECURITY_SERVICE_SESSIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Session.h"          // Đã rút gọn include
#include "SessionDAO.h"       // Đã rút gọn include
#include "UserService.h"      // For User validation (needed for SessionService)
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

namespace ERP {
namespace Security {
namespace Services {

// Forward declare if UserService is only used via pointer/reference
// class IUserService;

/**
 * @brief ISessionService interface defines operations for managing user sessions.
 */
class ISessionService {
public:
    virtual ~ISessionService() = default;
    /**
     * @brief Creates a new session.
     * @param sessionDTO DTO containing new session information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SessionDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> createSession(
        const ERP::Security::DTO::SessionDTO& sessionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves session information by ID.
     * @param sessionId ID of the session to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SessionDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> getSessionById(
        const std::string& sessionId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all sessions or sessions matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching SessionDTOs.
     */
    virtual std::vector<ERP::Security::DTO::SessionDTO> getAllSessions(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves sessions for a specific user.
     * @param userIdToRetrieve ID of the user whose sessions are to be retrieved.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching SessionDTOs.
     */
    virtual std::vector<ERP::Security::DTO::SessionDTO> getSessionsForUser(
        const std::string& userIdToRetrieve,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates session information.
     * @param sessionDTO DTO containing updated session information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateSession(
        const ERP::Security::DTO::SessionDTO& sessionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a session record by ID.
     * @param sessionId ID of the session to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteSession(
        const std::string& sessionId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deactivates a session (marks as inactive without deleting).
     * @param sessionId ID of the session to deactivate.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deactivation is successful, false otherwise.
     */
    virtual bool deactivateSession(
        const std::string& sessionId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of ISessionService.
 * This class uses SessionDAO and ISecurityManager.
 */
class SessionService : public ISessionService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for SessionService.
     * @param sessionDAO Shared pointer to SessionDAO.
     * @param userService Shared pointer to IUserService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    SessionService(std::shared_ptr<DAOs::SessionDAO> sessionDAO,
                   std::shared_ptr<ERP::User::Services::IUserService> userService,
                   std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                   std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                   std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                   std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Security::DTO::SessionDTO> createSession(
        const ERP::Security::DTO::SessionDTO& sessionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Security::DTO::SessionDTO> getSessionById(
        const std::string& sessionId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Security::DTO::SessionDTO> getAllSessions(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Security::DTO::SessionDTO> getSessionsForUser(
        const std::string& userIdToRetrieve,
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateSession(
        const ERP::Security::DTO::SessionDTO& sessionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteSession(
        const std::string& sessionId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deactivateSession(
        const std::string& sessionId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::SessionDAO> sessionDAO_;
    std::shared_ptr<ERP::User::Services::IUserService> userService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Old private helper functions removed as they are now in BaseService
};
} // namespace Services
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_SESSIONSERVICE_H