// Modules/Sales/DTO/Payment.h
#ifndef MODULES_SALES_DTO_PAYMENT_H
#define MODULES_SALES_DTO_PAYMENT_H
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
 * @brief Enum for Payment Method.
 */
enum class PaymentMethod {
    CASH = 0,
    BANK_TRANSFER = 1,
    CREDIT_CARD = 2,
    ONLINE_PAYMENT = 3,
    OTHER = 4
};
/**
 * @brief Enum for Payment Status.
 */
enum class PaymentStatus {
    PENDING = 0,
    COMPLETED = 1,
    FAILED = 2,
    REFUNDED = 3,
    CANCELLED = 4
};
/**
 * @brief DTO for Payment entity.
 */
struct PaymentDTO : public BaseDTO {
    std::string customerId;
    std::string invoiceId;
    std::string paymentNumber;
    double amount;
    std::chrono::system_clock::time_point paymentDate;
    PaymentMethod method;
    PaymentStatus status;
    std::optional<std::string> transactionId; // External transaction ID (e.g., from payment gateway)
    std::optional<std::string> notes;
    std::string currency;

    PaymentDTO() : BaseDTO(), paymentNumber(ERP::Utils::generateUUID()), amount(0.0),
                   method(PaymentMethod::CASH), status(PaymentStatus::PENDING), currency("VND") {}
    virtual ~PaymentDTO() = default;

    // Utility methods
    std::string getMethodString() const {
        switch (method) {
            case PaymentMethod::CASH: return "Cash";
            case PaymentMethod::BANK_TRANSFER: return "Bank Transfer";
            case PaymentMethod::CREDIT_CARD: return "Credit Card";
            case PaymentMethod::ONLINE_PAYMENT: return "Online Payment";
            case PaymentMethod::OTHER: return "Other";
            default: return "Unknown";
        }
    }
    std::string getStatusString() const {
        switch (status) {
            case PaymentStatus::PENDING: return "Pending";
            case PaymentStatus::COMPLETED: return "Completed";
            case PaymentStatus::FAILED: return "Failed";
            case PaymentStatus::REFUNDED: return "Refunded";
            case PaymentStatus::CANCELLED: return "Cancelled";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_PAYMENT_H