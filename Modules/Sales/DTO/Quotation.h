// Modules/Sales/DTO/Quotation.h
#ifndef MODULES_SALES_DTO_QUOTATION_H
#define MODULES_SALES_DTO_QUOTATION_H
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
 * @brief Enum for Quotation Status.
 */
enum class QuotationStatus {
    DRAFT = 0,
    SENT = 1,
    ACCEPTED = 2,
    REJECTED = 3,
    EXPIRED = 4,
    CANCELLED = 5
};
/**
 * @brief DTO for Quotation entity.
 */
struct QuotationDTO : public BaseDTO {
    std::string quotationNumber;
    std::string customerId;
    std::string requestedByUserId;
    std::chrono::system_clock::time_point quotationDate;
    std::chrono::system_clock::time_point validUntilDate;
    QuotationStatus status;
    double totalAmount;
    double totalDiscount;
    double totalTax;
    double netAmount;
    std::string currency;
    std::optional<std::string> paymentTerms;
    std::optional<std::string> deliveryTerms;
    std::optional<std::string> notes;

    QuotationDTO() : BaseDTO(), quotationNumber(ERP::Utils::generateUUID()),
                     totalAmount(0.0), totalDiscount(0.0), totalTax(0.0), netAmount(0.0),
                     currency("VND"), status(QuotationStatus::DRAFT) {}
    virtual ~QuotationDTO() = default;

    // Utility methods
    std::string getStatusString() const {
        switch (status) {
            case QuotationStatus::DRAFT: return "Draft";
            case QuotationStatus::SENT: return "Sent";
            case QuotationStatus::ACCEPTED: return "Accepted";
            case QuotationStatus::REJECTED: return "Rejected";
            case QuotationStatus::EXPIRED: return "Expired";
            case QuotationStatus::CANCELLED: return "Cancelled";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_QUOTATION_H