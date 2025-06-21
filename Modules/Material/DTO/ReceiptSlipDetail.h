// Modules/Material/DTO/ReceiptSlipDetail.h
#ifndef MODULES_MATERIAL_DTO_RECEIPTSLIPDETAIL_H
#define MODULES_MATERIAL_DTO_RECEIPTSLIPDETAIL_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include "DataObjects/BaseDTO.h"   // Updated include path
#include "Modules/Common/Common.h>    // For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h"      // For UUID generation
namespace ERP {
namespace Material {
namespace DTO {
/**
 * @brief DTO for Material Receipt Slip Detail entity (Chi tiết phiếu nhập kho).
 */
struct ReceiptSlipDetailDTO : public BaseDTO {
    std::string receiptSlipId;    // ID của phiếu nhập kho
    std::string productId;        // ID vật tư/sản phẩm
    std::string locationId;       // ID vị trí nhập kho
    double expectedQuantity;      // Số lượng dự kiến nhận
    double receivedQuantity;      // Số lượng thực tế đã nhận
    std::optional<std::string> lotNumber;   // Số lô (nếu có)
    std::optional<std::string> serialNumber;// Số serial (nếu có)
    std::optional<std::chrono::system_clock::time_point> manufactureDate; // Ngày sản xuất (nếu có)
    std::optional<std::chrono::system_clock::time_point> expirationDate;  // Ngày hết hạn (nếu có)
    double unitCost;              // Giá vốn đơn vị tại thời điểm nhập
    std::optional<std::string> inventoryTransactionId; // Liên kết đến giao dịch tồn kho đã tạo
    std::optional<std::string> notes;
    bool isFullyReceived = false; // Đã nhận đủ số lượng dự kiến chưa

    ReceiptSlipDetailDTO() : BaseDTO(), expectedQuantity(0.0), receivedQuantity(0.0), unitCost(0.0) {}
    virtual ~ReceiptSlipDetailDTO() = default;
};
} // namespace DTO
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DTO_RECEIPTSLIPDETAIL_H