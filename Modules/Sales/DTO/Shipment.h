// Modules/Sales/DTO/Shipment.h
#ifndef MODULES_SALES_DTO_SHIPMENT_H
#define MODULES_SALES_DTO_SHIPMENT_H
#include <string>
#include <vector>
#include <optional>
#include <chrono> // For std::chrono::system_clock::time_point
#include "DataObjects/BaseDTO.h"// Updated include path
#include "Modules/Common/Common.h>// For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h" // For UUID generation
namespace ERP {
namespace Sales { // Namespace for Sales module
namespace DTO {
/**
 * @brief Enum for Shipment Type.
 */
enum class ShipmentType {
    SALES_DELIVERY = 0,
    SAMPLE_DELIVERY = 1,
    RETURN_SHIPMENT = 2,
    OTHER = 3
};
/**
 * @brief Enum for Shipment Status.
 */
enum class ShipmentStatus {
    PENDING = 0,
    PACKED = 1,
    SHIPPED = 2,
    DELIVERED = 3,
    CANCELLED = 4,
    RETURNED = 5
};
/**
 * @brief DTO for Shipment entity.
 */
struct ShipmentDTO : public BaseDTO {
    std::string shipmentNumber;
    std::string salesOrderId;
    std::string customerId;
    std::string shippedByUserId;
    std::chrono::system_clock::time_point shipmentDate;
    std::optional<std::chrono::system_clock::time_point> deliveryDate;
    ShipmentType type;
    ShipmentStatus status;
    std::optional<std::string> carrierName;
    std::optional<std::string> trackingNumber;
    std::optional<std::string> deliveryAddress;
    std::optional<std::string> notes;

    ShipmentDTO() : BaseDTO(), shipmentNumber(ERP::Utils::generateUUID()),
                    type(ShipmentType::SALES_DELIVERY), status(ShipmentStatus::PENDING) {}
    virtual ~ShipmentDTO() = default;

    // Utility methods
    std::string getTypeString() const {
        switch (type) {
            case ShipmentType::SALES_DELIVERY: return "Sales Delivery";
            case ShipmentType::SAMPLE_DELIVERY: return "Sample Delivery";
            case ShipmentType::RETURN_SHIPMENT: return "Return Shipment";
            case ShipmentType::OTHER: return "Other";
            default: return "Unknown";
        }
    }
    std::string getStatusString() const {
        switch (status) {
            case ShipmentStatus::PENDING: return "Pending";
            case ShipmentStatus::PACKED: return "Packed";
            case ShipmentStatus::SHIPPED: return "Shipped";
            case ShipmentStatus::DELIVERED: return "Delivered";
            case ShipmentStatus::CANCELLED: return "Cancelled";
            case ShipmentStatus::RETURNED: return "Returned";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_SHIPMENT_H