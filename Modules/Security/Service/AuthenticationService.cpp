// Modules/Security/Service/AuthenticationService.cpp
#include "AuthenticationService.h" // Standard includes
#include "User.h"
#include "Event.h"
#include "ConnectionPool.h"
#include "DBConnection.h"
#include "Common.h"
#include "Utils.h"
#include "DateUtils.h"
#include "PasswordHasher.h"
#include "EncryptionService.h"
#include "AuditLogService.h"
#include "AutoRelease.h"
// #include "DTOUtils.h" // Not needed here for QJsonObject conversions anymore

// Removed Qt includes as they are no longer needed here
// #include <QJsonObject>
// #include <QJsonDocument>

namespace ERP {
namespace Security {
namespace Services {

// Template method implementation for executeTransactionInternal
template<typename Func>
bool AuthenticationService::executeTransactionInternal(Func operation, const std::string& serviceName, const std::string& operationName) {
    std::shared_ptr<ERP::Database::DBConnection> db = connectionPool_->getConnection();
    ERP::Utils::AutoRelease releaseGuard([&]() {
        if (db) connectionPool_->releaseConnection(db);
    });
    if (!db) {
        ERP::Logger::Logger::getInstance().critical(serviceName, "Database connection is null. Cannot perform " + operationName + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "Database connection is null.", "Lỗi hệ thống: Không có kết nối cơ sở dữ liệu.");
        return false;
    }
    try {
        db->beginTransaction();
        bool success = operation(db);
        if (success) {
            db->commitTransaction();
            ERP::Logger::Logger::getInstance().info(serviceName, operationName + " completed successfully.");
        } else {
            db->rollbackTransaction();
            ERP::Logger::Logger::getInstance().error(serviceName, operationName + " failed. Transaction rolled back.");
        }
        return success;
    } catch (const std::exception& e) {
        db->rollbackTransaction();
        ERP::Logger::Logger::getInstance().critical(serviceName, "Exception during " + operationName + ": " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Lỗi trong quá trình " + operationName + ": " + std::string(e.what()));
        return false;
    }
}

AuthenticationService::AuthenticationService(
    std::shared_ptr<ERP::User::DAOs::UserDAO> userDAO,
    std::shared_ptr<ERP::Security::DAOs::SessionDAO> sessionDAO,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : userDAO_(userDAO), sessionDAO_(sessionDAO),
      auditLogService_(auditLogService), connectionPool_(connectionPool) {
    if (!userDAO_ || !sessionDAO_ || !auditLogService_ || !connectionPool_) {
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "AuthenticationService: Initialized with null dependencies.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ xác thực.");
        ERP::Logger::Logger::getInstance().critical("AuthenticationService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("AuthenticationService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("AuthenticationService: Initialized.");
}

std::string AuthenticationService::generateSessionToken() {
    return ERP::Utils::generateUUID();
}

// recordAuditLogInternal signature updated to accept std::map<string, any>
void AuthenticationService::recordAuditLogInternal(
    const std::string& userId, const std::string& userName, const std::string& sessionId,
    ERP::Security::DTO::AuditActionType actionType, ERP::Common::LogSeverity severity,
    const std::string& module, const std::string& subModule,
    const std::optional<std::string>& entityId, const std::optional<std::string>& entityType, const std::optional<std::string>& entityName,
    const std::optional<std::string>& ipAddress, const std::optional<std::string>& userAgent, const std::optional<std::string>& workstationId,
    const std::optional<std::string>& productionLineId, const std::optional<std::string>& shiftId, const std::optional<std::string>& batchNumber, const std::optional<std::string>& partNumber,
    const std::optional<std::map<std::string, std::any>>& beforeData, const std::optional<std::map<std::string, std::any>>& afterData, // Changed types
    const std::optional<std::string>& changeReason, const std::map<std::string, std::any>& metadata, // Changed type
    const std::optional<std::string>& comments, const std::optional<std::string>& approvalId,
    bool isCompliant, const std::optional<std::string>& complianceNote) {

    if (!auditLogService_) {
        ERP::Logger::Logger::getInstance().warning("AuthenticationService: AuditLogService is not available. Cannot record audit log.");
        return;
    }

    // Pass std::map<string, any> directly to auditLogService_
    auditLogService_->recordLog(
        userId, userName, sessionId, actionType, severity, module, subModule,
        entityId, entityType, entityName, ipAddress, userAgent, workstationId,
        productionLineId, shiftId, batchNumber, partNumber,
        beforeData, afterData, changeReason, metadata, comments, approvalId, isCompliant, complianceNote
    );
}

std::optional<ERP::Security::DTO::SessionDTO> AuthenticationService::authenticate(
    const std::string& username,
    const std::string& password,
    const std::optional<std::string>& ipAddress,
    const std::optional<std::string>& userAgent,
    const std::optional<std::string>& deviceInfo) {

    ERP::Logger::Logger::getInstance().info("AuthenticationService: Attempting to authenticate user: " + username);

    std::map<std::string, std::any> userFilter;
    userFilter["username"] = username;
    std::vector<ERP::User::DTO::UserDTO> users = userDAO_->get(userFilter); // Using get from DAOBase template

    if (users.empty()) {
        ERP::Logger::Logger::getInstance().warning("AuthenticationService: Authentication failed for user " + username + " - User not found.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::AuthenticationFailed, "Authentication failed: User not found.", "Tên đăng nhập hoặc mật khẩu không đúng.");
        recordAuditLogInternal("N/A", username, "N/A", ERP::Security::DTO::AuditActionType::LOGIN_FAILED, ERP::Common::LogSeverity::WARNING, "Security", "Authentication", std::nullopt, "User", username, ipAddress, userAgent, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, "User not found.", {}, std::nullopt, std::nullopt, false, "Invalid username."); // Metadata empty map
        return std::nullopt;
    }

    ERP::User::DTO::UserDTO user = users[0];

    // Check if user is locked
    if (user.isLocked && user.lockUntilTime.has_value() && ERP::Utils::DateUtils::now() < *user.lockUntilTime) {
        ERP::Logger::Logger::getInstance().warning("AuthenticationService: Authentication failed for user " + username + " - Account locked until " + ERP::Utils::DateUtils::formatDateTime(*user.lockUntilTime, ERP::Common::DATETIME_FORMAT));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::AuthenticationFailed, "Account locked.", "Tài khoản bị khóa. Vui lòng thử lại sau.");
        recordAuditLogInternal(user.id, user.username, "N/A", ERP::Security::DTO::AuditActionType::LOGIN_FAILED, ERP::Common::LogSeverity::WARNING, "Security", "Authentication", user.id, "User", user.username, ipAddress, userAgent, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, "Account locked.", {}, std::nullopt, std::nullopt, false, "Account locked due to too many failed attempts."); // Metadata empty map
        return std::nullopt;
    }

    // Validate password
    if (!ERP::Security::Utils::PasswordHasher::verifyPassword(password, user.passwordSalt, user.passwordHash)) {
        ERP::Logger::Logger::getInstance().warning("AuthenticationService: Authentication failed for user " + username + " - Invalid password.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::AuthenticationFailed, "Authentication failed: Invalid password.", "Tên đăng nhập hoặc mật khẩu không đúng.");

        // Increment failed login attempts
        user.failedLoginAttempts++;
        if (user.failedLoginAttempts >= 5) { // Threshold for locking account
            user.isLocked = true;
            user.lockUntilTime = ERP::Utils::DateUtils::now() + std::chrono::minutes(30); // Lock for 30 minutes
            ERP::Logger::Logger::getInstance().warning("AuthenticationService: User account " + username + " locked due to too many failed attempts.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::AuthenticationFailed, "Too many failed login attempts. Account locked.", "Tài khoản bị khóa do quá nhiều lần đăng nhập sai. Vui lòng thử lại sau.");
        }
        user.updatedAt = ERP::Utils::DateUtils::now();
        user.updatedBy = "system"; // System update for login attempts

        executeTransactionInternal(
            [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                return userDAO_->update(user); // Update user record (failed attempts, lock status)
            },
            "AuthenticationService", "updateUserAfterFailedLogin"
        );

        recordAuditLogInternal(user.id, user.username, "N/A", ERP::Security::DTO::AuditActionType::LOGIN_FAILED, ERP::Common::LogSeverity::WARNING, "Security", "Authentication", user.id, "User", user.username, ipAddress, userAgent, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, "Invalid password.", {}, std::nullopt, std::nullopt, false, "Incorrect password provided."); // Metadata empty map
        return std::nullopt;
    }

    // Reset failed login attempts on successful login
    if (user.failedLoginAttempts > 0 || user.isLocked) {
        user.failedLoginAttempts = 0;
        user.isLocked = false;
        user.lockUntilTime = std::nullopt;
        user.updatedAt = ERP::Utils::DateUtils::now();
        user.updatedBy = "system";
        executeTransactionInternal(
            [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                return userDAO_->update(user); // Reset user record
            },
            "AuthenticationService", "resetUserLoginAttempts"
        );
    }

    // Create a new session
    ERP::Security::DTO::SessionDTO session;
    session.id = generateSessionToken(); // Use unique ID for session
    session.userId = user.id;
    session.token = generateSessionToken(); // Use UUID for token
    session.expirationTime = ERP::Utils::DateUtils::now() + std::chrono::minutes(30); // Session valid for 30 minutes
    session.ipAddress = ipAddress;
    session.userAgent = userAgent;
    session.deviceInfo = deviceInfo;
    session.createdAt = ERP::Utils::DateUtils::now();
    session.createdBy = user.id;
    session.status = ERP::Common::EntityStatus::ACTIVE;

    std::optional<ERP::Security::DTO::SessionDTO> createdSession = std::nullopt;
    bool success = executeTransactionInternal(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!sessionDAO_->create(session)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("AuthenticationService: Failed to create session for user " + username + " in DAO.");
                return false;
            }
            createdSession = session;
            // Update last login time and IP in user DTO
            user.lastLoginTime = ERP::Utils::DateUtils::now();
            user.lastLoginIp = ipAddress;
            user.updatedAt = ERP::Utils::DateUtils::now();
            user.updatedBy = "system";
            userDAO_->update(user); // Update user's last login info
            return true;
        },
        "AuthenticationService", "createSession"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("AuthenticationService: User " + username + " authenticated successfully. Session ID: " + createdSession->id);
        eventBus_.publish(std::make_shared<EventBus::UserLoggedInEvent>(user.id, user.username, createdSession->id, ipAddress.value_or("N/A")));
        recordAuditLogInternal(user.id, user.username, createdSession->id, ERP::Security::DTO::AuditActionType::LOGIN, ERP::Common::LogSeverity::INFO, "Security", "Authentication", user.id, "User", user.username, ipAddress, userAgent, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, createdSession->toMap(), "User logged in.", {}, std::nullopt, std::nullopt, true, std::nullopt); // Passed map directly
        return createdSession;
    }
    return std::nullopt;
}

bool AuthenticationService::logout(const std::string& sessionId) {
    ERP::Logger::Logger::getInstance().info("AuthenticationService: Attempting to logout session: " + sessionId);

    std::optional<ERP::Security::DTO::SessionDTO> sessionOpt = sessionDAO_->getById(sessionId); // Using getById from DAOBase
    if (!sessionOpt) {
        ERP::Logger::Logger::getInstance().warning("AuthenticationService: Logout failed for session " + sessionId + " - Session not found.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Session not found for logout.", "Phiên đăng nhập không tồn tại.");
        return false;
    }

    ERP::Security::DTO::SessionDTO session = *sessionOpt;

    // Mark session as inactive/deleted
    session.status = ERP::Common::EntityStatus::INACTIVE; // Soft delete
    session.updatedAt = ERP::Utils::DateUtils::now();
    session.updatedBy = session.userId; // User who owns the session

    bool success = executeTransactionInternal(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            return sessionDAO_->update(session); // Using update from DAOBase template
        },
        "AuthenticationService", "logout"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("AuthenticationService: Session " + sessionId + " logged out successfully.");
        eventBus_.publish(std::make_shared<EventBus::UserLoggedOutEvent>(session.userId, session.id));
        recordAuditLogInternal(session.userId, "N/A", session.id, ERP::Security::DTO::AuditActionType::LOGOUT, ERP::Common::LogSeverity::INFO, "Security", "Authentication", session.userId, "User", session.userId, session.ipAddress, session.userAgent, std::nullopt, std::nullopt, std::nullopt, std::nullopt, session.toMap(), std::nullopt, "User logged out.", {}, std::nullopt, std::nullopt, true, std::nullopt); // Passed map directly
        return true;
    }
    return false;
}

std::optional<ERP::Security::DTO::SessionDTO> AuthenticationService::validateSession(const std::string& token) {
    ERP::Logger::Logger::getInstance().debug("AuthenticationService: Validating session token.");

    std::map<std::string, std::any> tokenFilter;
    tokenFilter["token"] = token;
    std::vector<ERP::Security::DTO::SessionDTO> sessions = sessionDAO_->get(tokenFilter); // Using get from DAOBase template

    if (sessions.empty()) {
        ERP::Logger::Logger::getInstance().debug("AuthenticationService: Session validation failed - Token not found.");
        return std::nullopt;
    }

    ERP::Security::DTO::SessionDTO session = sessions[0];

    // Check if session is active and not expired
    if (session.status != ERP::Common::EntityStatus::ACTIVE ||
        (session.expirationTime < ERP::Utils::DateUtils::now())) {
        ERP::Logger::Logger::getInstance().debug("AuthenticationService: Session validation failed - Session is inactive or expired.");
        // Optionally, mark expired session as inactive in DB
        if (session.status == ERP::Common::EntityStatus::ACTIVE && session.expirationTime < ERP::Utils::DateUtils::now()) {
            session.status = ERP::Common::EntityStatus::INACTIVE;
            session.updatedAt = ERP::Utils::DateUtils::now();
            session.updatedBy = session.userId;
            executeTransactionInternal(
                [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                    return sessionDAO_->update(session);
                },
                "AuthenticationService", "markExpiredSessionInactive"
            );
        }
        return std::nullopt;
    }

    ERP::Logger::Logger::getInstance().debug("AuthenticationService: Session token valid for user: " + session.userId);
    return session;
}

std::optional<ERP::Security::DTO::SessionDTO> AuthenticationService::refreshSession(const std::string& sessionId) {
    ERP::Logger::Logger::getInstance().info("AuthenticationService: Attempting to refresh session: " + sessionId);

    std::optional<ERP::Security::DTO::SessionDTO> sessionOpt = sessionDAO_->getById(sessionId); // Using getById from DAOBase
    if (!sessionOpt) {
        ERP::Logger::Logger::getInstance().warning("AuthenticationService: Session refresh failed for session " + sessionId + " - Session not found.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Session not found for refresh.", "Phiên đăng nhập không tồn tại.");
        return std::nullopt;
    }

    ERP::Security::DTO::SessionDTO session = *sessionOpt;

    // Only refresh active and non-expired sessions
    if (session.status != ERP::Common::EntityStatus::ACTIVE ||
        (session.expirationTime < ERP::Utils::DateUtils::now())) {
        ERP::Logger::Logger::getInstance().warning("AuthenticationService: Session " + sessionId + " is not active or already expired, cannot refresh.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::SessionExpired, "Session is not active or expired, cannot refresh.", "Phiên đăng nhập không hoạt động hoặc đã hết hạn.");
        return std::nullopt;
    }

    session.expirationTime = ERP::Utils::DateUtils::now() + std::chrono::minutes(30); // Extend by 30 minutes
    session.updatedAt = ERP::Utils::DateUtils::now();
    session.updatedBy = session.userId;

    std::optional<ERP::Security::DTO::SessionDTO> refreshedSession = std::nullopt;
    bool success = executeTransactionInternal(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!sessionDAO_->update(session)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("AuthenticationService: Failed to refresh session " + sessionId + " in DAO.");
                return false;
            }
            refreshedSession = session;
            return true;
        },
        "AuthenticationService", "refreshSession"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("AuthenticationService: Session " + sessionId + " refreshed successfully. New expiry: " + ERP::Utils::DateUtils::formatDateTime(refreshedSession->expirationTime, ERP::Common::DATETIME_FORMAT));
        return refreshedSession;
    }
    return std::nullopt;
}

} // namespace Services
} // namespace Security
} // namespace ERP