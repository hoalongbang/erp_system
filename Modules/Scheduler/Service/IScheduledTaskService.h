// Modules/Scheduler/Service/IScheduledTaskService.h
#ifndef MODULES_SCHEDULER_SERVICE_ISCHEDULEDTASKSERVICE_H
#define MODULES_SCHEDULER_SERVICE_ISCHEDULEDTASKSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "ScheduledTask.h" // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Scheduler {
namespace Services {

/**
 * @brief IScheduledTaskService interface defines operations for managing scheduled tasks.
 */
class IScheduledTaskService {
public:
    virtual ~IScheduledTaskService() = default;
    /**
     * @brief Creates a new scheduled task.
     * @param scheduledTaskDTO DTO containing new scheduled task information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ScheduledTaskDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> createScheduledTask(
        const ERP::Scheduler::DTO::ScheduledTaskDTO& scheduledTaskDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves scheduled task information by ID.
     * @param scheduledTaskId ID of the scheduled task to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ScheduledTaskDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> getScheduledTaskById(
        const std::string& scheduledTaskId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all scheduled tasks or tasks matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ScheduledTaskDTOs.
     */
    virtual std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> getAllScheduledTasks(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates scheduled task information.
     * @param scheduledTaskDTO DTO containing updated scheduled task information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateScheduledTask(
        const ERP::Scheduler::DTO::ScheduledTaskDTO& scheduledTaskDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a scheduled task.
     * @param scheduledTaskId ID of the scheduled task.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateScheduledTaskStatus(
        const std::string& scheduledTaskId,
        ERP::Scheduler::DTO::ScheduledTaskStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a scheduled task record by ID (soft delete).
     * @param scheduledTaskId ID of the scheduled task to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteScheduledTask(
        const std::string& scheduledTaskId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Scheduler
} // namespace ERP
#endif // MODULES_SCHEDULER_SERVICE_ISCHEDULEDTASKSERVICE_H