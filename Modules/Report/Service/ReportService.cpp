// Modules/Report/Service/ReportService.cpp
#include "ReportService.h" // Đã rút gọn include
#include "Report.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Report {
namespace Services {

ReportService::ReportService(
    std::shared_ptr<DAOs::ReportDAO> reportDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      reportDAO_(reportDAO) {
    if (!reportDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ReportService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ báo cáo.");
        ERP::Logger::Logger::getInstance().critical("ReportService: Injected ReportDAO is null.");
        throw std::runtime_error("ReportService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("ReportService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Report::DTO::ReportRequestDTO> ReportService::createReportRequest(
    const ERP::Report::DTO::ReportRequestDTO& reportRequestDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReportService: Attempting to create report request: " + reportRequestDTO.reportName + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Report.CreateReportRequest", "Bạn không có quyền tạo yêu cầu báo cáo.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (reportRequestDTO.reportName.empty() || reportRequestDTO.reportType.empty()) {
        ERP::Logger::Logger::getInstance().warning("ReportService: Invalid input for report request creation (empty name or type).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ReportService: Invalid input for report request creation.", "Thông tin yêu cầu báo cáo không đầy đủ.");
        return std::nullopt;
    }

    ERP::Report::DTO::ReportRequestDTO newReportRequest = reportRequestDTO;
    newReportRequest.id = ERP::Utils::generateUUID();
    newReportRequest.createdAt = ERP::Utils::DateUtils::now();
    newReportRequest.createdBy = currentUserId;
    newReportRequest.requestedTime = newReportRequest.createdAt; // Set requestedTime as creation time
    newReportRequest.status = ERP::Common::EntityStatus::ACTIVE; // Default status for requests (means active schedule)

    std::optional<ERP::Report::DTO::ReportRequestDTO> createdReportRequest = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!reportDAO_->create(newReportRequest)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("ReportService: Failed to create report request " + newReportRequest.reportName + " in DAO.");
                return false;
            }
            createdReportRequest = newReportRequest;
            // Optionally, publish an event for new report request (e.g., for scheduler to pick up)
            // eventBus_.publish(std::make_shared<EventBus::ReportRequestCreatedEvent>(newReportRequest));
            return true;
        },
        "ReportService", "createReportRequest"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ReportService: Report request " + newReportRequest.reportName + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Report", "ReportRequest", newReportRequest.id, "ReportRequest", newReportRequest.reportName,
                       std::nullopt, newReportRequest.toMap(), "Report request created.");
        return createdReportRequest;
    }
    return std::nullopt;
}

std::optional<ERP::Report::DTO::ReportRequestDTO> ReportService::getReportRequestById(
    const std::string& reportRequestId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ReportService: Retrieving report request by ID: " + reportRequestId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Report.ViewReportRequests", "Bạn không có quyền xem yêu cầu báo cáo.")) {
        return std::nullopt;
    }

    return reportDAO_->getById(reportRequestId); // Using getById from DAOBase template
}

std::vector<ERP::Report::DTO::ReportRequestDTO> ReportService::getAllReportRequests(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReportService: Retrieving all report requests with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Report.ViewAllReportRequests", "Bạn không có quyền xem tất cả yêu cầu báo cáo.")) {
        return {};
    }

    return reportDAO_->get(filter); // Using get from DAOBase template
}

bool ReportService::updateReportRequest(
    const ERP::Report::DTO::ReportRequestDTO& reportRequestDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReportService: Attempting to update report request: " + reportRequestDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Report.UpdateReportRequest", "Bạn không có quyền cập nhật yêu cầu báo cáo.")) {
        return false;
    }

    std::optional<ERP::Report::DTO::ReportRequestDTO> oldReportRequestOpt = reportDAO_->getById(reportRequestDTO.id); // Using getById from DAOBase
    if (!oldReportRequestOpt) {
        ERP::Logger::Logger::getInstance().warning("ReportService: Report request with ID " + reportRequestDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu báo cáo cần cập nhật.");
        return false;
    }

    // Additional validation if necessary (e.g., cron expression format)

    ERP::Report::DTO::ReportRequestDTO updatedReportRequest = reportRequestDTO;
    updatedReportRequest.updatedAt = ERP::Utils::DateUtils::now();
    updatedReportRequest.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!reportDAO_->update(updatedReportRequest)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ReportService: Failed to update report request " + updatedReportRequest.id + " in DAO.");
                return false;
            }
            return true;
        },
        "ReportService", "updateReportRequest"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ReportService: Report request " + updatedReportRequest.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Report", "ReportRequest", updatedReportRequest.id, "ReportRequest", updatedReportRequest.reportName,
                       oldReportRequestOpt->toMap(), updatedReportRequest.toMap(), "Report request updated.");
        return true;
    }
    return false;
}

bool ReportService::updateReportRequestStatus(
    const std::string& reportRequestId,
    ERP::Report::DTO::ReportExecutionStatus newStatus, // Note: This uses ExecutionStatus for request status
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReportService: Attempting to update status for report request: " + reportRequestId + " to " + ERP::Report::DTO::ReportExecutionLogDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Report.UpdateReportRequestStatus", "Bạn không có quyền cập nhật trạng thái yêu cầu báo cáo.")) {
        return false;
    }

    std::optional<ERP::Report::DTO::ReportRequestDTO> reportRequestOpt = reportDAO_->getById(reportRequestId); // Using getById from DAOBase
    if (!reportRequestOpt) {
        ERP::Logger::Logger::getInstance().warning("ReportService: Report request with ID " + reportRequestId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu báo cáo để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Report::DTO::ReportRequestDTO oldReportRequest = *reportRequestOpt;
    // Note: ReportRequestDTO uses ERP::Common::EntityStatus for its 'status' field,
    // but this method uses ERP::Report::DTO::ReportExecutionStatus.
    // This is a discrepancy that needs to be resolved.
    // Assuming newStatus refers to the _execution_ status related to the request.
    // If you intend for ReportRequestDTO to have a status like ACTIVE/INACTIVE/COMPLETED/FAILED,
    // its 'status' field type should reflect that, or a new field should be added.
    // For now, I'll update ReportExecutionStatus and maybe metadata in ReportRequestDTO.

    ERP::Report::DTO::ReportRequestDTO updatedReportRequest = oldReportRequest;
    // Update a field in metadata to reflect last execution status, or add a dedicated field in ReportRequestDTO for this.
    // For now, I'll update a dummy metadata field.
    updatedReportRequest.metadata["last_execution_status"] = static_cast<int>(newStatus);
    updatedReportRequest.updatedAt = ERP::Utils::DateUtils::now();
    updatedReportRequest.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!reportDAO_->update(updatedReportRequest)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ReportService: Failed to update status for report request " + reportRequestId + " in DAO.");
                return false;
            }
            // Optionally, publish event for status change
            // eventBus_.publish(std::make_shared<EventBus::ReportRequestStatusChangedEvent>(reportRequestId, newStatus));
            return true;
        },
        "ReportService", "updateReportRequestStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ReportService: Status for report request " + reportRequestId + " updated successfully to " + ERP::Report::DTO::ReportExecutionLogDTO().getStatusString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Report", "ReportRequestStatus", reportRequestId, "ReportRequest", oldReportRequest.reportName,
                       oldReportRequest.toMap(), updatedReportRequest.toMap(), "Report request status changed to " + ERP::Report::DTO::ReportExecutionLogDTO().getStatusString(newStatus) + ".");
        return true;
    }
    return false;
}

bool ReportService::deleteReportRequest(
    const std::string& reportRequestId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReportService: Attempting to delete report request: " + reportRequestId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Report.DeleteReportRequest", "Bạn không có quyền xóa yêu cầu báo cáo.")) {
        return false;
    }

    std::optional<ERP::Report::DTO::ReportRequestDTO> reportRequestOpt = reportDAO_->getById(reportRequestId); // Using getById from DAOBase
    if (!reportRequestOpt) {
        ERP::Logger::Logger::getInstance().warning("ReportService: Report request with ID " + reportRequestId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu báo cáo cần xóa.");
        return false;
    }

    ERP::Report::DTO::ReportRequestDTO reportRequestToDelete = *reportRequestOpt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated execution logs first
            if (!reportDAO_->removeReportExecutionLogsByRequestId(reportRequestId)) { // Specific DAO method for logs
                ERP::Logger::Logger::getInstance().error("ReportService: Failed to remove associated report execution logs for request " + reportRequestId + ".");
                return false;
            }
            if (!reportDAO_->remove(reportRequestId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("ReportService: Failed to delete report request " + reportRequestId + " in DAO.");
                return false;
            }
            return true;
        },
        "ReportService", "deleteReportRequest"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ReportService: Report request " + reportRequestId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Report", "ReportRequest", reportRequestId, "ReportRequest", reportRequestToDelete.reportName,
                       reportRequestToDelete.toMap(), std::nullopt, "Report request deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Report
} // namespace ERP