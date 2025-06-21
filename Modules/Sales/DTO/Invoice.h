// Modules/Sales/DTO/Invoice.h
#ifndef MODULES_SALES_DTO_INVOICE_H
#define MODULES_SALES_DTO_INVOICE_H
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
 * @brief Enum for Invoice Type.
 */
enum class InvoiceType {
    SALES_INVOICE = 0,
    PROFORMA_INVOICE = 1,
    CREDIT_NOTE = 2,
    DEBIT_NOTE = 3
};
/**
 * @brief Enum for Invoice Status.
 */
enum class InvoiceStatus {
    DRAFT = 0,
    ISSUED = 1,
    PAID = 2,
    PARTIALLY_PAID = 3,
    CANCELLED = 4,
    OVERDUE = 5
};
/**
 * @brief DTO for Invoice entity.
 */
struct InvoiceDTO : public BaseDTO {
    std::string invoiceNumber;
    std::string customerId;
    std::string salesOrderId; // Foreign key to SalesOrderDTO
    InvoiceType type;
    std::chrono::system_clock::time_point invoiceDate;
    std::chrono::system_clock::time_point dueDate;
    InvoiceStatus status;
    double totalAmount;
    double totalDiscount;
    double totalTax;
    double netAmount;
    double amountPaid;
    double amountDue;
    std::string currency;
    std::optional<std::string> notes;

    InvoiceDTO() : BaseDTO(), invoiceNumber(ERP::Utils::generateUUID()), type(InvoiceType::SALES_INVOICE),
                   totalAmount(0.0), totalDiscount(0.0), totalTax(0.0), netAmount(0.0),
                   amountPaid(0.0), amountDue(0.0), currency("VND"),
                   status(InvoiceStatus::DRAFT) {}
    virtual ~InvoiceDTO() = default;

    // Utility methods
    std::string getTypeString() const {
        switch (type) {
            case InvoiceType::SALES_INVOICE: return "Sales Invoice";
            case InvoiceType::PROFORMA_INVOICE: return "Proforma Invoice";
            case InvoiceType::CREDIT_NOTE: return "Credit Note";
            case InvoiceType::DEBIT_NOTE: return "Debit Note";
            default: return "Unknown";
        }
    }
    std::string getStatusString() const {
        switch (status) {
            case InvoiceStatus::DRAFT: return "Draft";
            case InvoiceStatus::ISSUED: return "Issued";
            case InvoiceStatus::PAID: return "Paid";
            case InvoiceStatus::PARTIALLY_PAID: return "Partially Paid";
            case InvoiceStatus::CANCELLED: return "Cancelled";
            case InvoiceStatus::OVERDUE: return "Overdue";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_INVOICE_H