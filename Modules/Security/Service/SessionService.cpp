// Modules/Security/Service/SessionService.cpp
#include "SessionService.h" // Đã rút gọn include
#include "Session.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Security {
namespace Services {

SessionService::SessionService(
    std::shared_ptr<DAOs::SessionDAO> sessionDAO,
    std::shared_ptr<ERP::User::Services::IUserService> userService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      sessionDAO_(sessionDAO), userService_(userService) {
    if (!sessionDAO_ || !userService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "SessionService: Initialized with null DAO or UserService.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ phiên người dùng.");
        ERP::Logger::Logger::getInstance().critical("SessionService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("SessionService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("SessionService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Security::DTO::SessionDTO> SessionService::createSession(
    const ERP::Security::DTO::SessionDTO& sessionDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SessionService: Attempting to create session for user: " + sessionDTO.userId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Security.CreateSession", "Bạn không có quyền tạo phiên đăng nhập.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (sessionDTO.userId.empty() || sessionDTO.token.empty()) {
        ERP::Logger::Logger::getInstance().warning("SessionService: Invalid input for session creation (empty userId or token).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SessionService: Invalid input for session creation.", "Thông tin phiên không đầy đủ.");
        return std::nullopt;
    }

    // Validate User existence (assuming UserService handles permission internally)
    std::optional<ERP::User::DTO::UserDTO> user = userService_->getUserById(sessionDTO.userId, userRoleIds);
    if (!user || user->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("SessionService: Invalid User ID provided or user is not active: " + sessionDTO.userId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID người dùng không hợp lệ hoặc người dùng không hoạt động.");
        return std::nullopt;
    }

    ERP::Security::DTO::SessionDTO newSession = sessionDTO;
    newSession.id = ERP::Utils::generateUUID();
    newSession.createdAt = ERP::Utils::DateUtils::now();
    newSession.createdBy = currentUserId;
    newSession.status = ERP::Common::EntityStatus::ACTIVE; // Default to active

    std::optional<ERP::Security::DTO::SessionDTO> createdSession = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!sessionDAO_->create(newSession)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("SessionService: Failed to create session for user " + newSession.userId + " in DAO.");
                return false;
            }
            createdSession = newSession;
            // Optionally, publish an event for new session
            // eventBus_.publish(std::make_shared<EventBus::SessionCreatedEvent>(newSession));
            return true;
        },
        "SessionService", "createSession"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SessionService: Session created successfully for user: " + newSession.userId + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::LOGIN, // Assuming session creation is part of login
                       ERP::Common::LogSeverity::INFO,
                       "Security", "Session", newSession.id, "Session", newSession.userId,
                       std::nullopt, newSession.toMap(), "Session created for user: " + newSession.userId + ".");
        return createdSession;
    }
    return std::nullopt;
}

std::optional<ERP::Security::DTO::SessionDTO> SessionService::getSessionById(
    const std::string& sessionId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SessionService: Retrieving session by ID: " + sessionId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Security.ViewSession", "Bạn không có quyền xem phiên đăng nhập.")) {
        return std::nullopt;
    }

    return sessionDAO_->getById(sessionId); // Using getById from DAOBase template
}

std::vector<ERP::Security::DTO::SessionDTO> SessionService::getAllSessions(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SessionService: Retrieving all sessions with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Security.ViewAllSessions", "Bạn không có quyền xem tất cả phiên đăng nhập.")) {
        return {};
    }

    return sessionDAO_->get(filter); // Using get from DAOBase template
}

std::vector<ERP::Security::DTO::SessionDTO> SessionService::getSessionsForUser(
    const std::string& userIdToRetrieve,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SessionService: Retrieving sessions for user: " + userIdToRetrieve + ".");

    // Permission check: User can view their own sessions, or Admin/specific role can view others'
    if (userIdToRetrieve != currentUserId && !checkPermission(currentUserId, userRoleIds, "Security.ViewUserSessions", "Bạn không có quyền xem phiên của người dùng khác.")) {
        return {};
    }

    std::map<std::string, std::any> filter;
    filter["user_id"] = userIdToRetrieve;

    return sessionDAO_->get(filter); // Using get from DAOBase template
}

bool SessionService::updateSession(
    const ERP::Security::DTO::SessionDTO& sessionDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SessionService: Attempting to update session: " + sessionDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Security.UpdateSession", "Bạn không có quyền cập nhật phiên đăng nhập.")) {
        return false;
    }

    std::optional<ERP::Security::DTO::SessionDTO> oldSessionOpt = sessionDAO_->getById(sessionDTO.id); // Using getById from DAOBase
    if (!oldSessionOpt) {
        ERP::Logger::Logger::getInstance().warning("SessionService: Session with ID " + sessionDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiên đăng nhập cần cập nhật.");
        return false;
    }

    ERP::Security::DTO::SessionDTO updatedSession = sessionDTO;
    updatedSession.updatedAt = ERP::Utils::DateUtils::now();
    updatedSession.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!sessionDAO_->update(updatedSession)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SessionService: Failed to update session " + updatedSession.id + " in DAO.");
                return false;
            }
            return true;
        },
        "SessionService", "updateSession"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SessionService: Session " + updatedSession.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Security", "Session", updatedSession.id, "Session", updatedSession.userId,
                       oldSessionOpt->toMap(), updatedSession.toMap(), "Session updated.");
        return true;
    }
    return false;
}

bool SessionService::deleteSession(
    const std::string& sessionId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SessionService: Attempting to delete session: " + sessionId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Security.DeleteSession", "Bạn không có quyền xóa phiên đăng nhập.")) {
        return false;
    }

    std::optional<ERP::Security::DTO::SessionDTO> sessionOpt = sessionDAO_->getById(sessionId); // Using getById from DAOBase
    if (!sessionOpt) {
        ERP::Logger::Logger::getInstance().warning("SessionService: Session with ID " + sessionId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiên đăng nhập cần xóa.");
        return false;
    }

    ERP::Security::DTO::SessionDTO sessionToDelete = *sessionOpt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!sessionDAO_->remove(sessionId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("SessionService: Failed to delete session " + sessionId + " in DAO.");
                return false;
            }
            return true;
        },
        "SessionService", "deleteSession"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SessionService: Session " + sessionId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::LOGOUT, // Assuming deletion implies logout
                       ERP::Common::LogSeverity::INFO,
                       "Security", "Session", sessionId, "Session", sessionToDelete.userId,
                       sessionToDelete.toMap(), std::nullopt, "Session deleted.");
        return true;
    }
    return false;
}

bool SessionService::deactivateSession(
    const std::string& sessionId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SessionService: Attempting to deactivate session: " + sessionId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Security.DeactivateSession", "Bạn không có quyền vô hiệu hóa phiên đăng nhập.")) {
        return false;
    }

    std::optional<ERP::Security::DTO::SessionDTO> sessionOpt = sessionDAO_->getById(sessionId);
    if (!sessionOpt) {
        ERP::Logger::Logger::getInstance().warning("SessionService: Session with ID " + sessionId + " not found for deactivation.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiên đăng nhập để vô hiệu hóa.");
        return false;
    }
    
    ERP::Security::DTO::SessionDTO oldSession = *sessionOpt;
    if (oldSession.status == ERP::Common::EntityStatus::INACTIVE) {
        ERP::Logger::Logger::getInstance().info("SessionService: Session " + sessionId + " is already inactive.");
        return true; // Already inactive
    }

    ERP::Security::DTO::SessionDTO updatedSession = oldSession;
    updatedSession.status = ERP::Common::EntityStatus::INACTIVE;
    updatedSession.updatedAt = ERP::Utils::DateUtils::now();
    updatedSession.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!sessionDAO_->update(updatedSession)) {
                ERP::Logger::Logger::getInstance().error("SessionService: Failed to deactivate session " + sessionId + " in DAO.");
                return false;
            }
            // Optionally, publish event for session deactivation
            // eventBus_.publish(std::make_shared<EventBus::SessionDeactivatedEvent>(sessionId));
            return true;
        },
        "SessionService", "deactivateSession"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SessionService: Session " + sessionId + " deactivated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::LOGOUT, // Treated as a forced logout/deactivation
                       ERP::Common::LogSeverity::INFO,
                       "Security", "Session", sessionId, "Session", oldSession.userId,
                       oldSession.toMap(), updatedSession.toMap(), "Session deactivated.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Security
} // namespace ERP