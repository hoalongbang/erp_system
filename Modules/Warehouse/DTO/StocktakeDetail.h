// Modules/Warehouse/DTO/StocktakeDetail.h
#ifndef MODULES_WAREHOUSE_DTO_STOCKTAKEDETAIL_H
#define MODULES_WAREHOUSE_DTO_STOCKTAKEDETAIL_H
#include <string>
#include <optional>
#include "BaseDTO.h"   // Đã rút gọn include
#include "Common.h"    // Đã rút gọn include
#include "Utils.h"     // Đã rút gọn include
namespace ERP {
    namespace Warehouse {
        namespace DTO {
            /**
             * @brief DTO for Stocktake Detail entity.
             * Represents a single item counted during a stocktake.
             */
            struct StocktakeDetailDTO : public BaseDTO {
                std::string stocktakeRequestId; // Foreign key to StocktakeRequestDTO
                std::string productId;          // Foreign key to ProductDTO
                std::string warehouseId;        // Foreign key to WarehouseDTO
                std::string locationId;         // Foreign key to LocationDTO (where item is located)
                std::optional<std::string> lotNumber;   // Lot number (if applicable)
                std::optional<std::string> serialNumber;// Serial number (if applicable)
                double systemQuantity;          // Quantity recorded in the system
                double countedQuantity;         // Actual quantity counted
                double difference;              // Difference (counted - system)
                std::optional<std::string> adjustmentTransactionId; // Link to InventoryTransaction if adjustment made
                std::optional<std::string> notes;

                StocktakeDetailDTO() : systemQuantity(0.0), countedQuantity(0.0), difference(0.0) {} // Default constructor
                virtual ~StocktakeDetailDTO() = default;
            };
        } // namespace DTO
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DTO_STOCKTAKEDETAIL_H