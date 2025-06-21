// Modules/Scheduler/Service/TaskExecutionLogService.h
#ifndef MODULES_SCHEDULER_SERVICE_TASKEXECUTIONLOGSERVICE_H
#define MODULES_SCHEDULER_SERVICE_TASKEXECUTIONLOGSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"           // NEW: Kế thừa từ BaseService
#include "TaskExecutionLog.h"      // Đã rút gọn include
#include "TaskExecutionLogDAO.h"   // Đã rút gọn include
#include "ScheduledTaskService.h"  // For ScheduledTask validation
#include "ISecurityManager.h"      // Đã rút gọn include
#include "EventBus.h"              // Đã rút gọn include
#include "Logger.h"                // Đã rút gọn include
#include "ErrorHandler.h"          // Đã rút gọn include
#include "Common.h"                // Đã rút gọn include
#include "Utils.h"                 // Đã rút gọn include
#include "DateUtils.h"             // Đã rút gọn include

namespace ERP {
namespace Scheduler {
namespace Services {

// Forward declare if ScheduledTaskService is only used via pointer/reference
// class IScheduledTaskService;

/**
 * @brief ITaskExecutionLogService interface defines operations for managing task execution logs.
 */
class ITaskExecutionLogService {
public:
    virtual ~ITaskExecutionLogService() = default;
    /**
     * @brief Records a new task execution log.
     * @param taskExecutionLogDTO DTO containing new log information.
     * @param currentUserId ID of the user performing the operation (usually system or the user who triggered).
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional TaskExecutionLogDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> recordTaskExecutionLog(
        const ERP::Scheduler::DTO::TaskExecutionLogDTO& taskExecutionLogDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves task execution log information by ID.
     * @param taskExecutionLogId ID of the log to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional TaskExecutionLogDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> getTaskExecutionLogById(
        const std::string& taskExecutionLogId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all task execution logs or logs matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching TaskExecutionLogDTOs.
     */
    virtual std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> getAllTaskExecutionLogs(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves task execution logs for a specific scheduled task.
     * @param scheduledTaskId ID of the scheduled task.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching TaskExecutionLogDTOs.
     */
    virtual std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> getTaskExecutionLogsByScheduledTaskId(
        const std::string& scheduledTaskId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates task execution log information.
     * @param taskExecutionLogDTO DTO containing updated log information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateTaskExecutionLog(
        const ERP::Scheduler::DTO::TaskExecutionLogDTO& taskExecutionLogDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a task execution log record by ID.
     * @param taskExecutionLogId ID of the log to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteTaskExecutionLog(
        const std::string& taskExecutionLogId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of ITaskExecutionLogService.
 * This class uses TaskExecutionLogDAO and ISecurityManager.
 */
class TaskExecutionLogService : public ITaskExecutionLogService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for TaskExecutionLogService.
     * @param taskExecutionLogDAO Shared pointer to TaskExecutionLogDAO.
     * @param scheduledTaskService Shared pointer to IScheduledTaskService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    TaskExecutionLogService(std::shared_ptr<DAOs::TaskExecutionLogDAO> taskExecutionLogDAO,
                            std::shared_ptr<IScheduledTaskService> scheduledTaskService,
                            std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                            std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                            std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                            std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> recordTaskExecutionLog(
        const ERP::Scheduler::DTO::TaskExecutionLogDTO& taskExecutionLogDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> getTaskExecutionLogById(
        const std::string& taskExecutionLogId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> getAllTaskExecutionLogs(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> getTaskExecutionLogsByScheduledTaskId(
        const std::string& scheduledTaskId,
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateTaskExecutionLog(
        const ERP::Scheduler::DTO::TaskExecutionLogDTO& taskExecutionLogDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteTaskExecutionLog(
        const std::string& taskExecutionLogId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::TaskExecutionLogDAO> taskExecutionLogDAO_;
    std::shared_ptr<IScheduledTaskService> scheduledTaskService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Old private helper functions removed as they are now in BaseService
};
} // namespace Services
} // namespace Scheduler
} // namespace ERP
#endif // MODULES_SCHEDULER_SERVICE_TASKEXECUTIONLOGSERVICE_H