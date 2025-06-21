// Modules/Material/Service/IIssueSlipService.h
#ifndef MODULES_MATERIAL_SERVICE_IISSUESLIPSERVICE_H
#define MODULES_MATERIAL_SERVICE_IISSUESLIPSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "IssueSlip.h"          // DTO
#include "IssueSlipDetail.h"    // DTO
#include "Common.h"             // Enum Common
#include "BaseService.h"        // Base Service

namespace ERP {
namespace Material {
namespace Services {

/**
 * @brief IIssueSlipService interface defines operations for managing material issue slips (phiếu xuất kho).
 * This is for general material issues, potentially for sales, and distinct from manufacturing material issues.
 */
class IIssueSlipService {
public:
    virtual ~IIssueSlipService() = default;
    /**
     * @brief Creates a new material issue slip.
     * @param issueSlipDTO DTO containing new issue slip information.
     * @param issueSlipDetails Vector of IssueSlipDetailDTOs for the slip.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional IssueSlipDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::IssueSlipDTO> createIssueSlip(
        const ERP::Material::DTO::IssueSlipDTO& issueSlipDTO,
        const std::vector<ERP::Material::DTO::IssueSlipDetailDTO>& issueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves issue slip information by ID.
     * @param issueSlipId ID of the issue slip to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional IssueSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::IssueSlipDTO> getIssueSlipById(
        const std::string& issueSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves issue slip information by issue number.
     * @param issueNumber Issue number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional IssueSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::IssueSlipDTO> getIssueSlipByNumber(
        const std::string& issueNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all issue slips or slips matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching IssueSlipDTOs.
     */
    virtual std::vector<ERP::Material::DTO::IssueSlipDTO> getAllIssueSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates issue slip information.
     * @param issueSlipDTO DTO containing updated issue slip information (must have ID).
     * @param issueSlipDetails Vector of IssueSlipDetailDTOs for the slip (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateIssueSlip(
        const ERP::Material::DTO::IssueSlipDTO& issueSlipDTO,
        const std::vector<ERP::Material::DTO::IssueSlipDetailDTO>& issueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of an issue slip.
     * @param issueSlipId ID of the issue slip.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateIssueSlipStatus(
        const std::string& issueSlipId,
        ERP::Material::DTO::IssueSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes an issue slip record by ID (soft delete).
     * @param issueSlipId ID of the issue slip to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteIssueSlip(
        const std::string& issueSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records actual issued quantity for a specific issue slip detail.
     * This also creates an inventory transaction.
     * @param detailId ID of the issue slip detail.
     * @param issuedQuantity Actual quantity issued.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if successful, false otherwise.
     */
    virtual bool recordIssuedQuantity(
        const std::string& detailId,
        double issuedQuantity,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all details for a specific issue slip.
     * @param issueSlipId ID of the issue slip.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of IssueSlipDetailDTOs.
     */
    virtual std::vector<ERP::Material::DTO::IssueSlipDetailDTO> getIssueSlipDetails(
        const std::string& issueSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};

} // namespace Services
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_SERVICE_IISSUESLIPSERVICE_H