// Modules/Material/DTO/MaterialRequestSlipDetail.h
#ifndef MODULES_MATERIAL_DTO_MATERIALREQUESTSLIPDETAIL_H
#define MODULES_MATERIAL_DTO_MATERIALREQUESTSLIPDETAIL_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include "DataObjects/BaseDTO.h"// Updated include path
#include "Modules/Common/Common.h"// For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h" // For UUID generation
namespace ERP {
namespace Material {
namespace DTO {
/**
 * @brief DTO for Material Request Slip Detail entity (Chi tiết phiếu yêu cầu vật tư).
 */
struct MaterialRequestSlipDetailDTO : public BaseDTO {
    std::string materialRequestSlipId; // ID của phiếu yêu cầu vật tư
    std::string productId;             // ID vật tư/sản phẩm
    double requestedQuantity;          // Số lượng yêu cầu
    double issuedQuantity;             // Số lượng đã xuất (tích lũy từ Issue Slips)
    bool isFullyIssued = false;        // Đã xuất đủ số lượng yêu cầu chưa
    std::optional<std::string> notes;

    MaterialRequestSlipDetailDTO() : BaseDTO(), requestedQuantity(0.0), issuedQuantity(0.0) {}
    virtual ~MaterialRequestSlipDetailDTO() = default;
};
} // namespace DTO
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DTO_MATERIALREQUESTSLIPDETAIL_H