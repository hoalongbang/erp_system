// Modules/Finance/Service/AccountReceivableService.h
#ifndef MODULES_FINANCE_SERVICE_ACCOUNTRECEIVABLESERVICE_H
#define MODULES_FINANCE_SERVICE_ACCOUNTRECEIVABLESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

// Rút gọn các include paths
#include "BaseService.h"               // Base Service
#include "AccountReceivableBalance.h"  // DTO
#include "AccountReceivableTransaction.h" // NEW: DTO for AR Transactions
#include "Customer.h"                  // Customer DTO
#include "Invoice.h"                   // Invoice DTO
#include "Payment.h"                   // Payment DTO
#include "AccountReceivableDAO.h"      // AccountReceivable DAO
#include "AccountReceivableTransactionDAO.h" // NEW: AccountReceivableTransactionDAO
#include "CustomerService.h"           // Customer Service interface (dependency)
#include "InvoiceService.h"            // Invoice Service interface (dependency)
#include "PaymentService.h"            // Payment Service interface (dependency)
#include "ISecurityManager.h"          // Security Manager interface
#include "EventBus.h"                  // EventBus
#include "Logger.h"                    // Logger
#include "ErrorHandler.h"              // ErrorHandler
#include "Common.h"                    // Common enums/constants
#include "Utils.h"                     // Utilities
#include "DateUtils.h"                 // Date utilities

// Forward declarations for services that are dependencies but might cause circular includes
namespace ERP { namespace Customer { namespace Services { class ICustomerService; }}}
namespace ERP { namespace Sales { namespace Services { class IInvoiceService; }}}
namespace ERP { namespace Sales { namespace Services { class IPaymentService; }}}

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

/**
 * @brief Default implementation of IAccountReceivableService.
 * This class uses AccountReceivableDAO, AccountReceivableTransactionDAO and ISecurityManager.
 */
class AccountReceivableService : public IAccountReceivableService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for AccountReceivableService.
     * @param arBalanceDAO Shared pointer to AccountReceivableDAO (for balances).
     * @param arTransactionDAO Shared pointer to AccountReceivableTransactionDAO (NEW: for transactions).
     * @param customerService Shared pointer to ICustomerService (dependency).
     * @param invoiceService Shared pointer to IInvoiceService (dependency).
     * @param paymentService Shared pointer to IPaymentService (dependency).
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    AccountReceivableService(std::shared_ptr<DAOs::AccountReceivableDAO> arBalanceDAO,
                             std::shared_ptr<DAOs::AccountReceivableTransactionDAO> arTransactionDAO, // NEW
                             std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
                             std::shared_ptr<ERP::Sales::Services::IInvoiceService> invoiceService,
                             std::shared_ptr<ERP::Sales::Services::IPaymentService> paymentService,
                             std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                             std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                             std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                             std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    bool updateCustomerARBalance(
        const std::string& customerId,
        double amount,
        const std::string& currency,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool adjustARBalance(
        const std::string& customerId,
        double adjustmentAmount,
        const std::string& currency,
        const std::string& reason,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Finance::DTO::AccountReceivableBalanceDTO> getCustomerARBalance(
        const std::string& customerId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Finance::DTO::AccountReceivableBalanceDTO> getAllARBalances(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;

    // NEW: Account Receivable Transaction Management Implementations
    std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> recordARTransaction(
        const ERP::Finance::DTO::AccountReceivableTransactionDTO& transactionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> getARTransactionById(
        const std::string& transactionId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> getAllARTransactions(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;

private:
    std::shared_ptr<DAOs::AccountReceivableDAO> arBalanceDAO_; // For balances
    std::shared_ptr<DAOs::AccountReceivableTransactionDAO> arTransactionDAO_; // NEW: For transactions
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<ERP::Sales::Services::IInvoiceService> invoiceService_;
    std::shared_ptr<ERP::Sales::Services::IPaymentService> paymentService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();

    // Helper to get transaction number (could be from a dedicated numbering service in a real ERP)
    std::string generateTransactionNumber();
};
} // namespace Services
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_SERVICE_ACCOUNTRECEIVABLESERVICE_H