// Modules/Material/DTO/IssueSlip.h
#ifndef MODULES_MATERIAL_DTO_ISSUESLIP_H
#define MODULES_MATERIAL_DTO_ISSUESLIP_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include "DataObjects/BaseDTO.h"// Updated include path
#include "Modules/Common/Common.h"// For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h" // For UUID generation
namespace ERP {
namespace Material {
namespace DTO {
/**
 * @brief Enum for Issue Slip Status.
 */
enum class IssueSlipStatus {
    DRAFT = 0,
    PENDING_APPROVAL = 1,
    APPROVED = 2,
    IN_PROGRESS = 3,        // Issuing in progress
    COMPLETED = 4,          // All items issued
    CANCELLED = 5,
    REJECTED = 6
};
/**
 * @brief DTO for Material Issue Slip entity (Phiếu xuất kho).
 */
struct IssueSlipDTO : public BaseDTO {
    std::string issueNumber; // Số phiếu xuất vật tư (tạo tự động hoặc nhập thủ công)
    std::string warehouseId; // Kho hàng xuất vật tư
    std::string issuedByUserId; // Người tạo phiếu / người xuất vật tư
    std::chrono::system_clock::time_point issueDate; // Ngày xuất vật tư thực tế
    std::optional<std::string> materialRequestSlipId; // MỚI: Liên kết đến phiếu yêu cầu vật tư (tùy chọn)
    IssueSlipStatus status;
    std::optional<std::string> referenceDocumentId; // ID tài liệu tham chiếu (ví dụ: Sales Order ID)
    std::optional<std::string> referenceDocumentType; // Loại tài liệu tham chiếu (ví dụ: "SalesOrder")
    std::optional<std::string> notes;

    IssueSlipDTO() : BaseDTO(), issueNumber(ERP::Utils::generateUUID()), status(IssueSlipStatus::DRAFT) {}
    virtual ~IssueSlipDTO() = default;

    // Utility methods
    std::string getStatusString() const {
        switch (status) {
            case IssueSlipStatus::DRAFT: return "Draft";
            case IssueSlipStatus::PENDING_APPROVAL: return "Pending Approval";
            case IssueSlipStatus::APPROVED: return "Approved";
            case IssueSlipStatus::IN_PROGRESS: return "In Progress";
            case IssueSlipStatus::COMPLETED: return "Completed";
            case IssueSlipStatus::CANCELLED: return "Cancelled";
            case IssueSlipStatus::REJECTED: return "Rejected";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DTO_ISSUESLIP_H