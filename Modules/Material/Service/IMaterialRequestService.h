// Modules/Material/Service/IMaterialRequestService.h
#ifndef MODULES_MATERIAL_SERVICE_IMATERIALREQUESTSERVICE_H
#define MODULES_MATERIAL_SERVICE_IMATERIALREQUESTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "MaterialRequestSlip.h"     // DTO
#include "MaterialRequestSlipDetail.h" // DTO
#include "Common.h"                // Enum Common
#include "BaseService.h"           // Base Service

namespace ERP {
namespace Material {
namespace Services {

/**
 * @brief IMaterialRequestService interface defines operations for managing material request slips.
 */
class IMaterialRequestService {
public:
    virtual ~IMaterialRequestService() = default;
    /**
     * @brief Creates a new material request slip.
     * @param requestSlipDTO DTO containing new request information.
     * @param requestSlipDetails Vector of MaterialRequestSlipDetailDTOs for the slip.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaterialRequestSlipDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> createMaterialRequestSlip(
        const ERP::Material::DTO::MaterialRequestSlipDTO& requestSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO>& requestSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves material request slip information by ID.
     * @param requestSlipId ID of the request to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaterialRequestSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> getMaterialRequestSlipById(
        const std::string& requestSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves material request slip information by request number.
     * @param requestNumber Request number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaterialRequestSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> getMaterialRequestSlipByNumber(
        const std::string& requestNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all material request slips or slips matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching MaterialRequestSlipDTOs.
     */
    virtual std::vector<ERP::Material::DTO::MaterialRequestSlipDTO> getAllMaterialRequestSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates material request slip information.
     * @param requestSlipDTO DTO containing updated request information (must have ID).
     * @param requestSlipDetails Vector of MaterialRequestSlipDetailDTOs for the slip (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateMaterialRequestSlip(
        const ERP::Material::DTO::MaterialRequestSlipDTO& requestSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO>& requestSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a material request slip.
     * @param requestSlipId ID of the request.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateMaterialRequestSlipStatus(
        const std::string& requestSlipId,
        ERP::Material::DTO::MaterialRequestSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a material request slip record by ID (soft delete).
     * @param requestSlipId ID of the request to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteMaterialRequestSlip(
        const std::string& requestSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all details for a specific material request slip.
     * @param requestSlipId ID of the material request slip.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of MaterialRequestSlipDetailDTOs.
     */
    virtual std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetails(
        const std::string& requestSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves a specific material request slip detail by ID.
     * @param detailId ID of the detail.
     * @param userRoleIds Roles of the user making the request.
     * @return An optional MaterialRequestSlipDetailDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetailById(
        const std::string& detailId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};

} // namespace Services
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_SERVICE_IMATERIALREQUESTSERVICE_H