// Modules/Manufacturing/Service/IProductionLineService.h
#ifndef MODULES_MANUFACTURING_SERVICE_IPRODUCTIONLINESERVICE_H
#define MODULES_MANUFACTURING_SERVICE_IPRODUCTIONLINESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "ProductionLine.h" // DTO
#include "Common.h"         // Enum Common
#include "BaseService.h"    // Base Service

namespace ERP {
namespace Manufacturing {
namespace Services {

/**
 * @brief IProductionLineService interface defines operations for managing production lines.
 */
class IProductionLineService {
public:
    virtual ~IProductionLineService() = default;
    /**
     * @brief Creates a new production line.
     * @param productionLineDTO DTO containing new line information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionLineDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> createProductionLine(
        const ERP::Manufacturing::DTO::ProductionLineDTO& productionLineDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves production line information by ID.
     * @param lineId ID of the line to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionLineDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> getProductionLineById(
        const std::string& lineId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves production line information by line name.
     * @param lineName Name of the production line to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionLineDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> getProductionLineByName(
        const std::string& lineName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all production lines or lines matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ProductionLineDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::ProductionLineDTO> getAllProductionLines(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates production line information.
     * @param productionLineDTO DTO containing updated line information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateProductionLine(
        const ERP::Manufacturing::DTO::ProductionLineDTO& productionLineDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a production line.
     * @param lineId ID of the line.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateProductionLineStatus(
        const std::string& lineId,
        ERP::Manufacturing::DTO::ProductionLineStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a production line record by ID (soft delete).
     * @param lineId ID of the line to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteProductionLine(
        const std::string& lineId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_SERVICE_IPRODUCTIONLINESERVICE_H