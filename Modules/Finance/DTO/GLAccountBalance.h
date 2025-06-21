// Modules/Finance/DTO/GLAccountBalance.h
#ifndef MODULES_FINANCE_DTO_GLACCOUNTBALANCE_H
#define MODULES_FINANCE_DTO_GLACCOUNTBALANCE_H
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
 * @brief DTO for GL Account Balance entity.
 * Represents the current running balance for a specific General Ledger account.
 */
struct GLAccountBalanceDTO : public BaseDTO {
    std::string glAccountId;                /**< ID của tài khoản GL liên quan. */
    double currentDebitBalance = 0.0;       /**< Tổng số dư bên Nợ hiện tại. */
    double currentCreditBalance = 0.0;      /**< Tổng số dư bên Có hiện tại. */
    std::string currency;                   /**< Đơn vị tiền tệ của số dư. */
    std::chrono::system_clock::time_point lastPostedDate; /**< Ngày cuối cùng có giao dịch được hạch toán vào số dư này. */

    // Default constructor to initialize BaseDTO and specific fields
    GLAccountBalanceDTO() : BaseDTO() {}

    // Virtual destructor for proper polymorphic cleanup
    virtual ~GLAccountBalanceDTO() = default;
};
} // namespace DTO
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DTO_GLACCOUNTBALANCE_H