// Modules/Material/DTO/IssueSlipDetail.h
#ifndef MODULES_MATERIAL_DTO_ISSUESLIPDETAIL_H
#define MODULES_MATERIAL_DTO_ISSUESLIPDETAIL_H
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
 * @brief DTO for Material Issue Slip Detail entity (Chi tiết phiếu xuất kho).
 */
struct IssueSlipDetailDTO : public BaseDTO {
    std::string issueSlipId;    // ID của phiếu xuất vật tư
    std::string productId;      // ID vật tư/sản phẩm
    std::string locationId;     // ID vị trí xuất kho
    double requestedQuantity;   // Số lượng yêu cầu xuất
    double issuedQuantity;      // Số lượng thực tế đã xuất
    std::optional<std::string> lotNumber;   // Số lô (nếu có)
    std::optional<std::string> serialNumber;// Số serial (nếu có)
    std::optional<std::string> inventoryTransactionId; // Liên kết đến giao dịch tồn kho đã tạo
    std::optional<std::string> notes;
    bool isFullyIssued = false; // Đã xuất đủ số lượng yêu cầu chưa
    std::optional<std::string> materialRequestSlipDetailId; // MỚI: Liên kết đến chi tiết phiếu yêu cầu vật tư (tùy chọn)

    IssueSlipDetailDTO() : BaseDTO(), requestedQuantity(0.0), issuedQuantity(0.0) {}
    virtual ~IssueSlipDetailDTO() = default;
};
} // namespace DTO
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DTO_ISSUESLIPDETAIL_H