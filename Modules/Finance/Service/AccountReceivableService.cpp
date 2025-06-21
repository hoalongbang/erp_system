// Modules/Finance/Service/AccountReceivableService.cpp
#include "AccountReceivableService.h" // Standard includes
#include "AccountReceivableBalance.h"   // AR Balance DTO
#include "AccountReceivableTransaction.h" // AR Transaction DTO
#include "Customer.h"                   // Customer DTO
#include "Invoice.h"                    // Invoice DTO
#include "Payment.h"                    // Payment DTO
#include "Event.h"                      // Event DTO
#include "ConnectionPool.h"             // ConnectionPool
#include "DBConnection.h"               // DBConnection
#include "Common.h"                     // Common Enums/Constants
#include "Utils.h"                      // Utility functions
#include "DateUtils.h"                  // Date utility functions
#include "AutoRelease.h"                // RAII helper
#include "ISecurityManager.h"           // Security Manager interface
#include "UserService.h"                // User Service (for audit logging)
#include "CustomerService.h"            // CustomerService (for customer validation)
#include "InvoiceService.h"             // InvoiceService (for invoice validation)
#include "PaymentService.h"             // PaymentService (for payment validation)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
namespace Finance {
namespace Services {

AccountReceivableService::AccountReceivableService(
    std::shared_ptr<DAOs::AccountReceivableDAO> arBalanceDAO,
    std::shared_ptr<DAOs::AccountReceivableTransactionDAO> arTransactionDAO, // NEW
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
    std::shared_ptr<ERP::Sales::Services::IInvoiceService> invoiceService,
    std::shared_ptr<ERP::Sales::Services::IPaymentService> paymentService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
      arBalanceDAO_(arBalanceDAO),
      arTransactionDAO_(arTransactionDAO), // NEW
      customerService_(customerService),
      invoiceService_(invoiceService),
      paymentService_(paymentService) {
    
    if (!arBalanceDAO_ || !arTransactionDAO_ || !customerService_ || !invoiceService_ || !paymentService_ || !securityManager_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "AccountReceivableService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ công nợ phải thu.");
        ERP::Logger::Logger::getInstance().critical("AccountReceivableService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("AccountReceivableService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("AccountReceivableService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

// Helper to generate a unique transaction number (simplified)
std::string AccountReceivableService::generateTransactionNumber() {
    return "AR-TXN-" + ERP::Utils::generateUUID().substr(0, 8);
}

bool AccountReceivableService::updateCustomerARBalance(
    const std::string& customerId,
    double amount,
    const std::string& currency,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableService: Attempting to update AR balance for customer: " + customerId + " by amount: " + std::to_string(amount) + " " + currency + ".");

    // This is an internal method, permission check should happen at the calling service layer.
    // However, if called directly from UI, a permission like "Finance.ManageARBalanceInternal" might be needed.

    // Validate Customer existence
    std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(customerId, userRoleIds);
    if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("AccountReceivableService: Invalid Customer ID provided or customer is not active: " + customerId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc không hoạt động.");
        return false;
    }

    std::shared_ptr<ERP::Database::DBConnection> db_conn = connectionPool_->getConnection();
    ERP::Utils::AutoRelease releaseGuard([&]() { if (db_conn) connectionPool_->releaseConnection(db_conn); });
    if (!db_conn) {
        ERP::Logger::Logger::getInstance().critical("AccountReceivableService: Database connection is null. Cannot update AR balance.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "Database connection is null.", "Lỗi hệ thống: Không có kết nối cơ sở dữ liệu.");
        return false;
    }

    try {
        db_conn->beginTransaction();

        std::map<std::string, std::any> filter;
        filter["customer_id"] = customerId;
        std::vector<ERP::Finance::DTO::AccountReceivableBalanceDTO> existingBalances = arBalanceDAO_->getARBalances(filter);

        if (existingBalances.empty()) {
            // Create new balance record
            ERP::Finance::DTO::AccountReceivableBalanceDTO newBalance;
            newBalance.id = ERP::Utils::generateUUID();
            newBalance.customerId = customerId;
            newBalance.outstandingBalance = amount;
            newBalance.currency = currency;
            newBalance.lastActivityDate = ERP::Utils::DateUtils::now();
            newBalance.createdAt = newBalance.lastActivityDate;
            newBalance.createdBy = currentUserId;
            newBalance.status = ERP::Common::EntityStatus::ACTIVE;

            if (!arBalanceDAO_->create(newBalance)) {
                ERP::Logger::Logger::getInstance().error("AccountReceivableService: Failed to create new AR balance for customer " + customerId + " in DAO.");
                db_conn->rollbackTransaction();
                return false;
            }
            ERP::Logger::Logger::getInstance().info("AccountReceivableService: Created new AR balance for customer " + customerId + ". Balance: " + std::to_string(newBalance.outstandingBalance));
        } else {
            // Update existing balance
            ERP::Finance::DTO::AccountReceivableBalanceDTO existingBalance = existingBalances[0];
            existingBalance.outstandingBalance += amount;
            existingBalance.lastActivityDate = ERP::Utils::DateUtils::now();
            existingBalance.updatedAt = existingBalance.lastActivityDate;
            existingBalance.updatedBy = currentUserId;

            if (!arBalanceDAO_->update(existingBalance)) {
                ERP::Logger::Logger::getInstance().error("AccountReceivableService: Failed to update AR balance for customer " + customerId + " in DAO.");
                db_conn->rollbackTransaction();
                return false;
            }
            ERP::Logger::Logger::getInstance().info("AccountReceivableService: Updated AR balance for customer " + customerId + ". New balance: " + std::to_string(existingBalance.outstandingBalance));
        }

        db_conn->commitTransaction();
        ERP::Logger::Logger::getInstance().info("AccountReceivableService: AR balance for customer " + customerId + " updated successfully.");
        // Audit log for internal balance update (could be INFO or DEBUG depending on verbosity)
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::DEBUG, // or INFO
                       "Finance", "ARBalanceUpdate", customerId, "Customer", customer->name,
                       std::nullopt, std::nullopt, "AR balance updated by " + std::to_string(amount)); // Before/after data could be more specific
        return true;
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().critical("AccountReceivableService: Exception during updateCustomerARBalance: " + std::string(e.what()));
        db_conn->rollbackTransaction();
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Lỗi trong quá trình cập nhật số dư công nợ phải thu: " + std::string(e.what()));
        return false;
    }
}

bool AccountReceivableService::adjustARBalance(
    const std::string& customerId,
    double adjustmentAmount,
    const std::string& currency,
    const std::string& reason,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableService: Attempting to manually adjust AR balance for customer: " + customerId + " by: " + std::to_string(adjustmentAmount) + " " + currency + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.AdjustARBalance", "Bạn không có quyền điều chỉnh số dư công nợ phải thu.")) {
        return false;
    }

    // 1. Validate Customer existence
    std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(customerId, userRoleIds);
    if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("AccountReceivableService: Invalid Customer ID provided or customer is not active: " + customerId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc không hoạt động.");
        return false;
    }

    std::shared_ptr<ERP::Database::DBConnection> db_conn = connectionPool_->getConnection();
    ERP::Utils::AutoRelease releaseGuard([&]() { if (db_conn) connectionPool_->releaseConnection(db_conn); });
    if (!db_conn) {
        ERP::Logger::Logger::getInstance().critical("AccountReceivableService: Database connection is null. Cannot adjust AR balance.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "Database connection is null.", "Lỗi hệ thống: Không có kết nối cơ sở dữ liệu.");
        return false;
    }

    try {
        db_conn->beginTransaction();

        // Step 1: Update the AR Balance
        if (!updateCustomerARBalance(customerId, adjustmentAmount, currency, currentUserId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().error("AccountReceivableService: Failed to update customer AR balance during adjustment.");
            db_conn->rollbackTransaction();
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể cập nhật số dư công nợ phải thu.");
            return false;
        }

        // Step 2: Record an AR Transaction for the adjustment
        ERP::Finance::DTO::AccountReceivableTransactionDTO arTransaction;
        arTransaction.id = ERP::Utils::generateUUID();
        arTransaction.customerId = customerId;
        arTransaction.type = ERP::Finance::DTO::ARTransactionType::ADJUSTMENT;
        arTransaction.amount = adjustmentAmount;
        arTransaction.currency = currency;
        arTransaction.transactionDate = ERP::Utils::DateUtils::now();
        arTransaction.notes = "Manual adjustment: " + reason;
        arTransaction.createdAt = arTransaction.transactionDate;
        arTransaction.createdBy = currentUserId;
        arTransaction.status = ERP::Common::EntityStatus::ACTIVE;

        if (!arTransactionDAO_->save(arTransaction)) { // NEW: Use arTransactionDAO_
            ERP::Logger::Logger::getInstance().error("AccountReceivableService: Failed to record AR adjustment transaction.");
            db_conn->rollbackTransaction();
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể ghi nhận giao dịch điều chỉnh công nợ.");
            return false;
        }
        
        db_conn->commitTransaction();
        ERP::Logger::Logger::getInstance().info("AccountReceivableService: AR balance for customer " + customerId + " adjusted successfully by " + std::to_string(adjustmentAmount) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Finance", "ARAdjustment", customerId, "Customer", customer->name,
                       std::nullopt, std::nullopt, "AR balance adjusted by " + std::to_string(adjustmentAmount) + ". Reason: " + reason);
        return true;
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().critical("AccountReceivableService: Exception during adjustARBalance: " + std::string(e.what()));
        db_conn->rollbackTransaction();
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Lỗi trong quá trình điều chỉnh số dư công nợ phải thu: " + std::string(e.what()));
        return false;
    }
}

std::optional<ERP::Finance::DTO::AccountReceivableBalanceDTO> AccountReceivableService::getCustomerARBalance(
    const std::string& customerId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("AccountReceivableService: Retrieving AR balance for customer: " + customerId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewARBalance", "Bạn không có quyền xem số dư công nợ phải thu.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["customer_id"] = customerId;
    std::vector<ERP::Finance::DTO::AccountReceivableBalanceDTO> balances = arBalanceDAO_->getARBalances(filter);
    if (!balances.empty()) {
        return balances[0];
    }
    ERP::Logger::Logger::getInstance().debug("AccountReceivableService: AR balance for customer " + customerId + " not found.");
    return std::nullopt;
}

std::vector<ERP::Finance::DTO::AccountReceivableBalanceDTO> AccountReceivableService::getAllARBalances(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableService: Retrieving all AR balances with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewARBalance", "Bạn không có quyền xem tất cả số dư công nợ phải thu.")) {
        return {};
    }

    return arBalanceDAO_->getARBalances(filter);
}


// NEW: Account Receivable Transaction Management Implementations

std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> AccountReceivableService::recordARTransaction(
    const ERP::Finance::DTO::AccountReceivableTransactionDTO& transactionDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableService: Attempting to record AR transaction for customer " + transactionDTO.customerId + ", type " + transactionDTO.getTypeString() + ".");

    // This is typically called by other services like SalesInvoiceService, SalesPaymentService.
    // Permission check for `recordARTransaction` might be internal or via the calling service's permission.
    // For simplicity, we assume the calling service already handled permissions.
    // If exposed directly to UI, needs a specific permission like "Finance.RecordARTransaction".

    // Validate Customer existence
    std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(transactionDTO.customerId, userRoleIds);
    if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("AccountReceivableService: Invalid Customer ID provided for AR transaction: " + transactionDTO.customerId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc không hoạt động.");
        return std::nullopt;
    }
    // Validate amount/currency
    if (transactionDTO.amount <= 0 && transactionDTO.type != ERP::Finance::DTO::ARTransactionType::ADJUSTMENT) { // Adjustments can be negative
        ERP::Logger::Logger::getInstance().warning("AccountReceivableService: Transaction amount must be positive for type " + transactionDTO.getTypeString());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số tiền giao dịch không hợp lệ.");
        return std::nullopt;
    }

    ERP::Finance::DTO::AccountReceivableTransactionDTO newTransaction = transactionDTO;
    newTransaction.id = ERP::Utils::generateUUID();
    // newTransaction.transactionNumber = generateTransactionNumber(); // If you want unique numbers for transactions
    newTransaction.createdAt = ERP::Utils::DateUtils::now();
    newTransaction.createdBy = currentUserId;
    newTransaction.status = ERP::Common::EntityStatus::ACTIVE; // Transactions are generally active upon creation

    std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> createdTransaction = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!arTransactionDAO_->save(newTransaction)) { // NEW: Use arTransactionDAO_
                ERP::Logger::Logger::getInstance().error("AccountReceivableService: Failed to record AR transaction in DAO.");
                return false;
            }
            // After recording transaction, update customer's AR balance
            if (!updateCustomerARBalance(newTransaction.customerId, newTransaction.amount, newTransaction.currency, currentUserId, userRoleIds)) {
                ERP::Logger::Logger::getInstance().error("AccountReceivableService: Failed to update customer AR balance after recording transaction.");
                return false;
            }
            createdTransaction = newTransaction;
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::ARTransactionRecordedEvent>(newTransaction.id, newTransaction.customerId, newTransaction.type)); // Assuming such an event
            return true;
        },
        "AccountReceivableService", "recordARTransaction"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("AccountReceivableService: AR transaction " + newTransaction.id + " recorded successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Finance", "ARTransaction", newTransaction.id, "ARTransaction", newTransaction.customerId + ":" + newTransaction.getTypeString(),
                       std::nullopt, newTransaction.toMap(), "AR transaction recorded.");
        return createdTransaction;
    }
    return std::nullopt;
}

std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> AccountReceivableService::getARTransactionById(
    const std::string& transactionId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("AccountReceivableService: Retrieving AR transaction by ID: " + transactionId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewARTransactions", "Bạn không có quyền xem giao dịch công nợ phải thu.")) {
        return std::nullopt;
    }

    return arTransactionDAO_->findById(transactionId); // NEW: Use arTransactionDAO_
}

std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> AccountReceivableService::getAllARTransactions(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableService: Retrieving all AR transactions with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewARTransactions", "Bạn không có quyền xem tất cả giao dịch công nợ phải thu.")) {
        return {};
    }

    return arTransactionDAO_->getAccountReceivableTransactions(filter); // NEW: Use arTransactionDAO_
}

} // namespace Services
} // namespace Finance
} // namespace ERP