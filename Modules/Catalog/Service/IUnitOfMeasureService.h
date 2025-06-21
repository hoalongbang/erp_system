// Modules/Catalog/Service/IUnitOfMeasureService.h
#ifndef MODULES_CATALOG_SERVICE_UNITOFMEASURESERVICE_H
#define MODULES_CATALOG_SERVICE_UNITOFMEASURESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "UnitOfMeasure.h" // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Catalog {
namespace Services {

/**
 * @brief IUnitOfMeasureService interface defines operations for managing units of measure.
 */
class IUnitOfMeasureService {
public:
    virtual ~IUnitOfMeasureService() = default;
    /**
     * @brief Creates a new unit of measure.
     * @param uomDTO DTO containing new unit of measure information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional UnitOfMeasureDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> createUnitOfMeasure(
        const ERP::Catalog::DTO::UnitOfMeasureDTO& uomDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves unit of measure information by ID.
     * @param uomId ID of the unit of measure to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional UnitOfMeasureDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> getUnitOfMeasureById(
        const std::string& uomId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves unit of measure information by name or symbol.
     * @param nameOrSymbol Name or symbol of the unit of measure to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional UnitOfMeasureDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> getUnitOfMeasureByNameOrSymbol(
        const std::string& nameOrSymbol,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all units of measure or UoMs matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching UnitOfMeasureDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> getAllUnitsOfMeasure(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates unit of measure information.
     * @param uomDTO DTO containing updated UoM information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateUnitOfMeasure(
        const ERP::Catalog::DTO::UnitOfMeasureDTO& uomDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a unit of measure.
     * @param uomId ID of the unit of measure.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateUnitOfMeasureStatus(
        const std::string& uomId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a unit of measure record by ID (soft delete).
     * @param uomId ID of the unit of measure to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteUnitOfMeasure(
        const std::string& uomId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_UNITOFMEASURESERVICE_H