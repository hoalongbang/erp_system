// Modules/Report/Service/ReportService.h
#ifndef MODULES_REPORT_SERVICE_REPORTSERVICE_H
#define MODULES_REPORT_SERVICE_REPORTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Report.h"           // Đã rút gọn include
#include "ReportDAO.h"        // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include
namespace ERP {
namespace Report {
namespace Services {

/**
 * @brief IReportService interface defines operations for managing reports.
 */
class IReportService {
public:
    virtual ~IReportService() = default;
    /**
     * @brief Creates a new report request.
     * @param reportRequestDTO DTO containing new report request information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ReportRequestDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Report::DTO::ReportRequestDTO> createReportRequest(
        const ERP::Report::DTO::ReportRequestDTO& reportRequestDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves report request information by ID.
     * @param reportRequestId ID of the report request to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ReportRequestDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Report::DTO::ReportRequestDTO> getReportRequestById(
        const std::string& reportRequestId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all report requests or requests matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ReportRequestDTOs.
     */
    virtual std::vector<ERP::Report::DTO::ReportRequestDTO> getAllReportRequests(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates report request information.
     * @param reportRequestDTO DTO containing updated report request information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateReportRequest(
        const ERP::Report::DTO::ReportRequestDTO& reportRequestDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a report request.
     * @param reportRequestId ID of the report request.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateReportRequestStatus(
        const std::string& reportRequestId,
        ERP::Report::DTO::ReportExecutionStatus newStatus, // Note: This uses ExecutionStatus for request status
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a report request record by ID.
     * @param reportRequestId ID of the report request to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteReportRequest(
        const std::string& reportRequestId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of IReportService.
 * This class uses ReportDAO and ISecurityManager.
 */
class ReportService : public IReportService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for ReportService.
     * @param reportDAO Shared pointer to ReportDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    ReportService(std::shared_ptr<DAOs::ReportDAO> reportDAO,
                  std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                  std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                  std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                  std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Report::DTO::ReportRequestDTO> createReportRequest(
        const ERP::Report::DTO::ReportRequestDTO& reportRequestDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Report::DTO::ReportRequestDTO> getReportRequestById(
        const std::string& reportRequestId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Report::DTO::ReportRequestDTO> getAllReportRequests(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateReportRequest(
        const ERP::Report::DTO::ReportRequestDTO& reportRequestDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateReportRequestStatus(
        const std::string& reportRequestId,
        ERP::Report::DTO::ReportExecutionStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteReportRequest(
        const std::string& reportRequestId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::ReportDAO> reportDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Old private helper functions removed as they are now in BaseService
};
} // namespace Services
} // namespace Report
} // namespace ERP
#endif // MODULES_REPORT_SERVICE_REPORTSERVICE_H