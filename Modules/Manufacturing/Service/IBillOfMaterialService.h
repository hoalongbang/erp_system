// Modules/Manufacturing/Service/IBillOfMaterialService.h
#ifndef MODULES_MANUFACTURING_SERVICE_IBILLOFMATERIALSERVICE_H
#define MODULES_MANUFACTURING_SERVICE_IBILLOFMATERIALSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "BillOfMaterial.h"     // DTO
#include "BillOfMaterialItem.h" // DTO
#include "Common.h"             // Enum Common
#include "BaseService.h"        // Base Service

namespace ERP {
namespace Manufacturing {
namespace Services {

/**
 * @brief IBillOfMaterialService interface defines operations for managing Bills of Material (BOMs).
 */
class IBillOfMaterialService {
public:
    virtual ~IBillOfMaterialService() = default;
    /**
     * @brief Creates a new Bill of Material (BOM).
     * @param bomDTO DTO containing new BOM information.
     * @param bomItems Vector of BillOfMaterialItemDTOs for the BOM.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional BillOfMaterialDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> createBillOfMaterial(
        const ERP::Manufacturing::DTO::BillOfMaterialDTO& bomDTO,
        const std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO>& bomItems,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves BOM information by ID.
     * @param bomId ID of the BOM to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional BillOfMaterialDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> getBillOfMaterialById(
        const std::string& bomId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves BOM information by BOM name or product ID.
     * @param bomNameOrProductId Name of the BOM or product ID to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional BillOfMaterialDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> getBillOfMaterialByNameOrProductId(
        const std::string& bomNameOrProductId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all BOMs or BOMs matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching BillOfMaterialDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> getAllBillOfMaterials(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates BOM information.
     * @param bomDTO DTO containing updated BOM information (must have ID).
     * @param bomItems Vector of BillOfMaterialItemDTOs for the BOM (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateBillOfMaterial(
        const ERP::Manufacturing::DTO::BillOfMaterialDTO& bomDTO,
        const std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO>& bomItems,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a BOM.
     * @param bomId ID of the BOM.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateBillOfMaterialStatus(
        const std::string& bomId,
        ERP::Manufacturing::DTO::BillOfMaterialStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a BOM record by ID (soft delete).
     * @param bomId ID of the BOM to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteBillOfMaterial(
        const std::string& bomId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves items for a specific Bill of Material.
     * @param bomId ID of the BOM.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of BillOfMaterialItemDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> getBillOfMaterialItems(
        const std::string& bomId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};

} // namespace Services
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_SERVICE_IBILLOFMATERIALSERVICE_H