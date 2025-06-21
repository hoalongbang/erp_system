// Modules/Warehouse/Service/IInventoryManagementService.h
#ifndef MODULES_WAREHOUSE_SERVICE_IINVENTORYMANAGEMENTSERVICE_H
#define MODULES_WAREHOUSE_SERVICE_IINVENTORYMANAGEMENTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "BaseService.h"        // Base Service
#include "Inventory.h"          // Inventory DTO
#include "InventoryTransaction.h" // InventoryTransaction DTO
#include "InventoryCostLayer.h" // InventoryCostLayer DTO

namespace ERP {
namespace Warehouse {
namespace Services {

/**
 * @brief IInventoryManagementService interface defines operations for managing inventory levels and movements.
 */
class IInventoryManagementService {
public:
    virtual ~IInventoryManagementService() = default;
    /**
     * @brief Creates a new inventory record for a product at a specific location.
     * @param inventoryDTO DTO containing initial inventory information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional InventoryDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Warehouse::DTO::InventoryDTO> createInventory(
        const ERP::Warehouse::DTO::InventoryDTO& inventoryDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves inventory information by ID.
     * @param inventoryId ID of the inventory record to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional InventoryDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Warehouse::DTO::InventoryDTO> getInventoryById(
        const std::string& inventoryId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves inventory information for a specific product at a given warehouse and location.
     * @param productId ID of the product.
     * @param warehouseId ID of the warehouse.
     * @param locationId ID of the location.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional InventoryDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Warehouse::DTO::InventoryDTO> getInventoryByProductLocation(
        const std::string& productId,
        const std::string& warehouseId,
        const std::string& locationId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all inventory records or records matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching InventoryDTOs.
     */
    virtual std::vector<ERP::Warehouse::DTO::InventoryDTO> getAllInventory(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all inventory records for a specific product across all warehouses/locations.
     * @param productId ID of the product.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching InventoryDTOs.
     */
    virtual std::vector<ERP::Warehouse::DTO::InventoryDTO> getInventoryByProduct(
        const std::string& productId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates inventory information (e.g., reorder levels).
     * @param inventoryDTO DTO containing updated inventory information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateInventory(
        const ERP::Warehouse::DTO::InventoryDTO& inventoryDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records a goods receipt, increasing inventory quantity.
     * This method also creates an InventoryTransaction.
     * @param transactionDTO DTO containing details of the goods receipt.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if receipt is successful, false otherwise.
     */
    virtual bool recordGoodsReceipt(
        const ERP::Warehouse::DTO::InventoryTransactionDTO& transactionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records a goods issue, decreasing inventory quantity.
     * This method also creates an InventoryTransaction.
     * @param transactionDTO DTO containing details of the goods issue.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if issue is successful, false otherwise.
     */
    virtual bool recordGoodsIssue(
        const ERP::Warehouse::DTO::InventoryTransactionDTO& transactionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Adjusts inventory quantity (in or out) for discrepancies.
     * This method also creates an InventoryTransaction.
     * @param transactionDTO DTO containing details of the adjustment.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if adjustment is successful, false otherwise.
     */
    virtual bool adjustInventory(
        const ERP::Warehouse::DTO::InventoryTransactionDTO& transactionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Reserves a specified quantity of a product in inventory.
     * Decreases available quantity and increases reserved quantity.
     * @param productId ID of the product.
     * @param warehouseId ID of the warehouse.
     * @param locationId ID of the location.
     * @param quantityToReserve Quantity to reserve.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if reservation is successful, false otherwise.
     */
    virtual bool reserveInventory(
        const std::string& productId,
        const std::string& warehouseId,
        const std::string& locationId,
        double quantityToReserve,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Unreserves a specified quantity of a product in inventory.
     * Increases available quantity and decreases reserved quantity.
     * @param productId ID of the product.
     * @param warehouseId ID of the warehouse.
     * @param locationId ID of the location.
     * @param quantityToUnreserve Quantity to unreserve.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if unreservation is successful, false otherwise.
     */
    virtual bool unreserveInventory(
        const std::string& productId,
        const std::string& warehouseId,
        const std::string& locationId,
        double quantityToUnreserve,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Transfers stock from one location to another within or between warehouses.
     * Creates two inventory transactions (issue from source, receipt to destination).
     * @param productId ID of the product.
     * @param sourceWarehouseId Source warehouse ID.
     * @param sourceLocationId Source location ID.
     * @param destinationWarehouseId Destination warehouse ID.
     * @param destinationLocationId Destination location ID.
     * @param quantity Quantity to transfer.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if transfer is successful, false otherwise.
     */
    virtual bool transferStock(
        const std::string& productId,
        const std::string& sourceWarehouseId,
        const std::string& sourceLocationId,
        const std::string& destinationWarehouseId,
        const std::string& destinationLocationId,
        double quantity,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes an inventory record by ID (soft delete).
     * @param inventoryId ID of the inventory record to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteInventory(
        const std::string& inventoryId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Gets the default location ID for a given warehouse.
     * @param warehouseId The ID of the warehouse.
     * @param userRoleIds The roles of the current user.
     * @return An optional string containing the default location ID, or std::nullopt if not found.
     */
    virtual std::optional<std::string> getDefaultLocationForWarehouse(
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds) = 0;

    // NEW: Inventory Cost Layer Management (if implemented)
    /**
     * @brief Records a new cost layer for inventory (e.g., for FIFO/LIFO costing).
     * This is typically called internally by recordGoodsReceipt.
     * @param costLayerDTO DTO containing cost layer details.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user.
     * @return true if successful, false otherwise.
     */
    virtual bool recordInventoryCostLayer(
        const ERP::Warehouse::DTO::InventoryCostLayerDTO& costLayerDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Consumes quantity from inventory cost layers (e.g., when goods are issued).
     * This is typically called internally by recordGoodsIssue.
     * @param productId ID of the product.
     * @param warehouseId ID of the warehouse.
     * @param locationId ID of the location.
     * @param quantityToConsume Quantity to consume from layers.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user.
     * @return true if successful, false otherwise.
     */
    virtual bool consumeInventoryCostLayers(
        const std::string& productId,
        const std::string& warehouseId,
        const std::string& locationId,
        double quantityToConsume,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_SERVICE_IINVENTORYMANAGEMENTSERVICE_H