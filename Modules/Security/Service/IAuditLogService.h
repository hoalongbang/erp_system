// Modules/Security/Service/IAuditLogService.h
#ifndef MODULES_SECURITY_SERVICE_IAUDITLOGSERVICE_H
#define MODULES_SECURITY_SERVICE_IAUDITLOGSERVICE_H
#include <string>
#include <optional>
#include <vector>
#include <map>    // Now directly used in interface
#include <chrono> // For std::chrono::system_clock::time_point
#include <any>    // For std::any in map values

// Rút gọn các include paths
#include "AuditLog.h"     // DTO
#include "Common.h"       // Enum Common

// QJsonObject is NO LONGER NEEDED in this interface.
// The conversion will happen internally in AuditLogService.cpp if still required.
// #include <QJsonObject> 
// #include <QJsonDocument>

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
     * @param beforeData Optional std::map<std::string, std::any> representing data before change.
     * @param afterData Optional std::map<std::string, std::any> representing data after change.
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
        const std::optional<std::map<std::string, std::any>>& beforeData = std::nullopt, const std::optional<std::map<std::string, std::any>>& afterData = std::nullopt,
        const std::optional<std::string>& changeReason = std::nullopt, const std::map<std::string, std::any>& metadata = {}, // Changed from QJsonObject to std::map
        const std::optional<std::string>& comments = std::nullopt, const std::optional<std::string>& approvalId = std::nullopt,
        bool isCompliant = true, const std::optional<std::string>& complianceNote = std::nullopt) = 0;
};

} // namespace Services
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_IAUDITLOGSERVICE_H