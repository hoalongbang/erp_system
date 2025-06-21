// Modules/Warehouse/DTO/InventoryTransaction.h
#ifndef MODULES_WAREHOUSE_DTO_INVENTORYTRANSACTION_H
#define MODULES_WAREHOUSE_DTO_INVENTORYTRANSACTION_H
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
             * @brief Enum for Inventory Transaction Type.
             */
            enum class InventoryTransactionType {
                GOODS_RECEIPT = 0,      /**< Nhập kho (từ mua hàng, sản xuất) */
                GOODS_ISSUE = 1,        /**< Xuất kho (cho bán hàng, sản xuất) */
                ADJUSTMENT_IN = 2,      /**< Điều chỉnh tăng tồn kho (kiểm kê, phát hiện thừa) */
                ADJUSTMENT_OUT = 3,     /**< Điều chỉnh giảm tồn kho (kiểm kê, mất mát) */
                TRANSFER_IN = 4,        /**< Chuyển kho vào */
                TRANSFER_OUT = 5,       /**< Chuyển kho ra */
                RESERVATION = 6,        /**< Giữ hàng (cho đơn hàng) */
                RESERVATION_RELEASE = 7 /**< Giải phóng hàng đã giữ */
            };
            /**
             * @brief DTO for Inventory Transaction entity.
             * Represents a record of any movement or change in inventory quantity.
             */
            struct InventoryTransactionDTO : public BaseDTO {
                std::string productId;      // Foreign key to ProductDTO
                std::string warehouseId;    // Foreign key to WarehouseDTO
                std::string locationId;     // Foreign key to LocationDTO
                InventoryTransactionType type; // Type of transaction
                double quantity;            // Quantity involved in the transaction
                double unitCost;            // Unit cost at the time of transaction
                std::chrono::system_clock::time_point transactionDate; // Date of transaction
                std::optional<std::string> lotNumber;   // Lot number (if applicable)
                std::optional<std::string> serialNumber;// Serial number (if applicable)
                std::optional<std::chrono::system_clock::time_point> manufactureDate; // Manufacture date (if applicable)
                std::optional<std::chrono::system_clock::time_point> expirationDate;  // Expiration date (if applicable)
                std::optional<std::string> referenceDocumentId;   // ID of related document (e.g., Sales Order ID, Receipt Slip ID)
                std::optional<std::string> referenceDocumentType; // Type of related document
                std::optional<std::string> notes;

                InventoryTransactionDTO() : quantity(0.0), unitCost(0.0) {} // Default constructor
                virtual ~InventoryTransactionDTO() = default;

                // Utility methods
                std::string getTypeString() const {
                    switch (type) {
                        case InventoryTransactionType::GOODS_RECEIPT: return "Goods Receipt";
                        case InventoryTransactionType::GOODS_ISSUE: return "Goods Issue";
                        case InventoryTransactionType::ADJUSTMENT_IN: return "Adjustment In";
                        case InventoryTransactionType::ADJUSTMENT_OUT: return "Adjustment Out";
                        case InventoryTransactionType::TRANSFER_IN: return "Transfer In";
                        case InventoryTransactionType::TRANSFER_OUT: return "Transfer Out";
                        case InventoryTransactionType::RESERVATION: return "Reservation";
                        case InventoryTransactionType::RESERVATION_RELEASE: return "Reservation Release";
                        default: return "Unknown";
                    }
                }
            };
        } // namespace DTO
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DTO_INVENTORYTRANSACTION_H