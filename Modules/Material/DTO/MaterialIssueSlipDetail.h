// Modules/Material/DTO/MaterialIssueSlipDetail.h
#ifndef MODULES_MATERIAL_DTO_MATERIALISSUESLIPDETAIL_H
#define MODULES_MATERIAL_DTO_MATERIALISSUESLIPDETAIL_H
#include <string>
#include <optional>
#include <chrono>
#include "DataObjects/BaseDTO.h"   // Updated include path
#include "Modules/Common/Common.h"    // For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h"      // For UUID generation
namespace ERP {
namespace Material {
namespace DTO {
/**
 * @brief DTO for Material Issue Slip Detail entity (Chi tiết phiếu xuất vật tư cho sản xuất).
 */
struct MaterialIssueSlipDetailDTO : public BaseDTO {
    std::string materialIssueSlipId; // ID của phiếu xuất vật tư sản xuất
    std::string productId;           // ID vật tư/sản phẩm
    double issuedQuantity;           // Số lượng thực tế đã xuất
    std::optional<std::string> lotNumber;   // Số lô (nếu có)
    std::optional<std::string> serialNumber;// Số serial (nếu có)
    std::optional<std::string> inventoryTransactionId; // Liên kết đến giao dịch tồn kho đã tạo
    std::optional<std::string> notes;

    MaterialIssueSlipDetailDTO() : BaseDTO(), issuedQuantity(0.0) {}
    virtual ~MaterialIssueSlipDetailDTO() = default;
};
} // namespace DTO
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DTO_MATERIALISSUESLIPDETAIL_H