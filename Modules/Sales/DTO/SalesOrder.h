// Modules/Sales/DTO/SalesOrder.h
#ifndef MODULES_SALES_DTO_SALESORDER_H
#define MODULES_SALES_DTO_SALESORDER_H
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
 * @brief Enum for Sales Order Status.
 */
enum class SalesOrderStatus {
    DRAFT = 0,
    PENDING_APPROVAL = 1,
    APPROVED = 2,
    IN_PROGRESS = 3,        // Order being fulfilled (picking, packing, shipping)
    COMPLETED = 4,          // All items shipped and invoiced
    CANCELLED = 5,
    REJECTED = 6,
    PARTIALLY_DELIVERED = 7 // Some items delivered, but not all
};
/**
 * @brief DTO for Sales Order entity.
 */
struct SalesOrderDTO : public BaseDTO {
    std::string orderNumber;
    std::string customerId;
    std::string requestedByUserId;
    std::optional<std::string> approvedByUserId;
    std::chrono::system_clock::time_point orderDate;
    std::optional<std::chrono::system_clock::time_point> requiredDeliveryDate;
    SalesOrderStatus status;
    double totalAmount;
    double totalDiscount;
    double totalTax;
    double netAmount;
    double amountPaid;
    double amountDue;
    std::string currency;
    std::optional<std::string> paymentTerms;
    std::optional<std::string> deliveryAddress;
    std::optional<std::string> notes;
    std::string warehouseId; // Default warehouse for the order
    std::optional<std::string> quotationId; // Link to QuotationDTO if order created from quotation

    SalesOrderDTO() : BaseDTO(), orderNumber(ERP::Utils::generateUUID()),
                      totalAmount(0.0), totalDiscount(0.0), totalTax(0.0), netAmount(0.0),
                      amountPaid(0.0), amountDue(0.0), currency("VND"),
                      status(SalesOrderStatus::DRAFT) {}
    virtual ~SalesOrderDTO() = default;

    // Utility methods
    std::string getStatusString() const {
        switch (status) {
            case SalesOrderStatus::DRAFT: return "Draft";
            case SalesOrderStatus::PENDING_APPROVAL: return "Pending Approval";
            case SalesOrderStatus::APPROVED: return "Approved";
            case SalesOrderStatus::IN_PROGRESS: return "In Progress";
            case SalesOrderStatus::COMPLETED: return "Completed";
            case SalesOrderStatus::CANCELLED: return "Cancelled";
            case SalesOrderStatus::REJECTED: return "Rejected";
            case SalesOrderStatus::PARTIALLY_DELIVERED: return "Partially Delivered";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_SALESORDER_H