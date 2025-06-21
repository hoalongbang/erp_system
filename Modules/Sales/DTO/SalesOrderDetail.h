// Modules/Sales/DTO/SalesOrderDetail.h
#ifndef MODULES_SALES_DTO_SALESORDERDETAIL_H
#define MODULES_SALES_DTO_SALESORDERDETAIL_H
#include <string>
#include <vector>
#include <optional>
#include "DataObjects/BaseDTO.h"// Updated include path
#include "Modules/Common/Common.h>// For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h" // For UUID generation
// Includes for nested enums if they are not forward declared
#include "Modules/Sales/DTO/InvoiceDetail.h" // For DiscountType
namespace ERP {
namespace Sales { // Namespace for Sales module
namespace DTO {
/**
 * @brief DTO for Sales Order Detail entity.
 */
struct SalesOrderDetailDTO : public BaseDTO {
    std::string salesOrderId;
    std::string productId;
    double quantity;
    double unitPrice;
    double discount;
    DiscountType discountType; // Using DiscountType from InvoiceDetail.h
    double taxRate;
    double lineTotal;
    double deliveredQuantity;
    double invoicedQuantity;
    bool isFullyDelivered = false;
    bool isFullyInvoiced = false;
    std::optional<std::string> notes;

    SalesOrderDetailDTO() : BaseDTO(), quantity(0.0), unitPrice(0.0), discount(0.0),
                            discountType(DiscountType::FIXED_AMOUNT), taxRate(0.0), lineTotal(0.0),
                            deliveredQuantity(0.0), invoicedQuantity(0.0) {}
    virtual ~SalesOrderDetailDTO() = default;
};
} // namespace DTO
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_SALESORDERDETAIL_H