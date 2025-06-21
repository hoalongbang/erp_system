// Modules/Finance/DTO/AccountReceivableBalance.h
#ifndef MODULES_FINANCE_DTO_ACCOUNTRECEIVABLEBALANCE_H
#define MODULES_FINANCE_DTO_ACCOUNTRECEIVABLEBALANCE_H
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
 * @brief DTO for Account Receivable Balance entity.
 * Represents the current outstanding balance for a customer.
 */
struct AccountReceivableBalanceDTO : public BaseDTO {
    std::string customerId;                 /**< ID của khách hàng liên quan. */
    double outstandingBalance = 0.0;        /**< Số dư nợ hiện tại của khách hàng. */
    std::string currency;                   /**< Đơn vị tiền tệ của số dư. */
    std::chrono::system_clock::time_point lastActivityDate; /**< Ngày hoạt động cuối cùng trên tài khoản này. */

    // Default constructor to initialize BaseDTO and specific fields
    AccountReceivableBalanceDTO() : BaseDTO() {}

    // Virtual destructor for proper polymorphic cleanup
    virtual ~AccountReceivableBalanceDTO() = default;
};
} // namespace DTO
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DTO_ACCOUNTRECEIVABLEBALANCE_H