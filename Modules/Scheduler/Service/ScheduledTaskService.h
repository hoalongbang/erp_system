// Modules/Scheduler/Service/ScheduledTaskService.h
#ifndef MODULES_SCHEDULER_SERVICE_SCHEDULEDTASKSERVICE_H
#define MODULES_SCHEDULER_SERVICE_SCHEDULEDTASKSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "ScheduledTask.h"      // Đã rút gọn include
#include "ScheduledTaskDAO.h"   // Đã rút gọn include
#include "ISecurityManager.h"   // Đã rút gọn include
#include "EventBus.h"           // Đã rút gọn include
#include "Logger.h"             // Đã rút gọn include
#include "ErrorHandler.h"       // Đã rút gọn include
#include "Common.h"             // Đã rút gọn include
#include "Utils.h"              // Đã rút gọn include
#include "DateUtils.h"          // Đã rút gọn include

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
     * @brief Deletes a scheduled task record by ID.
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
/**
 * @brief Default implementation of IScheduledTaskService.
 * This class uses ScheduledTaskDAO and ISecurityManager.
 */
class ScheduledTaskService : public IScheduledTaskService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for ScheduledTaskService.
     * @param scheduledTaskDAO Shared pointer to ScheduledTaskDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    ScheduledTaskService(std::shared_ptr<DAOs::ScheduledTaskDAO> scheduledTaskDAO,
                         std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                         std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                         std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                         std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> createScheduledTask(
        const ERP::Scheduler::DTO::ScheduledTaskDTO& scheduledTaskDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> getScheduledTaskById(
        const std::string& scheduledTaskId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> getAllScheduledTasks(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateScheduledTask(
        const ERP::Scheduler::DTO::ScheduledTaskDTO& scheduledTaskDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateScheduledTaskStatus(
        const std::string& scheduledTaskId,
        ERP::Scheduler::DTO::ScheduledTaskStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteScheduledTask(
        const std::string& scheduledTaskId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::ScheduledTaskDAO> scheduledTaskDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Old private helper functions removed as they are now in BaseService
};
} // namespace Services
} // namespace Scheduler
} // namespace ERP
#endif // MODULES_SCHEDULER_SERVICE_SCHEDULEDTASKSERVICE_H