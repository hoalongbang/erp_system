// Modules/Common/Service/BaseService.h
#ifndef MODULES_COMMON_SERVICE_BASESERVICE_H
#define MODULES_COMMON_SERVICE_BASESERVICE_H
#include <memory>       // For std::shared_ptr
#include <string>       // For std::string
#include <vector>       // For std::vector
#include <functional>   // For std::function
#include <optional>     // For std::optional
#include <map>          // For std::map<std::string, std::any>
#include <any>          // For std::any in map values

// Rút gọn các include paths
#include "ConnectionPool.h"      // Database
#include "DBConnection.h"        // Database
#include "ISecurityManager.h"    // Security
#include "IAuthorizationService.h" // Security
#include "IAuditLogService.h"    // Security
#include "Logger.h"              // Logging
#include "ErrorHandler.h"        // Error Handling
#include "Common.h"              // Common enums/constants
#include "AutoRelease.h"         // Utils

// Forward declarations to avoid circular dependencies
namespace ERP { namespace Security { class ISecurityManager; }}
namespace ERP { namespace Security { namespace Service { class IAuthorizationService; class IAuditLogService; }}}
namespace ERP { namespace Database { class ConnectionPool; class DBConnection; }}

namespace ERP {
namespace Common {
namespace Services {

/**
 * @brief Base class for all business logic services.
 * This class provides common functionalities such as permission checking,
 * transaction management, and audit logging, reducing code duplication
 * across different service implementations.
 */
class BaseService {
protected:
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService_;
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService_;
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool_;
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager_; // Can be used to get other services

    /**
     * @brief Constructor for BaseService.
     * @param authorizationService Shared pointer to the AuthorizationService.
     * @param auditLogService Shared pointer to the AuditLogService.
     * @param connectionPool Shared pointer to the ConnectionPool.
     */
    BaseService(
        std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
        std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
        std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
        std::shared_ptr<ERP::Security::ISecurityManager> securityManager = nullptr // Optional
    );

    /**
     * @brief Helper to check user permission for a given action.
     * @param userId The ID of the user.
     * @param roleIds The list of role IDs the user belongs to.
     * @param permission The permission string to check (e.g., "Module.Action").
     * @param errorMessage The message to display/log if permission is denied.
     * @return True if the user has the permission, false otherwise.
     */
    bool checkPermission(const std::string& userId, const std::vector<std::string>& roleIds,
                         const std::string& permission, const std::string& errorMessage);

    /**
     * @brief Records an audit log entry.
     * This method is a convenience wrapper for auditLogService_->recordAuditLog.
     * @param userId The ID of the user performing the action.
     * @param userName The name of the user performing the action.
     * @param sessionId The current session ID.
     * @param actionType The type of audit action.
     * @param severity The severity of the audit log.
     * @param module The module related to the action.
     * @param subModule The submodule/feature related to the action.
     * @param entityId Optional ID of the entity affected.
     * @param entityType Optional type of the entity affected.
     * @param entityName Optional name of the entity affected.
     * @param ipAddress Optional IP address.
     * @param userAgent Optional user agent string.
     * @param workstationId Optional workstation ID.
     * @param productionLineId Optional production line ID.
     * @param shiftId Optional shift ID.
     * @param batchNumber Optional batch number.
     * @param partNumber Optional part number.
     * @param beforeData Optional std::map<std::string, std::any> representing data before change. (Changed type)
     * @param afterData Optional std::map<std::string, std::any> representing data after change. (Changed type)
     * @param changeReason Optional reason for the change.
     * @param metadata Optional additional metadata (Changed type).
     * @param comments Optional comments.
     * @param approvalId Optional approval ID.
     * @param isCompliant True if compliant, false otherwise.
     * @param complianceNote Optional compliance note.
     */
    void recordAuditLog(
        const std::string& userId, const std::string& userName, const std::string& sessionId,
        ERP::Security::DTO::AuditActionType actionType, ERP::Common::LogSeverity severity,
        const std::string& module, const std::string& subModule,
        const std::optional<std::string>& entityId = std::nullopt, const std::optional<std::string>& entityType = std::nullopt, const std::optional<std::string>& entityName = std::nullopt,
        const std::optional<std::string>& ipAddress = std::nullopt, const std::optional<std::string>& userAgent = std::nullopt, const std::optional<std::string>& workstationId = std::nullopt,
        const std::optional<std::string>& productionLineId = std::nullopt, const std::optional<std::string>& shiftId = std::nullopt, const std::optional<std::string>& batchNumber = std::nullopt, const std::optional<std::string>& partNumber = std::nullopt,
        const std::optional<std::map<std::string, std::any>>& beforeData = std::nullopt, const std::optional<std::map<std::string, std::any>>& afterData = std::nullopt,
        const std::optional<std::string>& changeReason = std::nullopt, const std::map<std::string, std::any>& metadata = {}, // Changed type
        const std::optional<std::string>& comments = std::nullopt, const std::optional<std::string>& approvalId = std::nullopt,
        bool isCompliant = true, const std::optional<std::string>& complianceNote = std::nullopt);

    /**
     * @brief Executes an operation within a database transaction.
     * Manages transaction begin, commit, rollback, and connection release.
     * @tparam Func The type of the callable object (lambda, function pointer) representing the operation.
     * @param operation The callable object that takes a shared_ptr<DBConnection> as argument.
     * @param serviceName The name of the service calling this (for logging).
     * @param operationName The name of the specific operation (for logging).
     * @return True if the operation and transaction succeeded, false otherwise.
     */
    template<typename Func>
    bool executeTransaction(Func operation, const std::string& serviceName, const std::string& operationName);

    /**
     * @brief Gets the current session ID for audit logging purposes.
     * This is a placeholder and should be implemented to retrieve the actual session ID.
     * @return The current session ID as a string.
     */
    std::string getCurrentSessionId(); // Placeholder, might need a proper SessionService injected

public:
    virtual ~BaseService() = default;
};

// Template method implementation must be in header or .tpp file
template<typename Func>
bool BaseService::executeTransaction(Func operation, const std::string& serviceName, const std::string& operationName) {
    std::shared_ptr<ERP::Database::DBConnection> db = connectionPool_->getConnection();
    ERP::Utils::AutoRelease releaseGuard([&]() {
        if (db) {
            connectionPool_->releaseConnection(db);
        }
    });
    if (!db) {
        ERP::Logger::Logger::getInstance().critical(serviceName, "Database connection is null. Cannot perform " + operationName + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "Database connection is null.", "Lỗi hệ thống: Không có kết nối cơ sở dữ liệu.");
        return false;
    }
    try {
        db->beginTransaction();
        bool success = operation(db); // Call lambda containing business logic
        if (success) {
            db->commitTransaction();
            ERP::Logger::Logger::getInstance().info(serviceName, operationName + " completed successfully.");
        } else {
            db->rollbackTransaction();
            ERP::Logger::Logger::getInstance().error(serviceName, operationName + " failed. Transaction rolled back.");
            // ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, operationName + " failed.", "Thao tác không thành công.");
            // Error handling already done by the specific service methods or DAOs if they use executeDbOperation/queryDbOperation
        }
        return success;
    } catch (const std::exception& e) {
        db->rollbackTransaction();
        ERP::Logger::Logger::getInstance().critical(serviceName, "Exception during " + operationName + ": " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Lỗi trong quá trình " + operationName + ": " + std::string(e.what()));
        return false;
    }
}

} // namespace Services
} // namespace Common
} // namespace ERP
#endif // MODULES_COMMON_SERVICE_BASESERVICE_H