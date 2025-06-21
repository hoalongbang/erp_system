// Modules/Common/Service/BaseService.cpp
#include "BaseService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "IAuthorizationService.h" // Đã rút gọn include
#include "IAuditLogService.h" // Đã rút gọn include
#include "Logger.h" // Đã rút gọn include
#include "ErrorHandler.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DTOUtils.h" // For mapToJsonString
#include <nlohmann/json.hpp> // For JSON conversion

// Removed Qt includes as they are no longer needed here
// #include <QJsonObject>
// #include <QJsonDocument>

namespace ERP {
namespace Common {
namespace Services {

BaseService::BaseService(
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager) // Optional securityManager
    : authorizationService_(authorizationService),
      auditLogService_(auditLogService),
      connectionPool_(connectionPool),
      securityManager_(securityManager)
{
    if (!authorizationService_ || !auditLogService_ || !connectionPool_) {
        ERP::Logger::Logger::getInstance().critical("BaseService", "One or more core dependencies are null.");
        throw std::runtime_error("BaseService: Null core dependencies.");
    }
    ERP::Logger::Logger::getInstance().debug("BaseService: Initialized.");
}

bool BaseService::checkPermission(const std::string& userId, const std::vector<std::string>& roleIds,
                                 const std::string& permission, const std::string& errorMessage) {
    if (!authorizationService_) {
        ERP::Logger::Logger::getInstance().critical("BaseService", "AuthorizationService is null. Cannot perform permission check for " + permission);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "AuthorizationService is null.", "Lỗi hệ thống: Dịch vụ ủy quyền không khả dụng.");
        return false;
    }

    if (!authorizationService_->hasPermission(userId, roleIds, permission)) {
        ERP::Logger::Logger::getInstance().warning("BaseService", "Permission denied for user " + userId + ": " + permission);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::Forbidden, "Permission denied: " + permission, errorMessage);
        return false;
    }
    return true;
}

// recordAuditLog signature updated to accept std::map<string, any>
void BaseService::recordAuditLog(
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
        ERP::Logger::Logger::getInstance().warning("BaseService", "AuditLogService is null. Cannot record audit log.");
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

std::string BaseService::getCurrentSessionId() {
    // This is a placeholder. In a real system, you'd get the current session ID
    // from an AuthenticationService or a global context/thread-local storage.
    // Assuming securityManager_ can provide AuthenticationService which can give session ID.
    if (securityManager_ && securityManager_->getAuthenticationService()) {
        // This is a simplified call; actual AuthenticationService might have a getSession() method
        // or session details linked to the current user context.
        return "system_generated_session_id"; // Placeholder
    }
    return "unknown_session";
}

} // namespace Services
} // namespace Common
} // namespace ERP