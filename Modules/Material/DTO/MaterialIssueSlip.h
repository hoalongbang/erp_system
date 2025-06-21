// Modules/Material/DTO/MaterialIssueSlip.h
#ifndef MODULES_MATERIAL_DTO_MATERIALISSUESLIP_H
#define MODULES_MATERIAL_DTO_MATERIALISSUESLIP_H
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
 * @brief Enum for Material Issue Slip Status for manufacturing context.
 */
enum class MaterialIssueSlipStatus {
    DRAFT = 0,
    PENDING_APPROVAL = 1,
    APPROVED = 2,
    ISSUED = 3,         // Materials have been issued
    COMPLETED = 4,      // All associated processes are done
    CANCELLED = 5,
    REJECTED = 6
};
/**
 * @brief DTO for Material Issue Slip entity in manufacturing context (Phiếu xuất vật tư cho sản xuất).
 * This is distinct from a general IssueSlip for sales/other purposes.
 */
struct MaterialIssueSlipDTO : public BaseDTO {
    std::string issueNumber; // Số phiếu xuất vật tư (tạo tự động hoặc nhập thủ công)
    std::string productionOrderId; // Liên kết đến lệnh sản xuất (Work Order)
    std::string warehouseId; // Kho hàng xuất vật tư
    std::string issuedByUserId; // Người tạo phiếu / người xuất vật tư
    std::chrono::system_clock::time_point issueDate; // Ngày xuất vật tư thực tế
    MaterialIssueSlipStatus status;
    std::optional<std::string> notes;

    MaterialIssueSlipDTO() : BaseDTO(), issueNumber(ERP::Utils::generateUUID()), status(MaterialIssueSlipStatus::DRAFT) {}
    virtual ~MaterialIssueSlipDTO() = default;

    // Utility methods
    std::string getStatusString() const {
        switch (status) {
            case MaterialIssueSlipStatus::DRAFT: return "Draft";
            case MaterialIssueSlipStatus::PENDING_APPROVAL: return "Pending Approval";
            case MaterialIssueSlipStatus::APPROVED: return "Approved";
            case MaterialIssueSlipStatus::ISSUED: return "Issued";
            case MaterialIssueSlipStatus::COMPLETED: return "Completed";
            case MaterialIssueSlipStatus::CANCELLED: return "Cancelled";
            case MaterialIssueSlipStatus::REJECTED: return "Rejected";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DTO_MATERIALISSUESLIP_H