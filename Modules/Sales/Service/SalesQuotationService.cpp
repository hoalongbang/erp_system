// Modules/Sales/Service/SalesQuotationService.cpp
#include "SalesQuotationService.h" // Đã rút gọn include
#include "Quotation.h" // Đã rút gọn include
#include "QuotationDetail.h" // Đã rút gọn include
#include "SalesOrder.h" // For sales order conversion
#include "SalesOrderDetail.h" // For sales order conversion
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "CustomerService.h" // Đã rút gọn include
#include "ProductService.h" // Đã rút gọn include
#include "UnitOfMeasureService.h" // Đã rút gọn include
#include "SalesOrderService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Sales {
namespace Services {

SalesQuotationService::SalesQuotationService(
    std::shared_ptr<DAOs::QuotationDAO> quotationDAO,
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> unitOfMeasureService,
    std::shared_ptr<ISalesOrderService> salesOrderService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      quotationDAO_(quotationDAO), customerService_(customerService),
      productService_(productService), unitOfMeasureService_(unitOfMeasureService),
      salesOrderService_(salesOrderService) {
    if (!quotationDAO_ || !customerService_ || !productService_ || !unitOfMeasureService_ || !salesOrderService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "SalesQuotationService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ báo giá bán hàng.");
        ERP::Logger::Logger::getInstance().critical("SalesQuotationService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("SalesQuotationService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("SalesQuotationService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Sales::DTO::QuotationDTO> SalesQuotationService::createQuotation(
    const ERP::Sales::DTO::QuotationDTO& quotationDTO,
    const std::vector<ERP::Sales::DTO::QuotationDetailDTO>& quotationDetails,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesQuotationService: Attempting to create quotation: " + quotationDTO.quotationNumber + " for customer: " + quotationDTO.customerId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.CreateQuotation", "Bạn không có quyền tạo báo giá.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (quotationDTO.quotationNumber.empty() || quotationDTO.customerId.empty() || quotationDetails.empty()) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Invalid input for quotation creation (empty number, customerId, or no details).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesQuotationService: Invalid input for quotation creation.", "Thông tin báo giá không đầy đủ.");
        return std::nullopt;
    }

    // Check if quotation number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["quotation_number"] = quotationDTO.quotationNumber;
    if (quotationDAO_->count(filterByNumber) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Quotation with number " + quotationDTO.quotationNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesQuotationService: Quotation with number " + quotationDTO.quotationNumber + " already exists.", "Số báo giá đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate Customer existence
    std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(quotationDTO.customerId, userRoleIds);
    if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Invalid Customer ID provided or customer is not active: " + quotationDTO.customerId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc khách hàng không hoạt động.");
        return std::nullopt;
    }

    // Validate details
    for (const auto& detail : quotationDetails) {
        if (!productService_->getProductById(detail.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Detail product " + detail.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        if (detail.quantity <= 0) {
            ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Detail product " + detail.productId + " has non-positive quantity.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng trong chi tiết báo giá phải lớn hơn 0.");
            return std::nullopt;
        }
        // Unit Price / Discount / TaxRate validation can be added here
    }

    ERP::Sales::DTO::QuotationDTO newQuotation = quotationDTO;
    newQuotation.id = ERP::Utils::generateUUID();
    newQuotation.createdAt = ERP::Utils::DateUtils::now();
    newQuotation.createdBy = currentUserId;
    newQuotation.status = ERP::Sales::DTO::QuotationStatus::DRAFT; // Default status

    std::optional<ERP::Sales::DTO::QuotationDTO> createdQuotation = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!quotationDAO_->create(newQuotation)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to create quotation " + newQuotation.quotationNumber + " in DAO.");
                return false;
            }
            // Create Quotation Details
            for (auto detail : quotationDetails) {
                detail.id = ERP::Utils::generateUUID();
                detail.quotationId = newQuotation.id;
                detail.createdAt = newQuotation.createdAt;
                detail.createdBy = newQuotation.createdBy;
                detail.status = ERP::Common::EntityStatus::ACTIVE; // Assuming detail has status
                
                if (!quotationDAO_->createQuotationDetail(detail)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to create quotation detail for product " + detail.productId + ".");
                    return false;
                }
            }
            createdQuotation = newQuotation;
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::QuotationCreatedEvent>(newQuotation));
            return true;
        },
        "SalesQuotationService", "createQuotation"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesQuotationService: Quotation " + newQuotation.quotationNumber + " created successfully with " + std::to_string(quotationDetails.size()) + " details.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Quotation", newQuotation.id, "Quotation", newQuotation.quotationNumber,
                       std::nullopt, newQuotation.toMap(), "Quotation created.");
        return createdQuotation;
    }
    return std::nullopt;
}

std::optional<ERP::Sales::DTO::QuotationDTO> SalesQuotationService::getQuotationById(
    const std::string& quotationId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesQuotationService: Retrieving quotation by ID: " + quotationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewQuotations", "Bạn không có quyền xem báo giá.")) {
        return std::nullopt;
    }

    return quotationDAO_->getById(quotationId); // Using getById from DAOBase template
}

std::optional<ERP::Sales::DTO::QuotationDTO> SalesQuotationService::getQuotationByNumber(
    const std::string& quotationNumber,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesQuotationService: Retrieving quotation by number: " + quotationNumber + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewQuotations", "Bạn không có quyền xem báo giá.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["quotation_number"] = quotationNumber;
    std::vector<ERP::Sales::DTO::QuotationDTO> quotations = quotationDAO_->get(filter); // Using get from DAOBase template
    if (!quotations.empty()) {
        return quotations[0];
    }
    ERP::Logger::Logger::getInstance().debug("SalesQuotationService: Quotation with number " + quotationNumber + " not found.");
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::QuotationDTO> SalesQuotationService::getAllQuotations(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesQuotationService: Retrieving all quotations with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewQuotations", "Bạn không có quyền xem tất cả báo giá.")) {
        return {};
    }

    return quotationDAO_->get(filter); // Using get from DAOBase template
}

bool SalesQuotationService::updateQuotation(
    const ERP::Sales::DTO::QuotationDTO& quotationDTO,
    const std::vector<ERP::Sales::DTO::QuotationDetailDTO>& quotationDetails,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesQuotationService: Attempting to update quotation: " + quotationDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateQuotation", "Bạn không có quyền cập nhật báo giá.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::QuotationDTO> oldQuotationOpt = quotationDAO_->getById(quotationDTO.id); // Using getById from DAOBase
    if (!oldQuotationOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Quotation with ID " + quotationDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy báo giá cần cập nhật.");
        return false;
    }

    // If quotation number is changed, check for uniqueness
    if (quotationDTO.quotationNumber != oldQuotationOpt->quotationNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["quotation_number"] = quotationDTO.quotationNumber;
        if (quotationDAO_->count(filterByNumber) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("SalesQuotationService: New quotation number " + quotationDTO.quotationNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesQuotationService: New quotation number " + quotationDTO.quotationNumber + " already exists.", "Số báo giá mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate Customer existence
    if (quotationDTO.customerId != oldQuotationOpt->customerId) { // Only check if changed
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(quotationDTO.customerId, userRoleIds);
        if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
            ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Invalid Customer ID provided for update or customer is not active: " + quotationDTO.customerId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc khách hàng không hoạt động.");
            return false;
        }
    }

    // Validate details (similar to create)
    for (const auto& detail : quotationDetails) {
        if (detail.productId.empty() || detail.quantity <= 0) {
             ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Invalid detail input for product " + detail.productId);
             ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin chi tiết báo giá không đầy đủ.");
             return false;
        }
        if (!productService_->getProductById(detail.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Detail product " + detail.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm trong chi tiết không tồn tại.");
            return std::nullopt;
        }
    }

    ERP::Sales::DTO::QuotationDTO updatedQuotation = quotationDTO;
    updatedQuotation.updatedAt = ERP::Utils::DateUtils::now();
    updatedQuotation.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!quotationDAO_->update(updatedQuotation)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to update quotation " + updatedQuotation.id + " in DAO.");
                return false;
            }
            // Update details (remove all old, add new ones - simpler but less efficient for large lists)
            if (!quotationDAO_->removeQuotationDetailsByQuotationId(updatedQuotation.id)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to remove old quotation details for quotation " + updatedQuotation.id + ".");
                return false;
            }
            for (auto detail : quotationDetails) {
                detail.id = ERP::Utils::generateUUID(); // New UUID for new details
                detail.quotationId = updatedQuotation.id;
                detail.createdAt = updatedQuotation.createdAt; // Use parent creation info
                detail.createdBy = updatedQuotation.createdBy;
                detail.status = ERP::Common::EntityStatus::ACTIVE; // Assuming detail has status
                
                if (!quotationDAO_->createQuotationDetail(detail)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to create new quotation detail for product " + detail.productId + " for quotation " + updatedQuotation.id + ".");
                    return false;
                }
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::QuotationUpdatedEvent>(updatedQuotation));
            return true;
        },
        "SalesQuotationService", "updateQuotation"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesQuotationService: Quotation " + updatedQuotation.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Quotation", updatedQuotation.id, "Quotation", updatedQuotation.quotationNumber,
                       oldQuotationOpt->toMap(), updatedQuotation.toMap(), "Quotation updated.");
        return true;
    }
    return false;
}

bool SalesQuotationService::updateQuotationStatus(
    const std::string& quotationId,
    ERP::Sales::DTO::QuotationStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesQuotationService: Attempting to update status for quotation: " + quotationId + " to " + ERP::Sales::DTO::QuotationDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateQuotationStatus", "Bạn không có quyền cập nhật trạng thái báo giá.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::QuotationDTO> quotationOpt = quotationDAO_->getById(quotationId); // Using getById from DAOBase
    if (!quotationOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Quotation with ID " + quotationId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy báo giá để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Sales::DTO::QuotationDTO oldQuotation = *quotationOpt;
    if (oldQuotation.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("SalesQuotationService: Quotation " + quotationId + " is already in status " + oldQuotation.getStatusString() + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from ACCEPTED to DRAFT.

    ERP::Sales::DTO::QuotationDTO updatedQuotation = oldQuotation;
    updatedQuotation.status = newStatus;
    updatedQuotation.updatedAt = ERP::Utils::DateUtils::now();
    updatedQuotation.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!quotationDAO_->update(updatedQuotation)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to update status for quotation " + quotationId + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::QuotationStatusChangedEvent>(quotationId, newStatus));
            return true;
        },
        "SalesQuotationService", "updateQuotationStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesQuotationService: Status for quotation " + quotationId + " updated successfully to " + updatedQuotation.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "QuotationStatus", quotationId, "Quotation", oldQuotation.quotationNumber,
                       oldQuotation.toMap(), updatedQuotation.toMap(), "Quotation status changed to " + updatedQuotation.getStatusString() + ".");
        return true;
    }
    return false;
}

bool SalesQuotationService::deleteQuotation(
    const std::string& quotationId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesQuotationService: Attempting to delete quotation: " + quotationId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.DeleteQuotation", "Bạn không có quyền xóa báo giá.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::QuotationDTO> quotationOpt = quotationDAO_->getById(quotationId); // Using getById from DAOBase
    if (!quotationOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Quotation with ID " + quotationId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy báo giá cần xóa.");
        return false;
    }

    ERP::Sales::DTO::QuotationDTO quotationToDelete = *quotationOpt;

    // Additional checks: Prevent deletion if quotation is ACCEPTED or converted to Sales Order
    if (quotationToDelete.status == ERP::Sales::DTO::QuotationStatus::ACCEPTED) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Cannot delete quotation " + quotationId + " as it is accepted.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa báo giá đã được chấp nhận.");
        return false;
    }
    // Check if any SalesOrder has this quotationId as reference
    std::map<std::string, std::any> salesOrderFilter;
    salesOrderFilter["quotation_id"] = quotationId;
    if (securityManager_->getSalesOrderService()->getAllSalesOrders(salesOrderFilter).size() > 0) { // Assuming SalesOrderService can query by quotationId
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Cannot delete quotation " + quotationId + " as it has associated sales orders.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa báo giá có đơn hàng bán liên quan.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated details first
            if (!quotationDAO_->removeQuotationDetailsByQuotationId(quotationId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to remove associated quotation details for quotation " + quotationId + ".");
                return false;
            }
            if (!quotationDAO_->remove(quotationId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to delete quotation " + quotationId + " in DAO.");
                return false;
            }
            return true;
        },
        "SalesQuotationService", "deleteQuotation"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesQuotationService: Quotation " + quotationId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Quotation", quotationId, "Quotation", quotationToDelete.quotationNumber,
                       quotationToDelete.toMap(), std::nullopt, "Quotation deleted.");
        return true;
    }
    return false;
}

std::optional<ERP::Sales::DTO::SalesOrderDTO> SalesQuotationService::convertQuotationToSalesOrder(
    const std::string& quotationId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesQuotationService: Attempting to convert quotation " + quotationId + " to sales order by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ConvertQuotationToSalesOrder", "Bạn không có quyền chuyển đổi báo giá thành đơn hàng bán.")) {
        return std::nullopt;
    }

    std::optional<ERP::Sales::DTO::QuotationDTO> quotationOpt = quotationDAO_->getById(quotationId); // Using getById from DAOBase
    if (!quotationOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Quotation with ID " + quotationId + " not found for conversion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy báo giá để chuyển đổi.");
        return std::nullopt;
    }
    
    ERP::Sales::DTO::QuotationDTO quotation = *quotationOpt;

    if (quotation.status != ERP::Sales::DTO::QuotationStatus::ACCEPTED) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Cannot convert quotation " + quotationId + " to sales order as its status is " + quotation.getStatusString() + " (must be ACCEPTED).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Chỉ có thể chuyển đổi báo giá đã được chấp nhận thành đơn hàng bán.");
        return std::nullopt;
    }

    // Retrieve quotation details
    std::vector<ERP::Sales::DTO::QuotationDetailDTO> quotationDetails = quotationDAO_->getQuotationDetailsByQuotationId(quotationId); // Specific DAO method
    if (quotationDetails.empty()) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Quotation " + quotationId + " has no details. Cannot convert to sales order.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Báo giá không có chi tiết. Không thể chuyển đổi thành đơn hàng bán.");
        return std::nullopt;
    }

    // Create a new SalesOrderDTO from QuotationDTO
    ERP::Sales::DTO::SalesOrderDTO newSalesOrder;
    newSalesOrder.orderNumber = "SO-" + ERP::Utils::generateUUID().substr(0, 8); // Generate new unique order number
    newSalesOrder.customerId = quotation.customerId;
    newSalesOrder.requestedByUserId = currentUserId;
    newSalesOrder.orderDate = ERP::Utils::DateUtils::now();
    newSalesOrder.requiredDeliveryDate = quotation.validUntilDate; // Use quotation valid until date as suggested delivery
    newSalesOrder.status = ERP::Sales::DTO::SalesOrderStatus::DRAFT; // Initial draft status
    newSalesOrder.totalAmount = quotation.totalAmount;
    newSalesOrder.totalDiscount = quotation.totalDiscount;
    newSalesOrder.totalTax = quotation.totalTax;
    newSalesOrder.netAmount = quotation.netAmount;
    newSalesOrder.amountPaid = 0.0; // No payment yet for new sales order
    newSalesOrder.amountDue = quotation.netAmount;
    newSalesOrder.currency = quotation.currency;
    newSalesOrder.paymentTerms = quotation.paymentTerms;
    newSalesOrder.deliveryAddress = quotation.notes; // Using notes for delivery address, assuming it might contain it
    newSalesOrder.notes = "Converted from Quotation " + quotation.quotationNumber;
    newSalesOrder.warehouseId = "default_warehouse_id"; // Placeholder, needs actual logic to determine default warehouse
    newSalesOrder.quotationId = quotation.id; // Link back to original quotation
    newSalesOrder.createdAt = ERP::Utils::DateUtils::now();
    newSalesOrder.createdBy = currentUserId;

    // Convert QuotationDetails to SalesOrderDetails
    std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> salesOrderDetails;
    for (const auto& qDetail : quotationDetails) {
        ERP::Sales::DTO::SalesOrderDetailDTO soDetail;
        soDetail.productId = qDetail.productId;
        soDetail.quantity = qDetail.quantity;
        soDetail.unitPrice = qDetail.unitPrice;
        soDetail.discount = qDetail.discount;
        soDetail.discountType = qDetail.discountType;
        soDetail.taxRate = qDetail.taxRate;
        soDetail.lineTotal = qDetail.lineTotal;
        soDetail.deliveredQuantity = 0.0;
        soDetail.invoicedQuantity = 0.0;
        soDetail.isFullyDelivered = false;
        soDetail.isFullyInvoiced = false;
        soDetail.notes = qDetail.notes;
        soDetail.status = ERP::Common::EntityStatus::ACTIVE;
        soDetail.createdAt = newSalesOrder.createdAt;
        soDetail.createdBy = newSalesOrder.createdBy;
        salesOrderDetails.push_back(soDetail);
    }

    std::optional<ERP::Sales::DTO::SalesOrderDTO> createdSalesOrder = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Create the new Sales Order
            createdSalesOrder = salesOrderService_->createSalesOrder(newSalesOrder, currentUserId, userRoleIds);
            if (!createdSalesOrder) {
                ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to create new Sales Order from quotation " + quotationId + ".");
                return false;
            }
            // Add details to the new Sales Order
            for (auto soDetail : salesOrderDetails) {
                soDetail.salesOrderId = createdSalesOrder->id;
                if (!securityManager_->getSalesOrderService()->createSalesOrderDetail(soDetail, currentUserId, userRoleIds)) { // Assuming createSalesOrderDetail exists and handles its own logic
                    ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to create sales order detail for product " + soDetail.productId + " for new sales order " + createdSalesOrder->id + ".");
                    return false;
                }
            }
            // Update the original quotation's status to indicate conversion
            ERP::Sales::DTO::QuotationDTO updatedQuotation = quotation;
            updatedQuotation.status = ERP::Sales::DTO::QuotationStatus::COMPLETED; // Mark as completed/converted
            updatedQuotation.updatedAt = ERP::Utils::DateUtils::now();
            updatedQuotation.updatedBy = currentUserId;
            if (!quotationDAO_->update(updatedQuotation)) { // Using update from DAOBase
                ERP::Logger::Logger::getInstance().error("SalesQuotationService: Failed to update status of quotation " + quotationId + " after conversion.");
                return false;
            }
            return true;
        },
        "SalesQuotationService", "convertQuotationToSalesOrder"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesQuotationService: Quotation " + quotationId + " successfully converted to Sales Order " + createdSalesOrder->id + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::PROCESS_END, ERP::Common::LogSeverity::INFO,
                       "Sales", "QuotationConversion", quotationId, "Quotation", quotation.quotationNumber,
                       quotation.toMap(), createdSalesOrder->toMap(), "Quotation converted to Sales Order: " + createdSalesOrder->orderNumber + ".");
        return createdSalesOrder;
    }
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::QuotationDetailDTO> SalesQuotationService::getQuotationDetails(
    const std::string& quotationId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesQuotationService: Retrieving quotation details for quotation ID: " + quotationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewQuotations", "Bạn không có quyền xem chi tiết báo giá.")) {
        return {};
    }

    // Check if parent Quotation exists
    if (!quotationDAO_->getById(quotationId)) {
        ERP::Logger::Logger::getInstance().warning("SalesQuotationService: Parent Quotation " + quotationId + " not found when getting details.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Báo giá cha không tồn tại.");
        return {};
    }

    return quotationDAO_->getQuotationDetailsByQuotationId(quotationId); // Specific DAO method
}

} // namespace Services
} // namespace Sales
} // namespace ERP