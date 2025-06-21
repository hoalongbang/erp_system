// Modules/Security/Service/AuthenticationService.h
#ifndef MODULES_SECURITY_SERVICE_AUTHENTICATIONSERVICE_H
#define MODULES_SECURITY_SERVICE_AUTHENTICATIONSERVICE_H
#include <string>
#include <optional>
#include <memory>
#include <vector>
#include <chrono>

#include "User.h"             // Đã rút gọn include
#include "UserDAO.h"          // Đã rút gọn include
#include "Session.h"          // Đã rút gọn include
#include "SessionDAO.h"       // Đã rút gọn include
#include "PasswordHasher.h"   // Đã rút gọn include
#include "EncryptionService.h" // Đã rút gọn include
#include "AuditLogService.h"  // Đã rút gọn include
#include "ConnectionPool.h"   // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DTOUtils.h"         // Đã rút gọn include

namespace ERP {
namespace Security {
namespace Services {

// Forward declare if other services are used via interface
// class IUserService;
// class ISessionService;

/**
 * @brief IAuthenticationService interface defines operations for user authentication.
 */
class IAuthenticationService {
public:
    virtual ~IAuthenticationService() = default;
    /**
     * @brief Authenticates a user by username and password.
     * @param username The user's username.
     * @param password The user's plain text password.
     * @param ipAddress The IP address of the login attempt.
     * @param userAgent The user agent string (e.g., browser info).
     * @param deviceInfo Additional device information.
     * @return An optional SessionDTO if authentication is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> authenticate(
        const std::string& username,
        const std::string& password,
        const std::optional<std::string>& ipAddress = std::nullopt,
        const std::optional<std::string>& userAgent = std::nullopt,
        const std::optional<std::string>& deviceInfo = std::nullopt) = 0;
    /**
     * @brief Logs out a user by invalidating their session.
     * @param sessionId The ID of the session to invalidate.
     * @return true if logout is successful, false otherwise.
     */
    virtual bool logout(const std::string& sessionId) = 0;
    /**
     * @brief Validates a session token.
     * @param token The session token to validate.
     * @return An optional SessionDTO if the token is valid and active, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> validateSession(const std::string& token) = 0;
    /**
     * @brief Refreshes an existing session, extending its expiration time.
     * @param sessionId The ID of the session to refresh.
     * @return An optional updated SessionDTO if successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> refreshSession(const std::string& sessionId) = 0;
};
/**
 * @brief Default implementation of IAuthenticationService.
 * This class handles user login, logout, and session management.
 */
class AuthenticationService : public IAuthenticationService {
public:
    /**
     * @brief Constructor for AuthenticationService.
     * @param userDAO Shared pointer to UserDAO.
     * @param sessionDAO Shared pointer to SessionDAO.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     */
    AuthenticationService(std::shared_ptr<ERP::User::DAOs::UserDAO> userDAO,
                          std::shared_ptr<ERP::Security::DAOs::SessionDAO> sessionDAO,
                          std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                          std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);

    std::optional<ERP::Security::DTO::SessionDTO> authenticate(
        const std::string& username,
        const std::string& password,
        const std::optional<std::string>& ipAddress = std::nullopt,
        const std::optional<std::string>& userAgent = std::nullopt,
        const std::optional<std::string>& deviceInfo = std::nullopt) override;
    bool logout(const std::string& sessionId) override;
    std::optional<ERP::Security::DTO::SessionDTO> validateSession(const std::string& token) override;
    std::optional<ERP::Security::DTO::SessionDTO> refreshSession(const std::string& sessionId) override;

private:
    std::shared_ptr<ERP::User::DAOs::UserDAO> userDAO_;
    std::shared_ptr<ERP::Security::DAOs::SessionDAO> sessionDAO_;
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService_;
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool_;
    // No direct dependency on IUserService or ISessionService (circular), use DAOs directly.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance(); // Access singleton EventBus

    // Helper to generate session token (UUID is used for simplicity)
    std::string generateSessionToken();
    // Helper to record audit logs (replicated from BaseService for foundational services)
    void recordAuditLogInternal(
        const std::string& userId, const std::string& userName, const std::string& sessionId,
        ERP::Security::DTO::AuditActionType actionType, ERP::Common::LogSeverity severity,
        const std::string& module, const std::string& subModule,
        const std::optional<std::string>& entityId = std::nullopt, const std::optional<std::string>& entityType = std::nullopt, const std::optional<std::string>& entityName = std::nullopt,
        const std::optional<std::string>& ipAddress = std::nullopt, const std::optional<std::string>& userAgent = std::nullopt, const std::optional<std::string>& workstationId = std::nullopt,
        const std::optional<std::string>& productionLineId = std::nullopt, const std::optional<std::string>& shiftId = std::nullopt, const std::optional<std::string>& batchNumber = std::nullopt, const std::optional<std::string>& partNumber = std::nullopt,
        const std::optional<std::map<std::string, std::any>>& beforeData = std::nullopt, const std::optional<std::map<std::string, std::any>>& afterData = std::nullopt,
        const std::optional<std::string>& changeReason = std::nullopt, const std::map<std::string, std::any>& metadata = {},
        const std::optional<std::string>& comments = std::nullopt, const std::optional<std::string>& approvalId = std::nullopt,
        bool isCompliant = true, const std::optional<std::string>& complianceNote = std::nullopt);

    // Helper for transaction management (replicated from BaseService for foundational services)
    template<typename Func>
    bool executeTransactionInternal(Func operation, const std::string& serviceName, const std::string& operationName);
};
} // namespace Services
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_AUTHENTICATIONSERVICE_H