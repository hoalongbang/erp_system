// Modules/Sales/DTO/ShipmentDetail.h
#ifndef MODULES_SALES_DTO_SHIPMENTDETAIL_H
#define MODULES_SALES_DTO_SHIPMENTDETAIL_H
#include <string>
#include <vector>
#include <optional>
#include "DataObjects/BaseDTO.h"// Updated include path
#include "Modules/Common/Common.h>// For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h" // For UUID generation
namespace ERP {
namespace Sales { // Namespace for Sales module
namespace DTO {
/**
 * @brief DTO for Shipment Detail entity.
 */
struct ShipmentDetailDTO : public BaseDTO {
    std::string shipmentId;
    std::string salesOrderDetailId;
    std::string productId;
    std::string warehouseId;
    std::string locationId;
    double quantity;
    std::optional<std::string> lotNumber;
    std::optional<std::string> serialNumber;
    std::optional<std::string> notes;

    ShipmentDetailDTO() : BaseDTO(), quantity(0.0) {}
    virtual ~ShipmentDetailDTO() = default;
};
} // namespace DTO
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_SHIPMENTDETAIL_H