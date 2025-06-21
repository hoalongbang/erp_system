// Modules/Manufacturing/Service/MaintenanceManagementService.cpp
#include "MaintenanceManagementService.h" // Standard includes
#include "MaintenanceManagement.h"        // MaintenanceManagement DTOs
#include "Asset.h"                        // Asset DTO
#include "Event.h"                        // Event DTO
#include "ConnectionPool.h"               // ConnectionPool
#include "DBConnection.h"                 // DBConnection
#include "Common.h"                       // Common Enums/Constants
#include "Utils.h"                        // Utility functions
#include "DateUtils.h"                    // Date utility functions
#include "AutoRelease.h"                  // RAII helper
#include "ISecurityManager.h"             // Security Manager interface
#include "UserService.h"                  // User Service (for audit logging)
#include "AssetManagementService.h"       // AssetManagementService (for asset validation)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
namespace Manufacturing {
namespace Services {

MaintenanceManagementService::MaintenanceManagementService(
    std::shared_ptr<DAOs::MaintenanceManagementDAO> maintenanceManagementDAO,
    std::shared_ptr<Asset::Services::IAssetManagementService> assetManagementService, // Dependency
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
      maintenanceManagementDAO_(maintenanceManagementDAO),
      assetManagementService_(assetManagementService) { // Initialize specific dependencies
    
    if (!maintenanceManagementDAO_ || !assetManagementService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "MaintenanceManagementService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ quản lý bảo trì.");
        ERP::Logger::Logger::getInstance().critical("MaintenanceManagementService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("MaintenanceManagementService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> MaintenanceManagementService::createMaintenanceRequest(
    const ERP::Manufacturing::DTO::MaintenanceRequestDTO& requestDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Attempting to create maintenance request for asset: " + requestDTO.assetId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.CreateMaintenanceRequest", "Bạn không có quyền tạo yêu cầu bảo trì.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (requestDTO.assetId.empty() || requestDTO.requestType == ERP::Manufacturing::DTO::MaintenanceRequestType::UNKNOWN || requestDTO.priority == ERP::Manufacturing::DTO::MaintenancePriority::UNKNOWN) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Invalid input for maintenance request creation (missing asset ID, type, or priority).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "MaintenanceManagementService: Invalid input for request creation.", "Thông tin yêu cầu bảo trì không đầy đủ.");
        return std::nullopt;
    }

    // Validate Asset existence
    std::optional<ERP::Asset::DTO::AssetDTO> asset = assetManagementService_->getAssetById(requestDTO.assetId, userRoleIds);
    if (!asset || asset->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Invalid Asset ID provided or asset is not active: " + requestDTO.assetId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID tài sản không hợp lệ hoặc tài sản không hoạt động.");
        return std::nullopt;
    }

    // Validate assignedToUserId if provided
    if (requestDTO.assignedToUserId && !securityManager_->getUserService()->getUserById(*requestDTO.assignedToUserId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Assigned user " + *requestDTO.assignedToUserId + " not found.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người được giao không tồn tại.");
        return std::nullopt;
    }

    ERP::Manufacturing::DTO::MaintenanceRequestDTO newRequest = requestDTO;
    newRequest.id = ERP::Utils::generateUUID();
    newRequest.createdAt = ERP::Utils::DateUtils::now();
    newRequest.createdBy = currentUserId;
    newRequest.status = ERP::Manufacturing::DTO::MaintenanceRequestStatus::PENDING; // Default status
    newRequest.requestedDate = ERP::Utils::DateUtils::now(); // Set requested date/time

    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> createdRequest = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!maintenanceManagementDAO_->createMaintenanceRequest(newRequest)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("MaintenanceManagementService: Failed to create maintenance request in DAO.");
                return false;
            }
            createdRequest = newRequest;
            // Optionally, publish event
            eventBus_.publish(std::make_shared<EventBus::MaintenanceRequestCreatedEvent>(newRequest.id, newRequest.assetId, newRequest.requestType));
            return true;
        },
        "MaintenanceManagementService", "createMaintenanceRequest"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Maintenance request " + newRequest.id + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "MaintenanceRequest", newRequest.id, "MaintenanceRequest", newRequest.assetId,
                       std::nullopt, newRequest.toMap(), "Maintenance request created.");
        return createdRequest;
    }
    return std::nullopt;
}

std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> MaintenanceManagementService::getMaintenanceRequestById(
    const std::string& requestId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("MaintenanceManagementService: Retrieving maintenance request by ID: " + requestId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewMaintenanceManagement", "Bạn không có quyền xem yêu cầu bảo trì.")) {
        return std::nullopt;
    }

    return maintenanceManagementDAO_->getMaintenanceRequestById(requestId); // Specific DAO method
}

std::vector<ERP::Manufacturing::DTO::MaintenanceRequestDTO> MaintenanceManagementService::getAllMaintenanceRequests(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Retrieving all maintenance requests with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewMaintenanceManagement", "Bạn không có quyền xem tất cả yêu cầu bảo trì.")) {
        return {};
    }

    return maintenanceManagementDAO_->getMaintenanceRequests(filter); // Specific DAO method
}

bool MaintenanceManagementService::updateMaintenanceRequest(
    const ERP::Manufacturing::DTO::MaintenanceRequestDTO& requestDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Attempting to update maintenance request: " + requestDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.UpdateMaintenanceRequest", "Bạn không có quyền cập nhật yêu cầu bảo trì.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> oldRequestOpt = maintenanceManagementDAO_->getMaintenanceRequestById(requestDTO.id); // Specific DAO method
    if (!oldRequestOpt) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Maintenance request with ID " + requestDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu bảo trì cần cập nhật.");
        return false;
    }

    // Validate Asset existence
    if (requestDTO.assetId != oldRequestOpt->assetId) { // Only check if changed
        std::optional<ERP::Asset::DTO::AssetDTO> asset = assetManagementService_->getAssetById(requestDTO.assetId, userRoleIds);
        if (!asset || asset->status != ERP::Common::EntityStatus::ACTIVE) {
            ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Invalid Asset ID provided for update or asset is not active: " + requestDTO.assetId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID tài sản không hợp lệ hoặc tài sản không hoạt động.");
            return false;
        }
    }
    // Validate assignedToUserId if provided
    if (requestDTO.assignedToUserId && !securityManager_->getUserService()->getUserById(*requestDTO.assignedToUserId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Assigned user " + *requestDTO.assignedToUserId + " not found.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người được giao không tồn tại.");
        return std::nullopt;
    }

    ERP::Manufacturing::DTO::MaintenanceRequestDTO updatedRequest = requestDTO;
    updatedRequest.updatedAt = ERP::Utils::DateUtils::now();
    updatedRequest.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!maintenanceManagementDAO_->updateMaintenanceRequest(updatedRequest)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("MaintenanceManagementService: Failed to update maintenance request " + updatedRequest.id + " in DAO.");
                return false;
            }
            // Optionally, publish event
            eventBus_.publish(std::make_shared<EventBus::MaintenanceRequestUpdatedEvent>(updatedRequest.id, updatedRequest.assetId, updatedRequest.requestType));
            return true;
        },
        "MaintenanceManagementService", "updateMaintenanceRequest"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Maintenance request " + updatedRequest.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "MaintenanceRequest", updatedRequest.id, "MaintenanceRequest", updatedRequest.assetId,
                       oldRequestOpt->toMap(), updatedRequest.toMap(), "Maintenance request updated.");
        return true;
    }
    return false;
}

bool MaintenanceManagementService::updateMaintenanceRequestStatus(
    const std::string& requestId,
    ERP::Manufacturing::DTO::MaintenanceRequestStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Attempting to update status for maintenance request: " + requestId + " to " + ERP::Manufacturing::DTO::MaintenanceRequestDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.UpdateMaintenanceRequestStatus", "Bạn không có quyền cập nhật trạng thái yêu cầu bảo trì.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> requestOpt = maintenanceManagementDAO_->getMaintenanceRequestById(requestId); // Specific DAO method
    if (!requestOpt) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Maintenance request with ID " + requestId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu bảo trì để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Manufacturing::DTO::MaintenanceRequestDTO oldRequest = *requestOpt;
    if (oldRequest.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Maintenance request " + requestId + " is already in status " + ERP::Manufacturing::DTO::MaintenanceRequestDTO().getStatusString(newStatus) + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from COMPLETED to IN_PROGRESS.

    ERP::Manufacturing::DTO::MaintenanceRequestDTO updatedRequest = oldRequest;
    updatedRequest.status = newStatus;
    updatedRequest.updatedAt = ERP::Utils::DateUtils::now();
    updatedRequest.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!maintenanceManagementDAO_->updateMaintenanceRequest(updatedRequest)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("MaintenanceManagementService: Failed to update status for maintenance request " + requestId + " in DAO.");
                return false;
            }
            // Optionally, publish event for status change
            eventBus_.publish(std::make_shared<EventBus::MaintenanceRequestStatusChangedEvent>(requestId, newStatus));
            return true;
        },
        "MaintenanceManagementService", "updateMaintenanceRequestStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Status for maintenance request " + requestId + " updated successfully to " + updatedRequest.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "MaintenanceRequestStatus", requestId, "MaintenanceRequest", oldRequest.assetId,
                       oldRequest.toMap(), updatedRequest.toMap(), "Maintenance request status changed to " + updatedRequest.getStatusString() + ".");
        return true;
    }
    return false;
}

bool MaintenanceManagementService::deleteMaintenanceRequest(
    const std::string& requestId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Attempting to delete maintenance request: " + requestId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.DeleteMaintenanceRequest", "Bạn không có quyền xóa yêu cầu bảo trì.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> requestOpt = maintenanceManagementDAO_->getMaintenanceRequestById(requestId); // Specific DAO method
    if (!requestOpt) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Maintenance request with ID " + requestId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu bảo trì cần xóa.");
        return false;
    }

    ERP::Manufacturing::DTO::MaintenanceRequestDTO requestToDelete = *requestOpt;

    // Additional checks: Prevent deletion if request has associated activities
    std::map<std::string, std::any> activityFilter;
    activityFilter["maintenance_request_id"] = requestId;
    if (maintenanceManagementDAO_->countMaintenanceActivities(activityFilter) > 0) { // Specific DAO method
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Cannot delete maintenance request " + requestId + " as it has associated activities.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa yêu cầu bảo trì có hoạt động liên quan.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!maintenanceManagementDAO_->removeMaintenanceRequest(requestId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("MaintenanceManagementService: Failed to delete maintenance request " + requestId + " in DAO.");
                return false;
            }
            return true;
        },
        "MaintenanceManagementService", "deleteMaintenanceRequest"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Maintenance request " + requestId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "MaintenanceRequest", requestId, "MaintenanceRequest", requestToDelete.assetId,
                       requestToDelete.toMap(), std::nullopt, "Maintenance request deleted.");
        return true;
    }
    return false;
}

std::optional<ERP::Manufacturing::DTO::MaintenanceActivityDTO> MaintenanceManagementService::recordMaintenanceActivity(
    const ERP::Manufacturing::DTO::MaintenanceActivityDTO& activityDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Attempting to record maintenance activity for request: " + activityDTO.maintenanceRequestId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.RecordMaintenanceActivity", "Bạn không có quyền ghi nhận hoạt động bảo trì.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (activityDTO.maintenanceRequestId.empty() || activityDTO.activityDescription.empty() || activityDTO.durationHours <= 0) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Invalid input for activity recording.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin hoạt động bảo trì không đầy đủ hoặc không hợp lệ.");
        return std::nullopt;
    }

    // Validate Maintenance Request existence
    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> request = maintenanceManagementDAO_->getMaintenanceRequestById(activityDTO.maintenanceRequestId);
    if (!request || request->status == ERP::Manufacturing::DTO::MaintenanceRequestStatus::CANCELLED || request->status == ERP::Manufacturing::DTO::MaintenanceRequestStatus::REJECTED) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Invalid Maintenance Request ID provided or request is not active: " + activityDTO.maintenanceRequestId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Yêu cầu bảo trì không hợp lệ hoặc không còn hiệu lực.");
        return std::nullopt;
    }

    // Validate performedByUserId
    if (!securityManager_->getUserService()->getUserById(activityDTO.performedByUserId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Performed by user " + activityDTO.performedByUserId + " not found.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người thực hiện không tồn tại.");
        return std::nullopt;
    }

    ERP::Manufacturing::DTO::MaintenanceActivityDTO newActivity = activityDTO;
    newActivity.id = ERP::Utils::generateUUID();
    newActivity.createdAt = ERP::Utils::DateUtils::now();
    newActivity.createdBy = currentUserId;
    newActivity.status = ERP::Common::EntityStatus::ACTIVE; // Default active

    std::optional<ERP::Manufacturing::DTO::MaintenanceActivityDTO> recordedActivity = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!maintenanceManagementDAO_->createMaintenanceActivity(newActivity)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("MaintenanceManagementService: Failed to create maintenance activity in DAO.");
                return false;
            }
            recordedActivity = newActivity;
            // Optionally, update the status of the parent maintenance request (e.g., to IN_PROGRESS or COMPLETED)
            if (request->status != ERP::Manufacturing::DTO::MaintenanceRequestStatus::IN_PROGRESS &&
                request->status != ERP::Manufacturing::DTO::MaintenanceRequestStatus::COMPLETED) {
                updateMaintenanceRequestStatus(request->id, ERP::Manufacturing::DTO::MaintenanceRequestStatus::IN_PROGRESS, currentUserId, userRoleIds);
            }
            eventBus_.publish(std::make_shared<EventBus::MaintenanceActivityRecordedEvent>(newActivity.id, newActivity.maintenanceRequestId));
            return true;
        },
        "MaintenanceManagementService", "recordMaintenanceActivity"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Maintenance activity " + newActivity.id + " recorded successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::PROCESS_END, ERP::Common::LogSeverity::INFO, // Could be specialized activity type
                       "Manufacturing", "MaintenanceActivity", newActivity.id, "MaintenanceActivity", newActivity.maintenanceRequestId,
                       std::nullopt, newActivity.toMap(), "Maintenance activity recorded.");
        return recordedActivity;
    }
    return std::nullopt;
}

std::vector<ERP::Manufacturing::DTO::MaintenanceActivityDTO> MaintenanceManagementService::getMaintenanceActivitiesByRequest(
    const std::string& requestId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementService: Retrieving maintenance activities for request ID: " + requestId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewMaintenanceActivities", "Bạn không có quyền xem hoạt động bảo trì.")) {
        return {};
    }

    // Validate Maintenance Request existence
    if (!maintenanceManagementDAO_->getMaintenanceRequestById(requestId)) {
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementService: Maintenance Request " + requestId + " not found when getting activities.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Yêu cầu bảo trì không tồn tại.");
        return {};
    }

    return maintenanceManagementDAO_->getMaintenanceActivitiesByRequestId(requestId); // Specific DAO method
}

} // namespace Services
} // namespace Manufacturing
} // namespace ERP