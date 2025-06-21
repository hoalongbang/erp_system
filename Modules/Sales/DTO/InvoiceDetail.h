// Modules/Sales/DTO/InvoiceDetail.h
#ifndef MODULES_SALES_DTO_INVOICEDETAIL_H
#define MODULES_SALES_DTO_INVOICEDETAIL_H
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
 * @brief Enum for Discount Type (used in invoice, sales order, quotation details).
 */
enum class DiscountType {
    PERCENTAGE = 0,
    FIXED_AMOUNT = 1
};
/**
 * @brief DTO for Invoice Detail entity.
 */
struct InvoiceDetailDTO : public BaseDTO {
    std::string invoiceId;
    std::optional<std::string> salesOrderDetailId; // Optional foreign key to SalesOrderDetailDTO
    std::string productId;
    double quantity;
    double unitPrice;
    double discount;
    DiscountType discountType;
    double taxRate;
    double lineTotal;
    std::optional<std::string> notes;

    InvoiceDetailDTO() : BaseDTO(), quantity(0.0), unitPrice(0.0), discount(0.0),
                         discountType(DiscountType::FIXED_AMOUNT), taxRate(0.0), lineTotal(0.0) {}
    virtual ~InvoiceDetailDTO() = default;
};
} // namespace DTO
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_INVOICEDETAIL_H