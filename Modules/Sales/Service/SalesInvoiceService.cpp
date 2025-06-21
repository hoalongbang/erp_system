// Modules/Sales/Service/SalesInvoiceService.cpp
#include "SalesInvoiceService.h" // Đã rút gọn include
#include "Invoice.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "SalesOrderService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
// #include "DTOUtils.h" // No longer needed here for QJsonObject conversions

// Removed Qt includes as they are no longer needed here
// #include <QJsonObject>
// #include <QJsonDocument>

namespace ERP {
namespace Sales {
namespace Services {

SalesInvoiceService::SalesInvoiceService(
    std::shared_ptr<DAOs::InvoiceDAO> invoiceDAO,
    std::shared_ptr<ISalesOrderService> salesOrderService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      invoiceDAO_(invoiceDAO), salesOrderService_(salesOrderService) {
    if (!invoiceDAO_ || !salesOrderService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "SalesInvoiceService: Initialized with null DAO or SalesOrderService.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ hóa đơn bán hàng.");
        ERP::Logger::Logger::getInstance().critical("SalesInvoiceService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("SalesInvoiceService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Initialized.");
}

// Old checkUserPermission and getUserName/getCurrentSessionId/logAudit removed as they are now in BaseService

std::optional<ERP::Sales::DTO::InvoiceDTO> SalesInvoiceService::createInvoice(
    const ERP::Sales::DTO::InvoiceDTO& invoiceDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Attempting to create invoice: " + invoiceDTO.invoiceNumber + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.CreateInvoice", "Bạn không có quyền tạo hóa đơn.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (invoiceDTO.invoiceNumber.empty() || invoiceDTO.customerId.empty() || invoiceDTO.salesOrderId.empty()) {
        ERP::Logger::Logger::getInstance().warning("SalesInvoiceService: Invalid input for invoice creation (empty number, customerId, or salesOrderId).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesInvoiceService: Invalid input for invoice creation.", "Thông tin hóa đơn không đầy đủ.");
        return std::nullopt;
    }

    // Check if invoice number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["invoice_number"] = invoiceDTO.invoiceNumber;
    if (invoiceDAO_->count(filterByNumber) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("SalesInvoiceService: Invoice with number " + invoiceDTO.invoiceNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesInvoiceService: Invoice with number " + invoiceDTO.invoiceNumber + " already exists.", "Số hóa đơn đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate Sales Order existence (assuming SalesOrderService handles permission internally)
    std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrder = salesOrderService_->getSalesOrderById(invoiceDTO.salesOrderId, userRoleIds);
    if (!salesOrder || salesOrder->status == ERP::Sales::DTO::SalesOrderStatus::CANCELLED || salesOrder->status == ERP::Sales::DTO::SalesOrderStatus::REJECTED) {
        ERP::Logger::Logger::getInstance().warning("SalesInvoiceService: Invalid Sales Order ID provided or sales order is not valid for invoicing: " + invoiceDTO.salesOrderId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID đơn hàng bán không hợp lệ hoặc đơn hàng không còn hiệu lực để lập hóa đơn.");
        return std::nullopt;
    }

    ERP::Sales::DTO::InvoiceDTO newInvoice = invoiceDTO;
    newInvoice.id = ERP::Utils::generateUUID();
    newInvoice.createdAt = ERP::Utils::DateUtils::now();
    newInvoice.createdBy = currentUserId;
    newInvoice.status = ERP::Sales::DTO::InvoiceStatus::DRAFT; // Default status

    std::optional<ERP::Sales::DTO::InvoiceDTO> createdInvoice = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Note: DAO methods already acquire/release connections internally now.
            // The transaction context ensures atomicity across multiple DAO calls if needed.
            if (!invoiceDAO_->create(newInvoice)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesInvoiceService: Failed to create invoice " + newInvoice.invoiceNumber + " in DAO.");
                return false;
            }
            createdInvoice = newInvoice;
            // Optionally, update sales order status to PARTIALLY_INVOICED or INVOICED
            // salesOrderService_->updateSalesOrderStatus(salesOrder->id, ERP::Sales::DTO::SalesOrderStatus::IN_PROGRESS, currentUserId, userRoleIds);
            return true;
        },
        "SalesInvoiceService", "createInvoice"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Invoice " + newInvoice.invoiceNumber + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Invoice", newInvoice.id, "Invoice", newInvoice.invoiceNumber,
                       std::nullopt, newInvoice.toMap(), "Invoice created."); // newInvoice.toMap() is for afterData
        return createdInvoice;
    }
    return std::nullopt;
}

std::optional<ERP::Sales::DTO::InvoiceDTO> SalesInvoiceService::getInvoiceById(
    const std::string& invoiceId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesInvoiceService: Retrieving invoice by ID: " + invoiceId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewInvoices", "Bạn không có quyền xem hóa đơn.")) {
        return std::nullopt;
    }

    return invoiceDAO_->getById(invoiceId); // Using getById from DAOBase template
}

std::optional<ERP::Sales::DTO::InvoiceDTO> SalesInvoiceService::getInvoiceByNumber(
    const std::string& invoiceNumber,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesInvoiceService: Retrieving invoice by number: " + invoiceNumber + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewInvoices", "Bạn không có quyền xem hóa đơn.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["invoice_number"] = invoiceNumber;
    std::vector<ERP::Sales::DTO::InvoiceDTO> invoices = invoiceDAO_->get(filter); // Using get from DAOBase template
    if (!invoices.empty()) {
        return invoices[0];
    }
    ERP::Logger::Logger::getInstance().debug("SalesInvoiceService: Invoice with number " + invoiceNumber + " not found.");
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::InvoiceDTO> SalesInvoiceService::getAllInvoices(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Retrieving all invoices with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewInvoices", "Bạn không có quyền xem tất cả hóa đơn.")) {
        return {};
    }

    return invoiceDAO_->get(filter); // Using get from DAOBase template
}

bool SalesInvoiceService::updateInvoice(
    const ERP::Sales::DTO::InvoiceDTO& invoiceDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Attempting to update invoice: " + invoiceDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateInvoice", "Bạn không có quyền cập nhật hóa đơn.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::InvoiceDTO> oldInvoiceOpt = invoiceDAO_->getById(invoiceDTO.id); // Using getById from DAOBase
    if (!oldInvoiceOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesInvoiceService: Invoice with ID " + invoiceDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy hóa đơn cần cập nhật.");
        return false;
    }

    // If invoice number is changed, check for uniqueness
    if (invoiceDTO.invoiceNumber != oldInvoiceOpt->invoiceNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["invoice_number"] = invoiceDTO.invoiceNumber;
        if (invoiceDAO_->count(filterByNumber) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("SalesInvoiceService: New invoice number " + invoiceDTO.invoiceNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesInvoiceService: New invoice number " + invoiceDTO.invoiceNumber + " already exists.", "Số hóa đơn mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate Sales Order existence (assuming SalesOrderService handles permission internally)
    std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrder = salesOrderService_->getSalesOrderById(invoiceDTO.salesOrderId, userRoleIds);
    if (!salesOrder || salesOrder->status == ERP::Sales::DTO::SalesOrderStatus::CANCELLED || salesOrder->status == ERP::Sales::DTO::SalesOrderStatus::REJECTED) {
        ERP::Logger::Logger::getInstance().warning("SalesInvoiceService: Invalid Sales Order ID provided or sales order is not valid for update: " + invoiceDTO.salesOrderId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID đơn hàng bán không hợp lệ hoặc đơn hàng không còn hiệu lực.");
        return false;
    }

    ERP::Sales::DTO::InvoiceDTO updatedInvoice = invoiceDTO;
    updatedInvoice.updatedAt = ERP::Utils::DateUtils::now();
    updatedInvoice.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!invoiceDAO_->update(updatedInvoice)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesInvoiceService: Failed to update invoice " + updatedInvoice.id + " in DAO.");
                return false;
            }
            return true;
        },
        "SalesInvoiceService", "updateInvoice"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Invoice " + updatedInvoice.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Invoice", updatedInvoice.id, "Invoice", updatedInvoice.invoiceNumber,
                       oldInvoiceOpt->toMap(), updatedInvoice.toMap(), "Invoice updated.");
        return true;
    }
    return false;
}

bool SalesInvoiceService::updateInvoiceStatus(
    const std::string& invoiceId,
    ERP::Sales::DTO::InvoiceStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Attempting to update status for invoice: " + invoiceId + " to " + ERP::Sales::DTO::InvoiceDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateInvoiceStatus", "Bạn không có quyền cập nhật trạng thái hóa đơn.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::InvoiceDTO> invoiceOpt = invoiceDAO_->getById(invoiceId); // Using getById from DAOBase
    if (!invoiceOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesInvoiceService: Invoice with ID " + invoiceId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy hóa đơn để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Sales::DTO::InvoiceDTO oldInvoice = *invoiceOpt;
    if (oldInvoice.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Invoice " + invoiceId + " is already in status " + ERP::Sales::DTO::InvoiceDTO().getStatusString(newStatus) + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from COMPLETED to IN_PROGRESS.

    ERP::Sales::DTO::InvoiceDTO updatedInvoice = oldInvoice;
    updatedInvoice.status = newStatus;
    updatedInvoice.updatedAt = ERP::Utils::DateUtils::now();
    updatedInvoice.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!invoiceDAO_->update(updatedInvoice)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesInvoiceService: Failed to update status for invoice " + invoiceId + " in DAO.");
                return false;
            }
            // Optionally, publish event for status change
            // eventBus_.publish(std::make_shared<EventBus::InvoiceStatusChangedEvent>(invoiceId, newStatus));
            return true;
        },
        "SalesInvoiceService", "updateInvoiceStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Status for invoice " + invoiceId + " updated successfully to " + updatedInvoice.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "InvoiceStatus", invoiceId, "Invoice", oldInvoice.invoiceNumber,
                       oldInvoice.toMap(), updatedInvoice.toMap(), "Invoice status changed to " + updatedInvoice.getStatusString() + ".");
        return true;
    }
    return false;
}

bool SalesInvoiceService::deleteInvoice(
    const std::string& invoiceId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Attempting to delete invoice: " + invoiceId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.DeleteInvoice", "Bạn không có quyền xóa hóa đơn.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::InvoiceDTO> invoiceOpt = invoiceDAO_->getById(invoiceId); // Using getById from DAOBase
    if (!invoiceOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesInvoiceService: Invoice with ID " + invoiceId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy hóa đơn cần xóa.");
        return false;
    }

    ERP::Sales::DTO::InvoiceDTO invoiceToDelete = *invoiceOpt;

    // Additional checks: Prevent deletion if there are associated payments or if the invoice is linked to financial records that disallow deletion.
    // (This would require dependencies on PaymentService, GLService, etc.)

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated details first
            if (!invoiceDAO_->removeInvoiceDetailsByInvoiceId(invoiceId)) { // Specific DAO method for details
                ERP::Logger::Logger::getInstance().error("SalesInvoiceService: Failed to remove associated invoice details for invoice " + invoiceId + ".");
                return false;
            }
            if (!invoiceDAO_->remove(invoiceId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesInvoiceService: Failed to delete invoice " + invoiceId + " in DAO.");
                return false;
            }
            return true;
        },
        "SalesInvoiceService", "deleteInvoice"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesInvoiceService: Invoice " + invoiceId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Invoice", invoiceId, "Invoice", invoiceToDelete.invoiceNumber,
                       invoiceToDelete.toMap(), std::nullopt, "Invoice deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Sales
} // namespace ERP