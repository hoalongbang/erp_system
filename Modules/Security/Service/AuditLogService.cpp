// Modules/Security/Service/AuditLogService.cpp
#include "AuditLogService.h" // Đã rút gọn include
#include "AuditLog.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Logger.h" // Đã rút gọn include
#include "ErrorHandler.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "DTOUtils.h" // For mapToJsonString (now internal)
#include <nlohmann/json.hpp> // For JSON serialization/deserialization

// Removed Qt includes as they are no longer exposed in the interface
// #include <QJsonObject>
// #include <QJsonDocument>

namespace ERP {
namespace Security {
namespace Services {

// Template method implementation for executeTransactionInternal
template<typename Func>
bool AuditLogService::executeTransactionInternal(Func operation, const std::string& serviceName, const std::string& operationName) {
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

AuditLogService::AuditLogService(
    std::shared_ptr<DAOs::AuditLogDAO> auditLogDAO,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : auditLogDAO_(auditLogDAO), connectionPool_(connectionPool) {
    if (!auditLogDAO_ || !connectionPool_) {
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "AuditLogService: Initialized with null dependencies.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ nhật ký kiểm toán.");
        ERP::Logger::Logger::getInstance().critical("AuditLogService: Injected AuditLogDAO or ConnectionPool is null.");
        throw std::runtime_error("AuditLogService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("AuditLogService: Initialized.");
}

// recordLog signature updated to accept std::map<string, any>
bool AuditLogService::recordLog(
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
    
    ERP::Security::DTO::AuditLogDTO logEntry;
    logEntry.id = ERP::Utils::generateUUID();
    logEntry.userId = userId;
    logEntry.userName = userName;
    logEntry.sessionId = sessionId;
    logEntry.actionType = actionType;
    logEntry.severity = severity;
    logEntry.module = module;
    logEntry.subModule = subModule;
    logEntry.entityId = entityId;
    logEntry.entityType = entityType;
    logEntry.entityName = entityName;
    logEntry.ipAddress = ipAddress;
    logEntry.userAgent = userAgent;
    logEntry.workstationId = workstationId;
    logEntry.productionLineId = productionLineId;
    logEntry.shiftId = shiftId;
    logEntry.batchNumber = batchNumber;
    logEntry.partNumber = partNumber;

    // Direct assignment from std::map<string, any> to std::map<string, any> in DTO
    logEntry.beforeData = beforeData;
    logEntry.afterData = afterData;
    logEntry.changeReason = changeReason;
    logEntry.metadata = metadata; // Direct assignment
    logEntry.comments = comments;
    logEntry.approvalId = approvalId;
    logEntry.isCompliant = isCompliant;
    logEntry.complianceNote = complianceNote;

    logEntry.createdAt = ERP::Utils::DateUtils::now();
    logEntry.createdBy = userId; // The user who initiated the action is the creator of the log

    ERP::Logger::Logger::getInstance().debug("AuditLogService: Recording log for action: " + logEntry.getActionTypeString() + " by " + logEntry.userName);

    bool success = executeTransactionInternal(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // AuditLogDAO uses templated DAOBase::create method
            return auditLogDAO_->create(logEntry);
        },
        "AuditLogService", "recordLog"
    );

    if (!success) {
        ERP::Logger::Logger::getInstance().error("AuditLogService: Failed to persist audit log for action: " + logEntry.getActionTypeString());
        return false;
    }
    return true;
}

} // namespace Services
} // namespace Security
} // namespace ERP