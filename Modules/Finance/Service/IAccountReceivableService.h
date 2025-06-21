// Modules/Finance/Service/IAccountReceivableService.h
#ifndef MODULES_FINANCE_SERVICE_IACCOUNTRECEIVABLESERVICE_H
#define MODULES_FINANCE_SERVICE_IACCOUNTRECEIVABLESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "BaseService.h"               // Base Service
#include "AccountReceivableBalance.h"  // DTO
#include "AccountReceivableTransaction.h" // NEW: DTO for AR Transactions

namespace ERP {
namespace Finance {
namespace Services {

/**
 * @brief IAccountReceivableService interface defines operations for managing accounts receivable.
 */
class IAccountReceivableService {
public:
    virtual ~IAccountReceivableService() = default;
    /**
     * @brief Creates or updates an account receivable balance for a customer.
     * This is typically called internally when invoices are issued or payments are received.
     * @param customerId ID of the customer.
     * @param amount The amount to affect the balance (positive for increase, negative for decrease).
     * @param currency The currency of the transaction.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateCustomerARBalance(
        const std::string& customerId,
        double amount,
        const std::string& currency,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Manually adjusts an account receivable balance. Requires specific permission.
     * @param customerId ID of the customer.
     * @param adjustmentAmount The amount to adjust the balance by.
     * @param currency The currency of the adjustment.
     * @param reason Reason for the adjustment.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if adjustment is successful, false otherwise.
     */
    virtual bool adjustARBalance(
        const std::string& customerId,
        double adjustmentAmount,
        const std::string& currency,
        const std::string& reason,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves AR balance information for a specific customer.
     * @param customerId ID of the customer.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional AccountReceivableBalanceDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::AccountReceivableBalanceDTO> getCustomerARBalance(
        const std::string& customerId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all AR balances or balances matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching AccountReceivableBalanceDTOs.
     */
    virtual std::vector<ERP::Finance::DTO::AccountReceivableBalanceDTO> getAllARBalances(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;

    // NEW: Account Receivable Transaction Management
    /**
     * @brief Records a new account receivable transaction.
     * This method is typically called internally by other services (e.g., SalesInvoiceService, SalesPaymentService).
     * @param transactionDTO DTO containing transaction details.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user.
     * @return An optional AccountReceivableTransactionDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> recordARTransaction(
        const ERP::Finance::DTO::AccountReceivableTransactionDTO& transactionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves account receivable transaction information by ID.
     * @param transactionId ID of the transaction to retrieve.
     * @param userRoleIds Roles of the user.
     * @return An optional AccountReceivableTransactionDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> getARTransactionById(
        const std::string& transactionId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all account receivable transactions matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user.
     * @return Vector of matching AccountReceivableTransactionDTOs.
     */
    virtual std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> getAllARTransactions(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
};

} // namespace Services
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_SERVICE_IACCOUNTRECEIVABLESERVICE_H