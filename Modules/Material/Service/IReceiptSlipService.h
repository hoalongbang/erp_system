// Modules/Material/Service/IReceiptSlipService.h
#ifndef MODULES_MATERIAL_SERVICE_IRECEIPTSLIPSERVICE_H
#define MODULES_MATERIAL_SERVICE_IRECEIPTSLIPSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "ReceiptSlip.h"          // DTO
#include "ReceiptSlipDetail.h"    // DTO
#include "Common.h"               // Enum Common
#include "BaseService.h"          // Base Service

namespace ERP {
namespace Material {
namespace Services {

/**
 * @brief IReceiptSlipService interface defines operations for managing material receipt slips (phiếu nhập kho).
 */
class IReceiptSlipService {
public:
    virtual ~IReceiptSlipService() = default;
    /**
     * @brief Creates a new material receipt slip.
     * @param receiptSlipDTO DTO containing new receipt information.
     * @param receiptSlipDetails Vector of ReceiptSlipDetailDTOs for the slip.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ReceiptSlipDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::ReceiptSlipDTO> createReceiptSlip(
        const ERP::Material::DTO::ReceiptSlipDTO& receiptSlipDTO,
        const std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO>& receiptSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves receipt slip information by ID.
     * @param receiptSlipId ID of the receipt slip to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ReceiptSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::ReceiptSlipDTO> getReceiptSlipById(
        const std::string& receiptSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves receipt slip information by receipt number.
     * @param receiptNumber Receipt number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ReceiptSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::ReceiptSlipDTO> getReceiptSlipByNumber(
        const std::string& receiptNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all receipt slips or slips matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ReceiptSlipDTOs.
     */
    virtual std::vector<ERP::Material::DTO::ReceiptSlipDTO> getAllReceiptSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates receipt slip information.
     * @param receiptSlipDTO DTO containing updated receipt information (must have ID).
     * @param receiptSlipDetails Vector of ReceiptSlipDetailDTOs for the slip (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateReceiptSlip(
        const ERP::Material::DTO::ReceiptSlipDTO& receiptSlipDTO,
        const std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO>& receiptSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a receipt slip.
     * @param receiptSlipId ID of the receipt slip.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateReceiptSlipStatus(
        const std::string& receiptSlipId,
        ERP::Material::DTO::ReceiptSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a receipt slip record by ID (soft delete).
     * @param receiptSlipId ID of the receipt slip to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteReceiptSlip(
        const std::string& receiptSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records actual received quantity for a specific receipt slip detail.
     * This also creates an inventory transaction and cost layer.
     * @param detailId ID of the receipt slip detail.
     * @param receivedQuantity Actual quantity received.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if successful, false otherwise.
     */
    virtual bool recordReceivedQuantity(
        const std::string& detailId,
        double receivedQuantity,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all details for a specific receipt slip.
     * @param receiptSlipId ID of the receipt slip.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of ReceiptSlipDetailDTOs.
     */
    virtual std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> getReceiptSlipDetails(
        const std::string& receiptSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};

} // namespace Services
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_SERVICE_IRECEIPTSLIPSERVICE_H