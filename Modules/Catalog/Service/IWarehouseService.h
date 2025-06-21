// Modules/Catalog/Service/IWarehouseService.h
#ifndef MODULES_CATALOG_SERVICE_IWAREHOUSESERVICE_H
#define MODULES_CATALOG_SERVICE_IWAREHOUSESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "Warehouse.h"     // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Catalog {
namespace Services {

/**
 * @brief IWarehouseService interface defines operations for managing warehouses.
 */
class IWarehouseService {
public:
    virtual ~IWarehouseService() = default;
    /**
     * @brief Creates a new warehouse.
     * @param warehouseDTO DTO containing new warehouse information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional WarehouseDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::WarehouseDTO> createWarehouse(
        const ERP::Catalog::DTO::WarehouseDTO& warehouseDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves warehouse information by ID.
     * @param warehouseId ID of the warehouse to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional WarehouseDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::WarehouseDTO> getWarehouseById(
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves warehouse information by name.
     * @param warehouseName Name of the warehouse to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional WarehouseDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::WarehouseDTO> getWarehouseByName(
        const std::string& warehouseName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all warehouses or warehouses matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching WarehouseDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::WarehouseDTO> getAllWarehouses(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates warehouse information.
     * @param warehouseDTO DTO containing updated warehouse information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateWarehouse(
        const ERP::Catalog::DTO::WarehouseDTO& warehouseDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a warehouse.
     * @param warehouseId ID of the warehouse.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateWarehouseStatus(
        const std::string& warehouseId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a warehouse record by ID (soft delete).
     * @param warehouseId ID of the warehouse to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteWarehouse(
        const std::string& warehouseId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_IWAREHOUSESERVICE_H