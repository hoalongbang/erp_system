// Modules/Warehouse/DTO/InventoryCostLayer.h
#ifndef MODULES_WAREHOUSE_DTO_INVENTORYCOSTLAYER_H
#define MODULES_WAREHOUSE_DTO_INVENTORYCOSTLAYER_H
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
             * @brief DTO for Inventory Cost Layer entity.
             * Represents a cost layer for inventory (e.g., for FIFO/LIFO costing methods).
             * Each layer tracks a specific quantity of a product received at a certain cost.
             */
            struct InventoryCostLayerDTO : public BaseDTO {
                std::string productId;      // Foreign key to ProductDTO
                std::string warehouseId;    // Foreign key to WarehouseDTO
                std::string locationId;     // Foreign key to LocationDTO
                std::optional<std::string> lotNumber;   // Lot number (if applicable)
                std::optional<std::string> serialNumber;// Serial number (if applicable)
                double quantity;            // Quantity in this cost layer
                double unitCost;            // Unit cost for this layer
                std::chrono::system_clock::time_point receiptDate; // Date when this layer was received
                std::optional<std::string> referenceTransactionId; // Link to the InventoryTransaction (e.g., Goods Receipt)
                std::optional<std::string> referenceDocumentType; // Type of document (e.g., "PurchaseOrder", "ProductionOrder")
                std::optional<std::string> referenceDocumentNumber; // Number of the document

                InventoryCostLayerDTO() : quantity(0.0), unitCost(0.0) {} // Default constructor
                virtual ~InventoryCostLayerDTO() = default;
            };
        } // namespace DTO
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DTO_INVENTORYCOSTLAYER_H