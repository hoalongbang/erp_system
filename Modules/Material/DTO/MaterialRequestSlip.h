// Modules/Material/DTO/MaterialRequestSlip.h
#ifndef MODULES_MATERIAL_DTO_MATERIALREQUESTSLIP_H
#define MODULES_MATERIAL_DTO_MATERIALREQUESTSLIP_H
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
 * @brief Enum for Material Request Slip Status.
 */
enum class MaterialRequestSlipStatus {
    DRAFT = 0,
    PENDING_APPROVAL = 1,
    APPROVED = 2,
    IN_PROGRESS = 3,        // Materials being issued
    COMPLETED = 4,          // All items issued
    CANCELLED = 5,
    REJECTED = 6
};
/**
 * @brief DTO for Material Request Slip entity (Phiếu yêu cầu vật tư).
 */
struct MaterialRequestSlipDTO : public BaseDTO {
    std::string requestNumber; // Số phiếu yêu cầu vật tư (tạo tự động hoặc nhập thủ công)
    std::string requestingDepartment; // Bộ phận yêu cầu
    std::string requestedByUserId; // Người tạo phiếu
    std::chrono::system_clock::time_point requestDate; // Ngày yêu cầu
    MaterialRequestSlipStatus status;
    std::optional<std::string> approvedByUserId; // Người phê duyệt
    std::optional<std::chrono::system_clock::time_point> approvalDate; // Ngày phê duyệt
    std::optional<std::string> notes;
    std::optional<std::string> referenceDocumentId; // ID tài liệu tham chiếu (ví dụ: Production Order ID)
    std::optional<std::string> referenceDocumentType; // Loại tài liệu tham chiếu (ví dụ: "ProductionOrder")

    MaterialRequestSlipDTO() : BaseDTO(), requestNumber(ERP::Utils::generateUUID()), status(MaterialRequestSlipStatus::DRAFT) {}
    virtual ~MaterialRequestSlipDTO() = default;

    // Utility methods
    std::string getStatusString() const {
        switch (status) {
            case MaterialRequestSlipStatus::DRAFT: return "Draft";
            case MaterialRequestSlipStatus::PENDING_APPROVAL: return "Pending Approval";
            case MaterialRequestSlipStatus::APPROVED: return "Approved";
            case MaterialRequestSlipStatus::IN_PROGRESS: return "In Progress";
            case MaterialRequestSlipStatus::COMPLETED: return "Completed";
            case MaterialRequestSlipStatus::CANCELLED: return "Cancelled";
            case MaterialRequestSlipStatus::REJECTED: return "Rejected";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DTO_MATERIALREQUESTSLIP_H