// Modules/Scheduler/Service/ITaskExecutionLogService.h
#ifndef MODULES_SCHEDULER_SERVICE_ITASKEXECUTIONLOGSERVICE_H
#define MODULES_SCHEDULER_SERVICE_ITASKEXECUTIONLOGSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "TaskExecutionLog.h" // DTO
#include "Common.h"           // Enum Common
#include "BaseService.h"      // Base Service

namespace ERP {
namespace Scheduler {
namespace Services {

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

} // namespace Services
} // namespace Scheduler
} // namespace ERP
#endif // MODULES_SCHEDULER_SERVICE_ITASKEXECUTIONLOGSERVICE_H