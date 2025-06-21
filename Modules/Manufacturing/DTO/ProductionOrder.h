// Modules/Manufacturing/DTO/ProductionOrder.h
#ifndef MODULES_MANUFACTURING_DTO_PRODUCTIONORDER_H
#define MODULES_MANUFACTURING_DTO_PRODUCTIONORDER_H
#include <string>
#include <optional>
#include <chrono>
#include <vector> // For BillOfMaterialItemDTO
#include <map>    // For std::map<std::string, std::any> replacement of QJsonObject
#include <any>    // For std::any replacement of QJsonObject
// #include <QJsonObject> // Removed
// #include <QString>     // Removed
#include "DataObjects/BaseDTO.h"
#include "Modules/Common/Common.h" // For EntityStatus
#include "Modules/Utils/Utils.h"   // For generateUUID
// Include nested DTOs (if any) e.g., ProductionOrderDetailDTO if separated
#include "Modules/Manufacturing/DTO/BillOfMaterialItem.h" // Might be needed for components

namespace ERP {
namespace Manufacturing {
namespace DTO {
/**
 * @brief Enum định nghĩa trạng thái của lệnh sản xuất.
 */
enum class ProductionOrderStatus {
    DRAFT = 0,         /**< Bản nháp */
    PLANNED = 1,       /**< Đã lập kế hoạch */
    RELEASED = 2,      /**< Đã phát hành (sẵn sàng sản xuất) */
    IN_PROGRESS = 3,   /**< Đang thực hiện */
    COMPLETED = 4,     /**< Đã hoàn thành */
    CANCELLED = 5,     /**< Đã hủy */
    ON_HOLD = 6,       /**< Tạm dừng */
    REJECTED = 7       /**< Đã từ chối */
};
/**
 * @brief DTO for Production Order entity.
 * Represents a command to produce a specific quantity of a product.
 */
struct ProductionOrderDTO : public BaseDTO {
    std::string orderNumber;      /**< Số lệnh sản xuất (duy nhất) */
    std::string productId;        /**< ID của sản phẩm cần sản xuất */
    double plannedQuantity;       /**< Số lượng sản xuất dự kiến */
    std::string unitOfMeasureId;  /**< ID đơn vị đo của sản phẩm */
    ProductionOrderStatus status; /**< Trạng thái hiện tại của lệnh */
    std::optional<std::string> bomId; /**< ID của định mức nguyên vật liệu (BOM) được sử dụng */
    std::optional<std::string> productionLineId; /**< ID dây chuyền sản xuất được chỉ định */
    std::chrono::system_clock::time_point plannedStartDate; /**< Ngày bắt đầu sản xuất dự kiến */
    std::chrono::system_clock::time_point plannedEndDate;   /**< Ngày hoàn thành sản xuất dự kiến */
    std::optional<std::chrono::system_clock::time_point> actualStartDate; /**< Ngày bắt đầu sản xuất thực tế */
    std::optional<std::chrono::system_clock::time_point> actualEndDate;   /**< Ngày hoàn thành sản xuất thực tế */
    double actualQuantityProduced; /**< Số lượng thực tế đã sản xuất */
    std::optional<std::string> notes; /**< Ghi chú thêm */

    std::map<std::string, std::any> metadata; /**< Dữ liệu bổ sung dạng map */
    
    ProductionOrderDTO() : BaseDTO(), orderNumber(ERP::Utils::generateUUID()), productId(""), plannedQuantity(0.0), unitOfMeasureId(""),
                           status(ProductionOrderStatus::DRAFT), actualQuantityProduced(0.0) {}
    virtual ~ProductionOrderDTO() = default;

    // Utility methods (can be moved to StringUtils or helper for DTOs)
    std::string getStatusString() const {
        switch (status) {
            case ProductionOrderStatus::DRAFT: return "Draft";
            case ProductionOrderStatus::PLANNED: return "Planned";
            case ProductionOrderStatus::RELEASED: return "Released";
            case ProductionOrderStatus::IN_PROGRESS: return "In Progress";
            case ProductionOrderStatus::COMPLETED: return "Completed";
            case ProductionOrderStatus::CANCELLED: return "Cancelled";
            case ProductionOrderStatus::ON_HOLD: return "On Hold";
            case ProductionOrderStatus::REJECTED: return "Rejected";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_DTO_PRODUCTIONORDER_H