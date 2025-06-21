// Modules/Warehouse/DTO/Inventory.h
#ifndef MODULES_WAREHOUSE_DTO_INVENTORY_H
#define MODULES_WAREHOUSE_DTO_INVENTORY_H
#include <string>
#include <optional>
#include <chrono>
#include "BaseDTO.h"   // Đã rút gọn include
#include "Common.h"    // Đã rút gọn include
#include "Utils.h"     // Đã rút gọn include
namespace ERP {
    namespace Warehouse {
        namespace DTO {
            /**
             * @brief DTO for Inventory entity.
             * Represents the current stock level of a product at a specific location.
             */
            struct InventoryDTO : public BaseDTO {
                std::string productId;      // Foreign key to ProductDTO
                std::string warehouseId;    // Foreign key to WarehouseDTO
                std::string locationId;     // Foreign key to LocationDTO
                double quantity = 0.0;      // Current quantity in base unit of measure
                std::optional<double> reservedQuantity; // Quantity reserved for sales orders/picking lists
                std::optional<double> availableQuantity; // Calculated: quantity - reservedQuantity
                std::optional<double> unitCost;         // Average cost or last purchase cost
                std::optional<std::string> lotNumber;   // Lot number (if applicable)
                std::optional<std::string> serialNumber;// Serial number (if applicable)
                std::optional<std::chrono::system_clock::time_point> manufactureDate; // Manufacture date (if applicable)
                std::optional<std::chrono::system_clock::time_point> expirationDate;  // Expiration date (if applicable)
                std::optional<double> reorderLevel;     // Minimum quantity to trigger reorder
                std::optional<double> reorderQuantity;  // Quantity to order when reorder level is met

                InventoryDTO() : quantity(0.0) {} // Default constructor
                virtual ~InventoryDTO() = default;
            };
        } // namespace DTO
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DTO_INVENTORY_H