// Modules/Sales/Service/SalesPaymentService.cpp
#include "SalesPaymentService.h" // Đã rút gọn include
#include "Payment.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "CustomerService.h" // Đã rút gọn include
#include "InvoiceService.h" // Đã rút gọn include
#include "AccountReceivableService.h" // For AR updates
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Sales {
namespace Services {

SalesPaymentService::SalesPaymentService(
    std::shared_ptr<DAOs::PaymentDAO> paymentDAO,
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
    std::shared_ptr<IInvoiceService> invoiceService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      paymentDAO_(paymentDAO), customerService_(customerService), invoiceService_(invoiceService) {
    if (!paymentDAO_ || !customerService_ || !invoiceService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "SalesPaymentService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ thanh toán bán hàng.");
        ERP::Logger::Logger::getInstance().critical("SalesPaymentService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("SalesPaymentService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("SalesPaymentService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Sales::DTO::PaymentDTO> SalesPaymentService::createPayment(
    const ERP::Sales::DTO::PaymentDTO& paymentDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesPaymentService: Attempting to create payment: " + paymentDTO.paymentNumber + " for invoice: " + paymentDTO.invoiceId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.RecordPayment", "Bạn không có quyền ghi nhận thanh toán.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (paymentDTO.paymentNumber.empty() || paymentDTO.customerId.empty() || paymentDTO.invoiceId.empty() || paymentDTO.amount <= 0 || paymentDTO.currency.empty()) {
        ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Invalid input for payment creation (missing number, customer, invoice, non-positive amount, or currency).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesPaymentService: Invalid input for payment creation.", "Thông tin thanh toán không đầy đủ hoặc không hợp lệ.");
        return std::nullopt;
    }

    // Check if payment number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["payment_number"] = paymentDTO.paymentNumber;
    if (paymentDAO_->count(filterByNumber) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Payment with number " + paymentDTO.paymentNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesPaymentService: Payment with number " + paymentDTO.paymentNumber + " already exists.", "Số thanh toán đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate Customer existence
    std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(paymentDTO.customerId, userRoleIds);
    if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Invalid Customer ID provided or customer is not active: " + paymentDTO.customerId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc khách hàng không hoạt động.");
        return std::nullopt;
    }

    // Validate Invoice existence and status (e.g., must be ISSUED or PARTIALLY_PAID, not PAID or CANCELLED)
    std::optional<ERP::Sales::DTO::InvoiceDTO> invoice = invoiceService_->getInvoiceById(paymentDTO.invoiceId, userRoleIds);
    if (!invoice || invoice->status == ERP::Sales::DTO::InvoiceStatus::CANCELLED || invoice->status == ERP::Sales::DTO::InvoiceStatus::PAID) {
        ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Invalid Invoice ID provided or invoice not in valid status for payment: " + paymentDTO.invoiceId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID hóa đơn không hợp lệ hoặc không ở trạng thái hợp lệ để thanh toán.");
        return std::nullopt;
    }
    // Check if payment amount exceeds amount due
    if (paymentDTO.amount > invoice->amountDue) {
        ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Payment amount " + std::to_string(paymentDTO.amount) + " exceeds invoice due amount " + std::to_string(invoice->amountDue) + " for invoice " + invoice->invoiceNumber + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số tiền thanh toán vượt quá số tiền còn nợ của hóa đơn.");
        return std::nullopt;
    }

    ERP::Sales::DTO::PaymentDTO newPayment = paymentDTO;
    newPayment.id = ERP::Utils::generateUUID();
    newPayment.createdAt = ERP::Utils::DateUtils::now();
    newPayment.createdBy = currentUserId;
    newPayment.status = ERP::Sales::DTO::PaymentStatus::PENDING; // Default status, might become COMPLETED immediately for cash payments

    std::optional<ERP::Sales::DTO::PaymentDTO> createdPayment = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!paymentDAO_->create(newPayment)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to create payment " + newPayment.paymentNumber + " in DAO.");
                return false;
            }
            createdPayment = newPayment;
            
            // Update Invoice's amount_paid and amount_due, and its status
            ERP::Sales::DTO::InvoiceDTO updatedInvoice = *invoice;
            updatedInvoice.amountPaid += newPayment.amount;
            updatedInvoice.amountDue -= newPayment.amount;
            
            if (updatedInvoice.amountDue <= 0.001) { // Floating point comparison
                updatedInvoice.status = ERP::Sales::DTO::InvoiceStatus::PAID;
            } else {
                updatedInvoice.status = ERP::Sales::DTO::InvoiceStatus::PARTIALLY_PAID;
            }
            updatedInvoice.updatedAt = ERP::Utils::DateUtils::now();
            updatedInvoice.updatedBy = currentUserId;

            if (!invoiceService_->updateInvoice(updatedInvoice, currentUserId, userRoleIds)) { // Calls updateInvoice
                 ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to update invoice " + updatedInvoice.invoiceNumber + " during payment creation.");
                 return false;
            }

            // Record AR transaction
            ERP::Finance::DTO::AccountReceivableTransactionDTO arTransaction;
            arTransaction.id = ERP::Utils::generateUUID();
            arTransaction.customerId = newPayment.customerId;
            arTransaction.type = ERP::Finance::DTO::ARTransactionType::PAYMENT;
            arTransaction.amount = newPayment.amount;
            arTransaction.currency = newPayment.currency;
            arTransaction.transactionDate = newPayment.paymentDate;
            arTransaction.referenceDocumentId = newPayment.id;
            arTransaction.referenceDocumentType = "Payment";
            arTransaction.notes = "Payment for Invoice " + invoice->invoiceNumber + " via Payment " + newPayment.paymentNumber;
            arTransaction.createdAt = newPayment.createdAt;
            arTransaction.createdBy = newPayment.createdBy;
            arTransaction.status = ERP::Common::EntityStatus::ACTIVE;

            // Call AccountReceivableService to record transaction
            if (!securityManager_->getAccountReceivableService()->recordARTransaction(arTransaction, currentUserId, userRoleIds)) {
                ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to record AR transaction for payment " + newPayment.paymentNumber + ".");
                return false;
            }

            // If payment method implies immediate completion (e.g., Cash)
            if (newPayment.method == ERP::Sales::DTO::PaymentMethod::CASH) {
                newPayment.status = ERP::Sales::DTO::PaymentStatus::COMPLETED;
                if (!paymentDAO_->update(newPayment)) {
                    ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to update payment status to COMPLETED in DAO.");
                    // Not critical enough to rollback entire transaction, just log.
                }
            }
            
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::PaymentCreatedEvent>(newPayment));
            return true;
        },
        "SalesPaymentService", "createPayment"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesPaymentService: Payment " + newPayment.paymentNumber + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Payment", newPayment.id, "Payment", newPayment.paymentNumber,
                       std::nullopt, newPayment.toMap(), "Payment created for invoice: " + newPayment.invoiceId + ".");
        return createdPayment;
    }
    return std::nullopt;
}

std::optional<ERP::Sales::DTO::PaymentDTO> SalesPaymentService::getPaymentById(
    const std::string& paymentId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesPaymentService: Retrieving payment by ID: " + paymentId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewPayments", "Bạn không có quyền xem thanh toán.")) {
        return std::nullopt;
    }

    return paymentDAO_->getById(paymentId); // Using getById from DAOBase template
}

std::optional<ERP::Sales::DTO::PaymentDTO> SalesPaymentService::getPaymentByNumber(
    const std::string& paymentNumber,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesPaymentService: Retrieving payment by number: " + paymentNumber + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewPayments", "Bạn không có quyền xem thanh toán.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["payment_number"] = paymentNumber;
    std::vector<ERP::Sales::DTO::PaymentDTO> payments = paymentDAO_->get(filter); // Using get from DAOBase template
    if (!payments.empty()) {
        return payments[0];
    }
    ERP::Logger::Logger::getInstance().debug("SalesPaymentService: Payment with number " + paymentNumber + " not found.");
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::PaymentDTO> SalesPaymentService::getAllPayments(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesPaymentService: Retrieving all payments with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewPayments", "Bạn không có quyền xem tất cả thanh toán.")) {
        return {};
    }

    return paymentDAO_->get(filter); // Using get from DAOBase template
}

bool SalesPaymentService::updatePayment(
    const ERP::Sales::DTO::PaymentDTO& paymentDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesPaymentService: Attempting to update payment: " + paymentDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdatePayment", "Bạn không có quyền cập nhật thanh toán.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::PaymentDTO> oldPaymentOpt = paymentDAO_->getById(paymentDTO.id); // Using getById from DAOBase
    if (!oldPaymentOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Payment with ID " + paymentDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy thanh toán cần cập nhật.");
        return false;
    }

    // If payment number is changed, check for uniqueness
    if (paymentDTO.paymentNumber != oldPaymentOpt->paymentNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["payment_number"] = paymentDTO.paymentNumber;
        if (paymentDAO_->count(filterByNumber) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("SalesPaymentService: New payment number " + paymentDTO.paymentNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesPaymentService: New payment number " + paymentDTO.paymentNumber + " already exists.", "Số thanh toán mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate Customer existence
    if (paymentDTO.customerId != oldPaymentOpt->customerId) { // Only check if changed
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(paymentDTO.customerId, userRoleIds);
        if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
            ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Invalid Customer ID provided for update or customer is not active: " + paymentDTO.customerId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc khách hàng không hoạt động.");
            return false;
        }
    }

    // Validate Invoice existence and status
    if (paymentDTO.invoiceId != oldPaymentOpt->invoiceId) { // Only check if changed
        std::optional<ERP::Sales::DTO::InvoiceDTO> invoice = invoiceService_->getInvoiceById(paymentDTO.invoiceId, userRoleIds);
        if (!invoice || invoice->status == ERP::Sales::DTO::InvoiceStatus::CANCELLED) {
            ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Invalid Invoice ID provided or invoice is cancelled for update: " + paymentDTO.invoiceId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID hóa đơn không hợp lệ hoặc hóa đơn đã bị hủy.");
            return false;
        }
    }

    ERP::Sales::DTO::PaymentDTO updatedPayment = paymentDTO;
    updatedPayment.updatedAt = ERP::Utils::DateUtils::now();
    updatedPayment.updatedBy = currentUserId;

    // Recalculate AR balance impact if amount changed
    double oldAmount = oldPaymentOpt->amount;
    double newAmount = updatedPayment.amount;
    double balanceChange = newAmount - oldAmount;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!paymentDAO_->update(updatedPayment)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to update payment " + updatedPayment.id + " in DAO.");
                return false;
            }
            
            // Update Invoice's amount_paid and amount_due, and its status
            std::optional<ERP::Sales::DTO::InvoiceDTO> invoice = invoiceService_->getInvoiceById(updatedPayment.invoiceId, userRoleIds); // Re-fetch to get latest status
            if (invoice) {
                ERP::Sales::DTO::InvoiceDTO updatedInvoice = *invoice;
                updatedInvoice.amountPaid = updatedInvoice.amountPaid - oldAmount + newAmount; // Adjust for change
                updatedInvoice.amountDue = updatedInvoice.amountDue + oldAmount - newAmount;
                
                if (updatedInvoice.amountDue <= 0.001) {
                    updatedInvoice.status = ERP::Sales::DTO::InvoiceStatus::PAID;
                } else if (updatedInvoice.amountPaid > 0) { // If some payment exists, but not fully paid
                    updatedInvoice.status = ERP::Sales::DTO::InvoiceStatus::PARTIALLY_PAID;
                } else { // If amountPaid is back to 0
                    updatedInvoice.status = ERP::Sales::DTO::InvoiceStatus::ISSUED;
                }
                updatedInvoice.updatedAt = ERP::Utils::DateUtils::now();
                updatedInvoice.updatedBy = currentUserId;

                if (!invoiceService_->updateInvoice(updatedInvoice, currentUserId, userRoleIds)) {
                     ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to update invoice " + updatedInvoice.invoiceNumber + " during payment update.");
                     // This might indicate a problem, but not fatal to payment update itself, just log
                }
            }

            // Record AR transaction adjustment
            if (balanceChange != 0) {
                ERP::Finance::DTO::AccountReceivableTransactionDTO arAdjustment;
                arAdjustment.id = ERP::Utils::generateUUID();
                arAdjustment.customerId = updatedPayment.customerId;
                arAdjustment.type = (balanceChange > 0) ? ERP::Finance::DTO::ARTransactionType::PAYMENT : ERP::Finance::DTO::ARTransactionType::ADJUSTMENT; // If positive change it's more payment, if negative it's reversal
                arAdjustment.amount = std::abs(balanceChange);
                arAdjustment.currency = updatedPayment.currency;
                arAdjustment.transactionDate = ERP::Utils::DateUtils::now();
                arAdjustment.referenceDocumentId = updatedPayment.id;
                arAdjustment.referenceDocumentType = "PaymentAdjustment";
                arAdjustment.notes = "Payment adjustment for Payment " + updatedPayment.paymentNumber + ": " + std::to_string(balanceChange);
                arAdjustment.createdAt = ERP::Utils::DateUtils::now();
                arAdjustment.createdBy = currentUserId;
                arAdjustment.status = ERP::Common::EntityStatus::ACTIVE;

                if (!securityManager_->getAccountReceivableService()->recordARTransaction(arAdjustment, currentUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to record AR adjustment transaction for payment " + updatedPayment.paymentNumber + ".");
                    // Log error, but proceed
                }
            }
            
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::PaymentUpdatedEvent>(updatedPayment));
            return true;
        },
        "SalesPaymentService", "updatePayment"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesPaymentService: Payment " + updatedPayment.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Payment", updatedPayment.id, "Payment", updatedPayment.paymentNumber,
                       oldPaymentOpt->toMap(), updatedPayment.toMap(), "Payment updated.");
        return true;
    }
    return false;
}

bool SalesPaymentService::updatePaymentStatus(
    const std::string& paymentId,
    ERP::Sales::DTO::PaymentStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesPaymentService: Attempting to update status for payment: " + paymentId + " to " + ERP::Sales::DTO::PaymentDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdatePaymentStatus", "Bạn không có quyền cập nhật trạng thái thanh toán.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::PaymentDTO> paymentOpt = paymentDAO_->getById(paymentId); // Using getById from DAOBase
    if (!paymentOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Payment with ID " + paymentId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy thanh toán để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Sales::DTO::PaymentDTO oldPayment = *paymentOpt;
    if (oldPayment.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("SalesPaymentService: Payment " + paymentId + " is already in status " + oldPayment.getStatusString() + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from REFUNDED to COMPLETED.

    ERP::Sales::DTO::PaymentDTO updatedPayment = oldPayment;
    updatedPayment.status = newStatus;
    updatedPayment.updatedAt = ERP::Utils::DateUtils::now();
    updatedPayment.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!paymentDAO_->update(updatedPayment)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to update status for payment " + paymentId + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::PaymentStatusChangedEvent>(paymentId, newStatus));
            return true;
        },
        "SalesPaymentService", "updatePaymentStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesPaymentService: Status for payment " + paymentId + " updated successfully to " + updatedPayment.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "PaymentStatus", paymentId, "Payment", oldPayment.paymentNumber,
                       oldPayment.toMap(), updatedPayment.toMap(), "Payment status changed to " + updatedPayment.getStatusString() + ".");
        return true;
    }
    return false;
}

bool SalesPaymentService::deletePayment(
    const std::string& paymentId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesPaymentService: Attempting to delete payment: " + paymentId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.DeletePayment", "Bạn không có quyền xóa thanh toán.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::PaymentDTO> paymentOpt = paymentDAO_->getById(paymentId); // Using getById from DAOBase
    if (!paymentOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Payment with ID " + paymentId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy thanh toán cần xóa.");
        return false;
    }

    ERP::Sales::DTO::PaymentDTO paymentToDelete = *paymentOpt;

    // Additional checks: Prevent deletion if payment is linked to a posted invoice or AR transaction.
    if (paymentToDelete.status == ERP::Sales::DTO::PaymentStatus::COMPLETED) { // Assuming completed payments are posted
        ERP::Logger::Logger::getInstance().warning("SalesPaymentService: Cannot delete payment " + paymentId + " as it is completed.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa thanh toán đã hoàn thành.");
        return false;
    }
    // This would also require reversing the AR transaction impact.

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Revert Invoice's amount_paid and amount_due, and its status
            std::optional<ERP::Sales::DTO::InvoiceDTO> invoice = invoiceService_->getInvoiceById(paymentToDelete.invoiceId, userRoleIds);
            if (invoice) {
                ERP::Sales::DTO::InvoiceDTO updatedInvoice = *invoice;
                updatedInvoice.amountPaid -= paymentToDelete.amount;
                updatedInvoice.amountDue += paymentToDelete.amount;
                
                if (updatedInvoice.amountPaid <= 0.001) { // If no payment remaining
                    updatedInvoice.status = ERP::Sales::DTO::InvoiceStatus::ISSUED;
                } else {
                    updatedInvoice.status = ERP::Sales::DTO::InvoiceStatus::PARTIALLY_PAID;
                }
                updatedInvoice.updatedAt = ERP::Utils::DateUtils::now();
                updatedInvoice.updatedBy = currentUserId;

                if (!invoiceService_->updateInvoice(updatedInvoice, currentUserId, userRoleIds)) {
                     ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to revert invoice " + updatedInvoice.invoiceNumber + " during payment deletion.");
                     // Log error, but proceed
                }
            }

            // Reverse AR transaction
            ERP::Finance::DTO::AccountReceivableTransactionDTO arReversal;
            arReversal.id = ERP::Utils::generateUUID();
            arReversal.customerId = paymentToDelete.customerId;
            arReversal.type = ERP::Finance::DTO::ARTransactionType::ADJUSTMENT; // Reversal as an adjustment
            arReversal.amount = -paymentToDelete.amount; // Negative amount to reverse
            arReversal.currency = paymentToDelete.currency;
            arReversal.transactionDate = ERP::Utils::DateUtils::now();
            arReversal.referenceDocumentId = paymentToDelete.id;
            arReversal.referenceDocumentType = "PaymentReversal";
            arReversal.notes = "Payment reversal for Payment " + paymentToDelete.paymentNumber + " during deletion.";
            arReversal.createdAt = ERP::Utils::DateUtils::now();
            arReversal.createdBy = currentUserId;
            arReversal.status = ERP::Common::EntityStatus::ACTIVE;

            if (!securityManager_->getAccountReceivableService()->recordARTransaction(arReversal, currentUserId, userRoleIds)) {
                ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to record AR reversal transaction for payment " + paymentToDelete.paymentNumber + ".");
                return false;
            }

            if (!paymentDAO_->remove(paymentId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesPaymentService: Failed to delete payment " + paymentId + " in DAO.");
                return false;
            }
            return true;
        },
        "SalesPaymentService", "deletePayment"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesPaymentService: Payment " + paymentId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Payment", paymentId, "Payment", paymentToDelete.paymentNumber,
                       paymentToDelete.toMap(), std::nullopt, "Payment deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Sales
} // namespace ERP