// Modules/Warehouse/DTO/PickingDetail.h
#ifndef MODULES_WAREHOUSE_DTO_PICKINGDETAIL_H
#define MODULES_WAREHOUSE_DTO_PICKINGDETAIL_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
    namespace Warehouse {
        namespace DTO {
            /**
             * @brief DTO for Picking Detail entity.
             * Represents a single item to be picked from inventory within a picking request.
             * (Previously named PickingListItemDTO, now unified as PickingDetailDTO)
             */
            struct PickingDetailDTO : public BaseDTO {
                std::string pickingRequestId;   /**< Foreign key to PickingRequestDTO. */
                std::string productId;          /**< Foreign key to ProductDTO. */
                std::string warehouseId;        /**< Kho hàng nơi lấy hàng. */
                std::string locationId;         /**< Vị trí cụ thể trong kho để lấy hàng. */
                double requestedQuantity = 0.0; /**< Số lượng yêu cầu lấy. */
                double pickedQuantity = 0.0;    /**< Số lượng thực tế đã lấy. */
                std::string unitOfMeasureId;    /**< Đơn vị đo của số lượng. */
                std::optional<std::string> lotNumber; /**< Số lô của mặt hàng (tùy chọn). */
                std::optional<std::string> serialNumber; /**< Số serial của mặt hàng (tùy chọn). */
                bool isPicked = false;          /**< Cờ chỉ báo mặt hàng đã được lấy đủ hay chưa. */
                std::optional<std::string> notes; /**< Ghi chú cho chi tiết lấy hàng (tùy chọn). */
                std::optional<std::string> salesOrderDetailId; /**< ID chi tiết đơn hàng bán gốc (nếu có) (tùy chọn). */
                std::optional<std::string> inventoryTransactionId; /**< ID giao dịch tồn kho liên quan (tùy chọn). */

                // Default constructor
                PickingDetailDTO() : BaseDTO(), requestedQuantity(0.0), pickedQuantity(0.0) {}

                // Virtual destructor for proper polymorphic cleanup
                virtual ~PickingDetailDTO() = default;
            };
        } // namespace DTO
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DTO_PICKINGDETAIL_H