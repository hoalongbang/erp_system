// Modules/Manufacturing/DTO/BillOfMaterialItem.h
#ifndef MODULES_MANUFACTURING_DTO_BILLOFMATERIALITEM_H
#define MODULES_MANUFACTURING_DTO_BILLOFMATERIALITEM_H
#include <string>
#include <optional>
#include <chrono>
#include <map>    // For std::map<std::string, std::any>
#include <any>    // For std::any
#include "Modules/Utils/Utils.h" // For generateUUID

namespace ERP {
namespace Manufacturing {
namespace DTO {
/**
 * @brief DTO for Bill of Material Item entity.
 * Represents a single component within a Bill of Material.
 */
struct BillOfMaterialItemDTO {
    std::string id;             /**< Unique ID for the BOM item. */
    std::string productId;      /**< ID của thành phần (nguyên vật liệu hoặc bán thành phẩm) */
    double quantity;            /**< Số lượng thành phần cần thiết */
    std::string unitOfMeasureId; /**< ID đơn vị đo của thành phần */
    std::optional<std::string> notes; /**< Ghi chú về thành phần này */
    std::map<std::string, std::any> metadata; /**< Dữ liệu bổ sung dạng map */
    
    BillOfMaterialItemDTO() : id(ERP::Utils::generateUUID()), productId(""), quantity(0.0), unitOfMeasureId("") {}
    BillOfMaterialItemDTO(const std::string& productId, double quantity, const std::string& unitOfMeasureId)
        : id(ERP::Utils::generateUUID()), productId(productId), quantity(quantity), unitOfMeasureId(unitOfMeasureId) {}
};
} // namespace DTO
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_DTO_BILLOFMATERIALITEM_H