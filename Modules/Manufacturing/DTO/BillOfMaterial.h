// Modules/Manufacturing/DTO/BillOfMaterial.h
#ifndef MODULES_MANUFACTURING_DTO_BILLOFMATERIAL_H
#define MODULES_MANUFACTURING_DTO_BILLOFMATERIAL_H
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
// Include BillOfMaterialItemDTO - Assuming it's a common DTO or in this folder
#include "Modules/Manufacturing/DTO/BillOfMaterialItem.h" // Assuming this DTO is defined nearby

namespace ERP {
namespace Manufacturing {
namespace DTO {
/**
 * @brief Enum định nghĩa trạng thái của định mức nguyên vật liệu (BOM).
 */
enum class BillOfMaterialStatus {
    DRAFT = 0,      /**< Bản nháp */
    ACTIVE = 1,     /**< Đang hoạt động */
    INACTIVE = 2,   /**< Không hoạt động */
    ARCHIVED = 3    /**< Đã lưu trữ */
};
/**
 * @brief DTO for Bill of Material (BOM) entity.
 * Defines the components and quantities required to produce a finished good or assembly.
 */
struct BillOfMaterialDTO : public BaseDTO {
    std::string bomName;        /**< Tên định mức */
    std::string productId;      /**< ID của sản phẩm được sản xuất (thành phẩm hoặc bán thành phẩm) */
    std::optional<std::string> description; /**< Mô tả định mức */
    std::string baseQuantityUnitId; /**< ID đơn vị của số lượng cơ sở (ví dụ: 1 "Cái" của thành phẩm) */
    double baseQuantity = 1.0;  /**< Số lượng cơ sở của thành phẩm mà định mức này áp dụng (ví dụ: 1 đơn vị) */
    BillOfMaterialStatus status;/**< Trạng thái của BOM */
    std::optional<int> version; /**< Phiên bản của BOM */

    std::vector<BillOfMaterialItemDTO> items; /**< Danh sách các thành phần (nguyên vật liệu, bán thành phẩm) */

    std::map<std::string, std::any> metadata; /**< Dữ liệu bổ sung dạng map (ví dụ: ghi chú, thông số kỹ thuật) */

    BillOfMaterialDTO() : BaseDTO(), bomName(""), productId(""), baseQuantityUnitId(""), status(BillOfMaterialStatus::DRAFT) {}
    virtual ~BillOfMaterialDTO() = default;

    // Utility methods (can be moved to StringUtils or helper for DTOs)
    std::string getStatusString() const {
        switch (status) {
            case BillOfMaterialStatus::DRAFT: return "Draft";
            case BillOfMaterialStatus::ACTIVE: return "Active";
            case BillOfMaterialStatus::INACTIVE: return "Inactive";
            case BillOfMaterialStatus::ARCHIVED: return "Archived";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_DTO_BILLOFMATERIAL_H