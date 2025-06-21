// Modules/Sales/Service/SalesOrderService.cpp
#include "SalesOrderService.h" // Đã rút gọn include
#include "SalesOrder.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "CustomerService.h" // Đã rút gọn include
#include "WarehouseService.h" // Đã rút gọn include
#include "ProductService.h // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
// #include "DTOUtils.h" // No longer needed here for QJsonObject conversions

namespace ERP {
namespace Sales {
namespace Services {

SalesOrderService::SalesOrderService(
    std::shared_ptr<DAOs::SalesOrderDAO> salesOrderDAO,
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      salesOrderDAO_(salesOrderDAO), customerService_(customerService),
      warehouseService_(warehouseService), productService_(productService) {
    if (!salesOrderDAO_ || !customerService_ || !warehouseService_ || !productService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "SalesOrderService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ đơn hàng bán.");
        ERP::Logger::Logger::getInstance().critical("SalesOrderService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("SalesOrderService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("SalesOrderService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Sales::DTO::SalesOrderDTO> SalesOrderService::createSalesOrder(
    const ERP::Sales::DTO::SalesOrderDTO& salesOrderDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesOrderService: Attempting to create sales order: " + salesOrderDTO.orderNumber + " for product: " + salesOrderDTO.productId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.CreateSalesOrder", "Bạn không có quyền tạo đơn hàng bán.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (salesOrderDTO.orderNumber.empty() || salesOrderDTO.customerId.empty() || salesOrderDTO.warehouseId.empty()) {
        ERP::Logger::Logger::getInstance().warning("SalesOrderService: Invalid input for sales order creation (empty number, customerId, or warehouseId).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesOrderService: Invalid input for sales order creation.", "Thông tin đơn hàng bán không đầy đủ.");
        return std::nullopt;
    }

    // Check if order number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["order_number"] = salesOrderDTO.orderNumber;
    if (salesOrderDAO_->count(filterByNumber) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("SalesOrderService: Sales order with number " + salesOrderDTO.orderNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesOrderService: Sales order with number " + salesOrderDTO.orderNumber + " already exists.", "Số đơn hàng bán đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate Customer existence (assuming CustomerService handles permission internally)
    std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(salesOrderDTO.customerId, userRoleIds);
    if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("SalesOrderService: Invalid Customer ID provided or customer is not active: " + salesOrderDTO.customerId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc khách hàng không hoạt động.");
        return std::nullopt;
    }

    // Validate Warehouse existence (assuming WarehouseService handles permission internally)
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(salesOrderDTO.warehouseId, userRoleIds); // userRoleIds is passed here
    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("SalesOrderService: Invalid Warehouse ID provided or warehouse is not active: " + salesOrderDTO.warehouseId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
        return std::nullopt;
    }

    ERP::Sales::DTO::SalesOrderDTO newSalesOrder = salesOrderDTO;
    newSalesOrder.id = ERP::Utils::generateUUID();
    newSalesOrder.createdAt = ERP::Utils::DateUtils::now();
    newSalesOrder.createdBy = currentUserId;
    newSalesOrder.status = ERP::Sales::DTO::SalesOrderStatus::DRAFT; // Default status

    std::optional<ERP::Sales::DTO::SalesOrderDTO> createdSalesOrder = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Note: DAO methods already acquire/release connections internally now.
            // The transaction context ensures atomicity across multiple DAO calls if needed.
            if (!salesOrderDAO_->create(newSalesOrder)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesOrderService: Failed to create sales order " + newSalesOrder.orderNumber + " in DAO.");
                return false;
            }
            createdSalesOrder = newSalesOrder;
            // Optionally, save sales order details here if they are part of the initial creation
            return true;
        },
        "SalesOrderService", "createSalesOrder"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesOrderService: Sales order " + newSalesOrder.orderNumber + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "SalesOrder", newSalesOrder.id, "SalesOrder", newSalesOrder.orderNumber,
                       std::nullopt, newSalesOrder.toMap(), "Sales order created.");
        return createdSalesOrder;
    }
    return std::nullopt;
}

std::optional<ERP::Sales::DTO::SalesOrderDTO> SalesOrderService::getSalesOrderById(
    const std::string& salesOrderId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesOrderService: Retrieving sales order by ID: " + salesOrderId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewSalesOrders", "Bạn không có quyền xem đơn hàng bán.")) {
        return std::nullopt;
    }

    return salesOrderDAO_->getById(salesOrderId); // Using getById from DAOBase template
}

std::optional<ERP::Sales::DTO::SalesOrderDTO> SalesOrderService::getSalesOrderByNumber(
    const std::string& orderNumber,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesOrderService: Retrieving sales order by number: " + orderNumber + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewSalesOrders", "Bạn không có quyền xem đơn hàng bán.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["order_number"] = orderNumber;
    std::vector<ERP::Sales::DTO::SalesOrderDTO> salesOrders = salesOrderDAO_->get(filter); // Using get from DAOBase template
    if (!salesOrders.empty()) {
        return salesOrders[0];
    }
    ERP::Logger::Logger::getInstance().debug("SalesOrderService: Sales order with number " + orderNumber + " not found.");
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::SalesOrderDTO> SalesOrderService::getAllSalesOrders(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesOrderService: Retrieving all sales orders with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewSalesOrders", "Bạn không có quyền xem tất cả đơn hàng bán.")) {
        return {};
    }

    return salesOrderDAO_->get(filter); // Using get from DAOBase template
}

bool SalesOrderService::updateSalesOrder(
    const ERP::Sales::DTO::SalesOrderDTO& salesOrderDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesOrderService: Attempting to update sales order: " + salesOrderDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateSalesOrder", "Bạn không có quyền cập nhật đơn hàng bán.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::SalesOrderDTO> oldSalesOrderOpt = salesOrderDAO_->getById(salesOrderDTO.id); // Using getById from DAOBase
    if (!oldSalesOrderOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesOrderService: Sales order with ID " + salesOrderDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy đơn hàng bán cần cập nhật.");
        return false;
    }

    // If order number is changed, check for uniqueness
    if (salesOrderDTO.orderNumber != oldSalesOrderOpt->orderNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["order_number"] = salesOrderDTO.orderNumber;
        if (salesOrderDAO_->count(filterByNumber) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("SalesOrderService: New sales order number " + salesOrderDTO.orderNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesOrderService: New sales order number " + salesOrderDTO.orderNumber + " already exists.", "Số đơn hàng bán mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate Customer existence (assuming CustomerService handles permission internally)
    std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(salesOrderDTO.customerId, userRoleIds);
    if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("SalesOrderService: Invalid Customer ID provided or customer is not active for update: " + salesOrderDTO.customerId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc khách hàng không hoạt động.");
        return false;
    }

    // Validate Warehouse existence (assuming WarehouseService handles permission internally)
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(salesOrderDTO.warehouseId, userRoleIds); // userRoleIds is passed here
    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("SalesOrderService: Invalid Warehouse ID provided or warehouse is not active for update: " + salesOrderDTO.warehouseId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
        return false;
    }

    ERP::Sales::DTO::SalesOrderDTO updatedSalesOrder = salesOrderDTO;
    updatedSalesOrder.updatedAt = ERP::Utils::DateUtils::now();
    updatedSalesOrder.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!salesOrderDAO_->update(updatedSalesOrder)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesOrderService: Failed to update sales order " + updatedSalesOrder.id + " in DAO.");
                return false;
            }
            // Optionally, update sales order details if they are part of the update operation
            return true;
        },
        "SalesOrderService", "updateSalesOrder"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesOrderService: Sales order " + updatedSalesOrder.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "SalesOrder", updatedSalesOrder.id, "SalesOrder", updatedSalesOrder.orderNumber,
                       oldSalesOrderOpt->toMap(), updatedSalesOrder.toMap(), "Sales order updated.");
        return true;
    }
    return false;
}

bool SalesOrderService::updateSalesOrderStatus(
    const std::string& salesOrderId,
    ERP::Sales::DTO::SalesOrderStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesOrderService: Attempting to update status for sales order: " + salesOrderId + " to " + ERP::Sales::DTO::SalesOrderDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateSalesOrder", "Bạn không có quyền cập nhật trạng thái đơn hàng bán.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrderOpt = salesOrderDAO_->getById(salesOrderId); // Using getById from DAOBase
    if (!salesOrderOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesOrderService: Sales order with ID " + salesOrderId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy đơn hàng bán để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Sales::DTO::SalesOrderDTO oldSalesOrder = *salesOrderOpt;
    if (oldSalesOrder.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("SalesOrderService: Sales order " + salesOrderId + " is already in status " + ERP::Sales::DTO::SalesOrderDTO().getStatusString(newStatus) + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from COMPLETED to IN_PROGRESS.

    ERP::Sales::DTO::SalesOrderDTO updatedSalesOrder = oldSalesOrder;
    updatedSalesOrder.status = newStatus;
    updatedSalesOrder.updatedAt = ERP::Utils::DateUtils::now();
    updatedSalesOrder.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!salesOrderDAO_->update(updatedSalesOrder)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesOrderService: Failed to update status for sales order " + salesOrderId + " in DAO.");
                return false;
            }
            // Optionally, publish event for status change
            // eventBus_.publish(std::make_shared<EventBus::SalesOrderStatusChangedEvent>(salesOrderId, newStatus));
            return true;
        },
        "SalesOrderService", "updateSalesOrderStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesOrderService: Status for sales order " + salesOrderId + " updated successfully to " + updatedSalesOrder.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "SalesOrderStatus", salesOrderId, "SalesOrder", oldSalesOrder.orderNumber,
                       oldSalesOrder.toMap(), updatedSalesOrder.toMap(), "Sales order status changed to " + updatedSalesOrder.getStatusString() + ".");
        return true;
    }
    return false;
}

bool SalesOrderService::deleteSalesOrder(
    const std::string& salesOrderId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesOrderService: Attempting to delete sales order: " + salesOrderId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.DeleteSalesOrder", "Bạn không có quyền xóa đơn hàng bán.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrderOpt = salesOrderDAO_->getById(salesOrderId); // Using getById from DAOBase
    if (!salesOrderOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesOrderService: Sales order with ID " + salesOrderId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy đơn hàng bán cần xóa.");
        return false;
    }

    ERP::Sales::DTO::SalesOrderDTO salesOrderToDelete = *salesOrderOpt;

    // Additional checks: Prevent deletion if there are associated invoices, shipments, or payments.
    // (This would require dependencies on SalesInvoiceService, SalesShipmentService, etc.)

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated details first
            if (!salesOrderDAO_->removeSalesOrderDetailsByOrderId(salesOrderId)) { // Specific DAO method for details
                ERP::Logger::Logger::getInstance().error("SalesOrderService: Failed to remove associated sales order details for order " + salesOrderId + ".");
                return false;
            }
            if (!salesOrderDAO_->remove(salesOrderId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesOrderService: Failed to delete sales order " + salesOrderId + " in DAO.");
                return false;
            }
            return true;
        },
        "SalesOrderService", "deleteSalesOrder"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesOrderService: Sales order " + salesOrderId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Sales", "SalesOrder", salesOrderId, "SalesOrder", salesOrderToDelete.orderNumber,
                       salesOrderToDelete.toMap(), std::nullopt, "Sales order deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Sales
} // namespace ERP