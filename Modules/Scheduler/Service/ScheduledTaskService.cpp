// Modules/Scheduler/Service/ScheduledTaskService.cpp
#include "ScheduledTaskService.h" // Standard includes
#include "ScheduledTask.h"
#include "Event.h"
#include "ConnectionPool.h"
#include "DBConnection.h"
#include "Common.h"
#include "Utils.h"
#include "DateUtils.h"
#include "AutoRelease.h"
#include "ISecurityManager.h"
#include "UserService.h"
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
// #include "DTOUtils.h" // Not needed here for QJsonObject conversions anymore

namespace ERP {
namespace Scheduler {
namespace Services {

ScheduledTaskService::ScheduledTaskService(
    std::shared_ptr<DAOs::ScheduledTaskDAO> scheduledTaskDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
      scheduledTaskDAO_(scheduledTaskDAO) {
    if (!scheduledTaskDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ScheduledTaskService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ tác vụ được lên lịch.");
        ERP::Logger::Logger::getInstance().critical("ScheduledTaskService: Injected ScheduledTaskDAO is null.");
        throw std::runtime_error("ScheduledTaskService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> ScheduledTaskService::createScheduledTask(
    const ERP::Scheduler::DTO::ScheduledTaskDTO& scheduledTaskDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Attempting to create scheduled task: " + scheduledTaskDTO.taskName + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.CreateScheduledTask", "Bạn không có quyền tạo tác vụ được lên lịch.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (scheduledTaskDTO.taskName.empty() || scheduledTaskDTO.taskType.empty()) {
        ERP::Logger::Logger::getInstance().warning("ScheduledTaskService: Invalid input for scheduled task creation (empty name or type).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ScheduledTaskService: Invalid input for scheduled task creation.", "Thông tin tác vụ không đầy đủ.");
        return std::nullopt;
    }

    ERP::Scheduler::DTO::ScheduledTaskDTO newScheduledTask = scheduledTaskDTO;
    newScheduledTask.id = ERP::Utils::generateUUID();
    newScheduledTask.createdAt = ERP::Utils::DateUtils::now();
    newScheduledTask.createdBy = currentUserId;
    newScheduledTask.status = ERP::Scheduler::DTO::ScheduledTaskStatus::ACTIVE; // Default status

    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> createdScheduledTask = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!scheduledTaskDAO_->create(newScheduledTask)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("ScheduledTaskService: Failed to create scheduled task " + newScheduledTask.taskName + " in DAO.");
                return false;
            }
            createdScheduledTask = newScheduledTask;
            // Optionally, publish an event for new scheduled task (e.g., for TaskEngine to pick up)
            // eventBus_.publish(std::make_shared<EventBus::ScheduledTaskCreatedEvent>(newScheduledTask));
            return true;
        },
        "ScheduledTaskService", "createScheduledTask"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Scheduled task " + newScheduledTask.taskName + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Scheduler", "ScheduledTask", newScheduledTask.id, "ScheduledTask", newScheduledTask.taskName,
                       std::nullopt, newScheduledTask.toMap(), "Scheduled task created."); // Passed map directly
        return createdScheduledTask;
    }
    return std::nullopt;
}

std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> ScheduledTaskService::getScheduledTaskById(
    const std::string& scheduledTaskId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ScheduledTaskService: Retrieving scheduled task by ID: " + scheduledTaskId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.ViewScheduledTasks", "Bạn không có quyền xem tác vụ được lên lịch.")) {
        return std::nullopt;
    }

    return scheduledTaskDAO_->getById(scheduledTaskId); // Using getById from DAOBase template
}

std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> ScheduledTaskService::getAllScheduledTasks(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Retrieving all scheduled tasks with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.ViewAllScheduledTasks", "Bạn không có quyền xem tất cả tác vụ được lên lịch.")) {
        return {};
    }

    return scheduledTaskDAO_->get(filter); // Using get from DAOBase template
}

bool ScheduledTaskService::updateScheduledTask(
    const ERP::Scheduler::DTO::ScheduledTaskDTO& scheduledTaskDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Attempting to update scheduled task: " + scheduledTaskDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.UpdateScheduledTask", "Bạn không có quyền cập nhật tác vụ được lên lịch.")) {
        return false;
    }

    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> oldScheduledTaskOpt = scheduledTaskDAO_->getById(scheduledTaskDTO.id); // Using getById from DAOBase
    if (!oldScheduledTaskOpt) {
        ERP::Logger::Logger::getInstance().warning("ScheduledTaskService: Scheduled task with ID " + scheduledTaskDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tác vụ được lên lịch cần cập nhật.");
        return false;
    }

    // Additional validation if necessary (e.g., cron expression format)

    ERP::Scheduler::DTO::ScheduledTaskDTO updatedScheduledTask = scheduledTaskDTO;
    updatedScheduledTask.updatedAt = ERP::Utils::DateUtils::now();
    updatedScheduledTask.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!scheduledTaskDAO_->update(updatedScheduledTask)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ScheduledTaskService: Failed to update scheduled task " + updatedScheduledTask.id + " in DAO.");
                return false;
            }
            return true;
        },
        "ScheduledTaskService", "updateScheduledTask"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Scheduled task " + updatedScheduledTask.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Scheduler", "ScheduledTask", updatedScheduledTask.id, "ScheduledTask", updatedScheduledTask.taskName,
                       oldScheduledTaskOpt->toMap(), updatedScheduledTask.toMap(), "Scheduled task updated."); // Passed map directly
        return true;
    }
    return false;
}

bool ScheduledTaskService::updateScheduledTaskStatus(
    const std::string& scheduledTaskId,
    ERP::Scheduler::DTO::ScheduledTaskStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Attempting to update status for scheduled task: " + scheduledTaskId + " to " + ERP::Scheduler::DTO::ScheduledTaskDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.UpdateScheduledTaskStatus", "Bạn không có quyền cập nhật trạng thái tác vụ được lên lịch.")) {
        return false;
    }

    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> scheduledTaskOpt = scheduledTaskDAO_->getById(scheduledTaskId); // Using getById from DAOBase
    if (!scheduledTaskOpt) {
        ERP::Logger::Logger::getInstance().warning("ScheduledTaskService: Scheduled task with ID " + scheduledTaskId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tác vụ được lên lịch để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Scheduler::DTO::ScheduledTaskDTO oldScheduledTask = *scheduledTaskOpt;
    if (oldScheduledTask.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Scheduled task " + scheduledTaskId + " is already in status " + ERP::Scheduler::DTO::ScheduledTaskDTO().getStatusString(newStatus) + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from COMPLETED to IN_PROGRESS.

    ERP::Scheduler::DTO::ScheduledTaskDTO updatedScheduledTask = oldScheduledTask;
    updatedScheduledTask.status = newStatus;
    updatedScheduledTask.updatedAt = ERP::Utils::DateUtils::now();
    updatedScheduledTask.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!scheduledTaskDAO_->update(updatedScheduledTask)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ScheduledTaskService: Failed to update status for scheduled task " + scheduledTaskId + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::ScheduledTaskStatusChangedEvent>(scheduledTaskId, newStatus));
            return true;
        },
        "ScheduledTaskService", "updateScheduledTaskStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Status for scheduled task " + scheduledTaskId + " updated successfully to " + updatedScheduledTask.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Scheduler", "ScheduledTaskStatus", scheduledTaskId, "ScheduledTask", oldScheduledTask.taskName,
                       oldScheduledTask.toMap(), updatedScheduledTask.toMap(), "Scheduled task status changed to " + updatedScheduledTask.getStatusString() + "."); // Passed map directly
        return true;
    }
    return false;
}

bool ScheduledTaskService::deleteScheduledTask(
    const std::string& scheduledTaskId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Attempting to delete scheduled task: " + scheduledTaskId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Scheduler.DeleteScheduledTask", "Bạn không có quyền xóa tác vụ được lên lịch.")) {
        return false;
    }

    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> scheduledTaskOpt = scheduledTaskDAO_->getById(scheduledTaskId); // Using getById from DAOBase
    if (!scheduledTaskOpt) {
        ERP::Logger::Logger::getInstance().warning("ScheduledTaskService: Scheduled task with ID " + scheduledTaskId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tác vụ được lên lịch cần xóa.");
        return false;
    }

    ERP::Scheduler::DTO::ScheduledTaskDTO scheduledTaskToDelete = *scheduledTaskOpt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Optionally, remove associated execution logs first
            // if (!scheduledTaskDAO_->removeTaskExecutionLogsByTaskId(scheduledTaskId)) { // This method doesn't exist in DAO
            //     ERP::Logger::Logger::getInstance().error("ScheduledTaskService: Failed to remove associated task execution logs for scheduled task " + scheduledTaskId + ".");
            //     return false;
            // }
            if (!scheduledTaskDAO_->remove(scheduledTaskId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("ScheduledTaskService: Failed to delete scheduled task " + scheduledTaskId + " in DAO.");
                return false;
            }
            return true;
        },
        "ScheduledTaskService", "deleteScheduledTask"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ScheduledTaskService: Scheduled task " + scheduledTaskId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Scheduler", "ScheduledTask", scheduledTaskId, "ScheduledTask", scheduledTaskToDelete.taskName,
                       scheduledTaskToDelete.toMap(), std::nullopt, "Scheduled task deleted."); // Passed map directly
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Scheduler
} // namespace ERP