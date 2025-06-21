// Modules/Report/Service/IReportService.h
#ifndef MODULES_REPORT_SERVICE_IREPORTSERVICE_H
#define MODULES_REPORT_SERVICE_IREPORTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "Report.h"        // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

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
     * @brief Deletes a report request record by ID (soft delete).
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

} // namespace Services
} // namespace Report
} // namespace ERP
#endif // MODULES_REPORT_SERVICE_IREPORTSERVICE_H