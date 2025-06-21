// Modules/Finance/DTO/AccountReceivableTransaction.h
#ifndef MODULES_FINANCE_DTO_ACCOUNTRECEIVABLETRANSACTION_H
#define MODULES_FINANCE_DTO_ACCOUNTRECEIVABLETRANSACTION_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
namespace Finance {
namespace DTO {

/**
 * @brief Enum defining types of Account Receivable transactions.
 */
enum class ARTransactionType {
    INVOICE = 0,    /**< Giao dịch phát sinh từ hóa đơn bán hàng (làm tăng nợ). */
    PAYMENT = 1,    /**< Giao dịch thanh toán từ khách hàng (làm giảm nợ). */
    ADJUSTMENT = 2, /**< Điều chỉnh công nợ (tăng hoặc giảm). */
    CREDIT_MEMO = 3,/**< Ghi nợ tín dụng (làm giảm nợ). */
    DEBIT_MEMO = 4  /**< Ghi nợ (làm tăng nợ). */
};

/**
 * @brief DTO for Account Receivable Transaction entity.
 * Represents a single transaction affecting a customer's AR balance.
 */
struct AccountReceivableTransactionDTO : public BaseDTO {
    std::string customerId;                 /**< ID của khách hàng liên quan. */
    ARTransactionType type;                 /**< Loại giao dịch AR (Invoice, Payment, Adjustment, etc.). */
    double amount;                          /**< Số tiền của giao dịch. */
    std::string currency;                   /**< Đơn vị tiền tệ của giao dịch. */
    std::chrono::system_clock::time_point transactionDate; /**< Ngày diễn ra giao dịch. */
    std::optional<std::string> referenceDocumentId; /**< ID của tài liệu gốc (ví dụ: Invoice ID, Payment ID) (tùy chọn). */
    std::optional<std::string> referenceDocumentType; /**< Loại tài liệu gốc (ví dụ: "Invoice", "Payment") (tùy chọn). */
    std::optional<std::string> notes;       /**< Ghi chú về giao dịch (tùy chọn). */

    // Default constructor to initialize BaseDTO and specific fields
    AccountReceivableTransactionDTO() : BaseDTO() {}

    // Virtual destructor for proper polymorphic cleanup
    virtual ~AccountReceivableTransactionDTO() = default;

    /**
     * @brief Converts an ARTransactionType enum value to its string representation.
     * @return The string representation of the transaction type.
     */
    std::string getTypeString() const {
        switch (type) {
            case ARTransactionType::INVOICE: return "Invoice";
            case ARTransactionType::PAYMENT: return "Payment";
            case ARTransactionType::ADJUSTMENT: return "Adjustment";
            case ARTransactionType::CREDIT_MEMO: return "Credit Memo";
            case ARTransactionType::DEBIT_MEMO: return "Debit Memo";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DTO_ACCOUNTRECEIVABLETRANSACTION_H