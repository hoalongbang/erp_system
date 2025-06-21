// Modules/Security/Service/AuditLogService.h
#ifndef MODULES_SECURITY_SERVICE_AUDITLOGSERVICE_H
#define MODULES_SECURITY_SERVICE_AUDITLOGSERVICE_H
#include <string>
#include <optional>
#include <chrono>
#include <vector>
#include <memory>
#include <map>
#include <any> // For std::any if AuditLogDTO uses it directly
#include "AuditLog.h"     // Đã rút gọn include
#include "AuditLogDAO.h"  // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "Logger.h"       // Đã rút gọn include
#include "ErrorHandler.h" // Đã rút gọn include
#include "Common.h"       // Đã rút gọn include
#include "DateUtils.h"    // Đã rút gọn include
#include "DTOUtils.h"     // Đã rút gọn include
// For Qt's QJsonObject, QJsonDocument, etc. if still needed by this specific service
#include <QJsonObject> // Still needed for now if recordLog expects QJsonObject
#include <QJsonDocument>

namespace ERP {
namespace Security {
namespace Services {

/**
 * @brief IAuditLogService interface defines operations for recording audit logs.
 */
class IAuditLogService {
public:
    virtual ~IAuditLogService() = default;
    /**
     * @brief Records an audit log entry.
     * This is the primary method for services to log important actions and changes.
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
     * @param beforeData Optional QJsonObject representing data before change.
     * @param afterData Optional QJsonObject representing data after change.
     * @param changeReason Optional reason for the change.
     * @param metadata Optional additional metadata.
     * @param comments Optional comments.
     * @param approvalId Optional approval ID.
     * @param isCompliant True if compliant, false if not.
     * @param complianceNote Optional compliance note.
     * @return true if logging is successful, false otherwise.
     */
    virtual bool recordLog(
        const std::string& userId, const std::string& userName, const std::string& sessionId,
        ERP::Security::DTO::AuditActionType actionType, ERP::Common::LogSeverity severity,
        const std::string& module, const std::string& subModule,
        const std::optional<std::string>& entityId = std::nullopt, const std::optional<std::string>& entityType = std::nullopt, const std::optional<std::string>& entityName = std::nullopt,
        const std::optional<std::string>& ipAddress = std::nullopt, const std::optional<std::string>& userAgent = std::nullopt, const std::optional<std::string>& workstationId = std::nullopt,
        const std::optional<std::string>& productionLineId = std::nullopt, const std::optional<std::string>& shiftId = std::nullopt, const std::optional<std::string>& batchNumber = std::nullopt, const std::optional<std::string>& partNumber = std::nullopt,
        const std::optional<QJsonObject>& beforeData = std::nullopt, const std::optional<QJsonObject>& afterData = std::nullopt,
        const std::optional<std::string>& changeReason = std::nullopt, const QJsonObject& metadata = QJsonObject(),
        const std::optional<std::string>& comments = std::nullopt, const std::optional<std::string>& approvalId = std::nullopt,
        bool isCompliant = true, const std::optional<std::string>& complianceNote = std::nullopt) = 0;
};
/**
 * @brief Default implementation of IAuditLogService.
 * This class handles the persistence of audit log entries.
 */
class AuditLogService : public IAuditLogService {
public:
    /**
     * @brief Constructor for AuditLogService.
     * @param auditLogDAO Shared pointer to AuditLogDAO.
     * @param connectionPool Shared pointer to ConnectionPool.
     */
    AuditLogService(std::shared_ptr<DAOs::AuditLogDAO> auditLogDAO,
                    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);

    bool recordLog(
        const std::string& userId, const std::string& userName, const std::string& sessionId,
        ERP::Security::DTO::AuditActionType actionType, ERP::Common::LogSeverity severity,
        const std::string& module, const std::string& subModule,
        const std::optional<std::string>& entityId = std::nullopt, const std::optional<std::string>& entityType = std::nullopt, const std::optional<std::string>& entityName = std::nullopt,
        const std::optional<std::string>& ipAddress = std::nullopt, const std::optional<std::string>& userAgent = std::nullopt, const std::optional<std::string>& workstationId = std::nullopt,
        const std::optional<std::string>& productionLineId = std::nullopt, const std::optional<std::string>& shiftId = std::nullopt, const std::optional<std::string>& batchNumber = std::nullopt, const std::optional<std::string>& partNumber = std::nullopt,
        const std::optional<QJsonObject>& beforeData = std::nullopt, const std::optional<QJsonObject>& afterData = std::nullopt,
        const std::optional<std::string>& changeReason = std::nullopt, const QJsonObject& metadata = QJsonObject(),
        const std::optional<std::string>& comments = std::nullopt, const std::optional<std::string>& approvalId = std::nullopt,
        bool isCompliant = true, const std::optional<std::string>& complianceNote = std::nullopt) override;

private:
    std::shared_ptr<DAOs::AuditLogDAO> auditLogDAO_;
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool_;

    // Helper for transaction management (replicated for foundational services)
    template<typename Func>
    bool executeTransactionInternal(Func operation, const std::string& serviceName, const std::string& operationName);
};
} // namespace Services
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_AUDITLOGSERVICE_H