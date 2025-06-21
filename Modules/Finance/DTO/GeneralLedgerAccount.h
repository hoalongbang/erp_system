// Modules/Finance/DTO/GeneralLedgerAccount.h
#ifndef MODULES_FINANCE_DTO_GENERALLEDGERACCOUNT_H
#define MODULES_FINANCE_DTO_GENERALLEDGERACCOUNT_H
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
 * @brief Enum defining types of General Ledger accounts.
 */
enum class GLAccountType {
    ASSET = 0,      /**< Tài sản (ví dụ: Tiền mặt, Tài khoản phải thu). */
    LIABILITY = 1,  /**< Nợ phải trả (ví dụ: Tài khoản phải trả, Vay ngân hàng). */
    EQUITY = 2,     /**< Vốn chủ sở hữu (ví dụ: Vốn góp, Lợi nhuận giữ lại). */
    REVENUE = 3,    /**< Doanh thu (ví dụ: Doanh thu bán hàng, Doanh thu dịch vụ). */
    EXPENSE = 4,    /**< Chi phí (ví dụ: Chi phí bán hàng, Chi phí quản lý doanh nghiệp). */
    OTHER = 99      /**< Các loại tài khoản khác không thuộc các loại trên. */
};

/**
 * @brief Enum defining the normal balance type for a GL account (Debit or Credit).
 */
enum class NormalBalanceType {
    DEBIT = 0,  /**< Số dư thông thường là bên Nợ (ví dụ: Tài sản, Chi phí). */
    CREDIT = 1  /**< Số dư thông thường là bên Có (ví dụ: Nợ phải trả, Vốn chủ sở hữu, Doanh thu). */
};

/**
 * @brief DTO for General Ledger Account entity.
 * Represents an account in the chart of accounts (e.g., Cash, Accounts Payable, Sales Revenue).
 */
struct GeneralLedgerAccountDTO : public BaseDTO {
    std::string accountNumber;                  /**< Số tài khoản GL duy nhất (ví dụ: "111", "511"). */
    std::string accountName;                    /**< Tên tài khoản GL (ví dụ: "Tiền mặt", "Doanh thu bán hàng"). */
    GLAccountType accountType;                  /**< Loại tài khoản GL (Asset, Liability, Equity, Revenue, Expense). */
    NormalBalanceType normalBalance;            /**< Loại số dư thông thường (Nợ hoặc Có). */
    std::optional<std::string> parentAccountId; /**< ID của tài khoản cha nếu là tài khoản con (tùy chọn). */
    std::optional<std::string> description;     /**< Mô tả tài khoản (tùy chọn). */

    // Default constructor
    GeneralLedgerAccountDTO() = default;

    // Virtual destructor for proper polymorphic cleanup
    virtual ~GeneralLedgerAccountDTO() = default;

    /**
     * @brief Converts a GLAccountType enum value to its string representation.
     * @return The string representation of the account type.
     */
    std::string getTypeString() const {
        switch (accountType) {
            case GLAccountType::ASSET: return "Asset";
            case GLAccountType::LIABILITY: return "Liability";
            case GLAccountType::EQUITY: return "Equity";
            case GLAccountType::REVENUE: return "Revenue";
            case GLAccountType::EXPENSE: return "Expense";
            case GLAccountType::OTHER: return "Other";
            default: return "Unknown";
        }
    }

    /**
     * @brief Converts a NormalBalanceType enum value to its string representation.
     * @return The string representation of the normal balance type.
     */
    std::string getNormalBalanceString() const {
        switch (normalBalance) {
            case NormalBalanceType::DEBIT: return "Debit";
            case NormalBalanceType::CREDIT: return "Credit";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DTO_GENERALLEDGERACCOUNT_H