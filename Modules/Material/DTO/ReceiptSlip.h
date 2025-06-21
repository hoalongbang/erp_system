// Modules/Material/DTO/ReceiptSlip.h
#ifndef MODULES_MATERIAL_DTO_RECEIPTSLIP_H
#define MODULES_MATERIAL_DTO_RECEIPTSLIP_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include "DataObjects/BaseDTO.h"// Updated include path
#include "Modules/Common/Common.h>// For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h" // For UUID generation
namespace ERP {
namespace Material {
namespace DTO {
/**
 * @brief Enum for Receipt Slip Status.
 */
enum class ReceiptSlipStatus {
    DRAFT = 0,
    PENDING_APPROVAL = 1,
    APPROVED = 2,
    IN_PROGRESS = 3,        // Receiving in progress
    COMPLETED = 4,          // All items received
    CANCELLED = 5,
    REJECTED = 6
};
/**
 * @brief DTO for Material Receipt Slip entity (Phiếu nhập kho).
 */
struct ReceiptSlipDTO : public BaseDTO {
    std::string receiptNumber; // Số phiếu nhập kho (tạo tự động hoặc nhập thủ công)
    std::string warehouseId; // Kho hàng nhập vật tư
    std::string receivedByUserId; // Người tạo phiếu / người nhận vật tư
    std::chrono::system_clock::time_point receiptDate; // Ngày nhập vật tư thực tế
    ReceiptSlipStatus status;
    std::optional<std::string> referenceDocumentId; // ID tài liệu tham chiếu (ví dụ: Purchase Order ID)
    std::optional<std::string> referenceDocumentType; // Loại tài liệu tham chiếu (ví dụ: "PurchaseOrder")
    std::optional<std::string> notes;

    ReceiptSlipDTO() : BaseDTO(), receiptNumber(ERP::Utils::generateUUID()), status(ReceiptSlipStatus::DRAFT) {}
    virtual ~ReceiptSlipDTO() = default;

    // Utility methods
    std::string getStatusString() const {
        switch (status) {
            case ReceiptSlipStatus::DRAFT: return "Draft";
            case ReceiptSlipStatus::PENDING_APPROVAL: return "Pending Approval";
            case ReceiptSlipStatus::APPROVED: return "Approved";
            case ReceiptSlipStatus::IN_PROGRESS: return "In Progress";
            case ReceiptSlipStatus::COMPLETED: return "Completed";
            case ReceiptSlipStatus::CANCELLED: return "Cancelled";
            case ReceiptSlipStatus::REJECTED: return "Rejected";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DTO_RECEIPTSLIP_H