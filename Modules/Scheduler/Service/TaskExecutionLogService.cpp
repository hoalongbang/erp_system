// Modules/Scheduler/Service/TaskExecutionLogService.cpp
#include "TaskExecutionLogService.h" // Standard includes
#include "TaskExecutionLog.h"
#include "Event.h"
#include "ConnectionPool.h"
#include "DBConnection.h"
#include "Common.h"
#include "Utils.h"
#include "DateUtils.h"
#include "AutoRelease.h"
#include "ScheduledTaskService.h"
#include "ISecurityManager.h"
#include "UserService.h"
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
// #include "DTOUtils.h" // Not needed here for QJsonObject conversions anymore

namespace ERP {
namespace Scheduler {
namespace Services {

TaskExecutionLogService::TaskExecutionLogService(
    std::shared_ptr<DAOs::TaskExecutionLogDAO> taskExecutionLogDAO,
    std::shared_ptr<IScheduledTaskService> scheduledTaskService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
      taskExecutionLogDAO_(taskExecutionLogDAO), scheduledTaskService_(scheduledTaskService) {
    if (!taskExecutionLogDAO_ || !scheduledTaskService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "TaskExecutionLogService: Initialized with null DAO or ScheduledTaskService.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ nhật ký thực thi tác vụ.");
        ERP::Logger::Logger::getInstance().critical("TaskExecutionLogService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("TaskExecutionLogService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("TaskExecutionLogService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> TaskExecutionLogService::recordTaskExecutionLog(
    const ERP::Scheduler::DTO::TaskExecutionLogDTO& taskExecutionLogDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("TaskExecutionLogService: Attempting to record task execution log for task: " + taskExecutionLogDTO.scheduledTaskId + " by " + currentUserId + ".");

    // This service might not always require explicit user permission if logs are recorded by system/TaskEngine itself
    // However, for manual recording or privileged operations, permission check is needed.
    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.RecordTaskExecutionLog", "Bạn không có quyền ghi nhật ký thực thi tác vụ.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (taskExecutionLogDTO.scheduledTaskId.empty()) {
        ERP::Logger::Logger::getInstance().warning("TaskExecutionLogService: Invalid input for log recording (empty scheduledTaskId).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "TaskExecutionLogService: Invalid input for log recording.", "Thông tin nhật ký tác vụ không đầy đủ.");
        return std::nullopt;
    }

    // Validate Scheduled Task existence
    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> scheduledTask = scheduledTaskService_->getScheduledTaskById(taskExecutionLogDTO.scheduledTaskId, userRoleIds);
    if (!scheduledTask || scheduledTask->status == ERP::Scheduler::DTO::ScheduledTaskStatus::INACTIVE) {
        ERP::Logger::Logger::getInstance().warning("TaskExecutionLogService: Invalid Scheduled Task ID provided or task is inactive: " + taskExecutionLogDTO.scheduledTaskId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID tác vụ được lên lịch không hợp lệ hoặc tác vụ không hoạt động.");
        return std::nullopt;
    }

    ERP::Scheduler::DTO::TaskExecutionLogDTO newExecutionLog = taskExecutionLogDTO;
    newExecutionLog.id = ERP::Utils::generateUUID();
    newExecutionLog.createdAt = ERP::Utils::DateUtils::now();
    newExecutionLog.createdBy = currentUserId; // Can be "system" for automated tasks
    newExecutionLog.eventTime = newExecutionLog.createdAt; // Set event time
    newExecutionLog.status = ERP::Common::EntityStatus::ACTIVE;

    std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> createdExecutionLog = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!taskExecutionLogDAO_->create(newExecutionLog)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("TaskExecutionLogService: Failed to create task execution log for task " + newExecutionLog.scheduledTaskId + " in DAO.");
                return false;
            }
            createdExecutionLog = newExecutionLog;
            // Optionally, update the lastRunTime and lastErrorMessage of the ScheduledTask
            // scheduledTaskService_->updateScheduledTask(updatedScheduledTask, currentUserId, userRoleIds); // Requires updateScheduledTask to accept minimal update
            return true;
        },
        "TaskExecutionLogService", "recordTaskExecutionLog"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("TaskExecutionLogService: Task execution log recorded successfully for task: " + newExecutionLog.scheduledTaskId + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::PROCESS_END, // Assuming recording means end of process
                       ERP::Common::LogSeverity::INFO, // Severity depends on log status
                       "Scheduler", "TaskExecutionLog", newExecutionLog.id, "TaskExecutionLog", newExecutionLog.scheduledTaskId,
                       std::nullopt, newExecutionLog.toMap(), "Task execution log recorded for task: " + newExecutionLog.scheduledTaskId + "."); // Passed map directly
        return createdExecutionLog;
    }
    return std::nullopt;
}

std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> TaskExecutionLogService::getTaskExecutionLogById(
    const std::string& taskExecutionLogId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("TaskExecutionLogService: Retrieving task execution log by ID: " + taskExecutionLogId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.ViewTaskExecutionLogs", "Bạn không có quyền xem nhật ký thực thi tác vụ.")) {
        return std::nullopt;
    }

    return taskExecutionLogDAO_->getById(taskExecutionLogId); // Using getById from DAOBase template
}

std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> TaskExecutionLogService::getAllTaskExecutionLogs(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("TaskExecutionLogService: Retrieving all task execution logs with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.ViewAllTaskExecutionLogs", "Bạn không có quyền xem tất cả nhật ký thực thi tác vụ.")) {
        return {};
    }

    return taskExecutionLogDAO_->get(filter); // Using get from DAOBase template
}

std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> TaskExecutionLogService::getTaskExecutionLogsByScheduledTaskId(
    const std::string& scheduledTaskId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("TaskExecutionLogService: Retrieving task execution logs for scheduled task ID: " + scheduledTaskId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.ViewTaskExecutionLogs", "Bạn không có quyền xem nhật ký thực thi tác vụ này.")) {
        return {};
    }

    std::map<std::string, std::any> filter;
    filter["scheduled_task_id"] = scheduledTaskId;

    return taskExecutionLogDAO_->get(filter); // Using get from DAOBase template
}

bool TaskExecutionLogService::updateTaskExecutionLog(
    const ERP::Scheduler::DTO::TaskExecutionLogDTO& taskExecutionLogDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("TaskExecutionLogService: Attempting to update task execution log: " + taskExecutionLogDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.UpdateTaskExecutionLog", "Bạn không có quyền cập nhật nhật ký thực thi tác vụ.")) {
        return false;
    }

    std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> oldLogOpt = taskExecutionLogDAO_->getById(taskExecutionLogDTO.id); // Using getById from DAOBase
    if (!oldLogOpt) {
        ERP::Logger::Logger::getInstance().warning("TaskExecutionLogService: Task execution log with ID " + taskExecutionLogDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy nhật ký thực thi tác vụ cần cập nhật.");
        return false;
    }

    ERP::Scheduler::DTO::TaskExecutionLogDTO updatedLog = taskExecutionLogDTO;
    updatedLog.updatedAt = ERP::Utils::DateUtils::now();
    updatedLog.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!taskExecutionLogDAO_->update(updatedLog)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("TaskExecutionLogService: Failed to update task execution log " + updatedLog.id + " in DAO.");
                return false;
            }
            return true;
        },
        "TaskExecutionLogService", "updateTaskExecutionLog"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("TaskExecutionLogService: Task execution log " + updatedLog.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Scheduler", "TaskExecutionLog", updatedLog.id, "TaskExecutionLog", updatedLog.scheduledTaskId,
                       oldLogOpt->toMap(), updatedLog.toMap(), "Task execution log updated."); // Passed map directly
        return true;
    }
    return false;
}

bool TaskExecutionLogService::deleteTaskExecutionLog(
    const std::string& taskExecutionLogId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("TaskExecutionLogService: Attempting to delete task execution log: " + taskExecutionLogId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.DeleteTaskExecutionLog", "Bạn không có quyền xóa nhật ký thực thi tác vụ.")) {
        return false;
    }

    std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> logOpt = taskExecutionLogDAO_->getById(taskExecutionLogId); // Using getById from DAOBase
    if (!logOpt) {
        ERP::Logger::Logger::getInstance().warning("TaskExecutionLogService: Task execution log with ID " + taskExecutionLogId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy nhật ký thực thi tác vụ cần xóa.");
        return false;
    }

    ERP::Scheduler::DTO::TaskExecutionLogDTO logToDelete = *logOpt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!taskExecutionLogDAO_->remove(taskExecutionLogId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("TaskExecutionLogService: Failed to delete task execution log " + taskExecutionLogId + " in DAO.");
                return false;
            }
            return true;
        },
        "TaskExecutionLogService", "deleteTaskExecutionLog"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("TaskExecutionLogService: Task execution log " + taskExecutionLogId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Scheduler", "TaskExecutionLog", taskExecutionLogId, "TaskExecutionLog", logToDelete.scheduledTaskId,
                       logToDelete.toMap(), std::nullopt, "Task execution log deleted."); // Passed map directly
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Scheduler
} // namespace ERP