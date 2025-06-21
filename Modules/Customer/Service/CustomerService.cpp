// Modules/Customer/Service/CustomerService.cpp
#include "CustomerService.h" // Đã rút gọn include
#include "Customer.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Customer {
namespace Services {

CustomerService::CustomerService(
    std::shared_ptr<DAOs::CustomerDAO> customerDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      customerDAO_(customerDAO) {
    if (!customerDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "CustomerService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ khách hàng.");
        ERP::Logger::Logger::getInstance().critical("CustomerService: Injected CustomerDAO is null.");
        throw std::runtime_error("CustomerService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("CustomerService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Customer::DTO::CustomerDTO> CustomerService::createCustomer(
    const ERP::Customer::DTO::CustomerDTO& customerDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CustomerService: Attempting to create customer: " + customerDTO.name + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Customer.CreateCustomer", "Bạn không có quyền tạo khách hàng.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (customerDTO.name.empty()) {
        ERP::Logger::Logger::getInstance().warning("CustomerService: Invalid input for customer creation (empty name).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "CustomerService: Invalid input for customer creation.", "Tên khách hàng không được để trống.");
        return std::nullopt;
    }

    // Check if customer name already exists
    std::map<std::string, std::any> filterByName;
    filterByName["name"] = customerDTO.name;
    if (customerDAO_->count(filterByName) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("CustomerService: Customer with name " + customerDTO.name + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "CustomerService: Customer with name " + customerDTO.name + " already exists.", "Tên khách hàng đã tồn tại. Vui lòng chọn tên khác.");
        return std::nullopt;
    }

    ERP::Customer::DTO::CustomerDTO newCustomer = customerDTO;
    newCustomer.id = ERP::Utils::generateUUID();
    newCustomer.createdAt = ERP::Utils::DateUtils::now();
    newCustomer.createdBy = currentUserId;
    newCustomer.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Customer::DTO::CustomerDTO> createdCustomer = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!customerDAO_->create(newCustomer)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("CustomerService: Failed to create customer " + newCustomer.name + " in DAO.");
                return false;
            }
            createdCustomer = newCustomer;
            eventBus_.publish(std::make_shared<EventBus::CustomerCreatedEvent>(newCustomer.id, newCustomer.name));
            return true;
        },
        "CustomerService", "createCustomer"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("CustomerService: Customer " + newCustomer.name + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Customer", "Customer", newCustomer.id, "Customer", newCustomer.name,
                       std::nullopt, newCustomer.toMap(), "Customer created.");
        return createdCustomer;
    }
    return std::nullopt;
}

std::optional<ERP::Customer::DTO::CustomerDTO> CustomerService::getCustomerById(
    const std::string& customerId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("CustomerService: Retrieving customer by ID: " + customerId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Customer.ViewCustomers", "Bạn không có quyền xem khách hàng.")) {
        return std::nullopt;
    }

    return customerDAO_->getById(customerId); // Using getById from DAOBase template
}

std::optional<ERP::Customer::DTO::CustomerDTO> CustomerService::getCustomerByName(
    const std::string& customerName,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("CustomerService: Retrieving customer by name: " + customerName + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Customer.ViewCustomers", "Bạn không có quyền xem khách hàng.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["name"] = customerName;
    std::vector<ERP::Customer::DTO::CustomerDTO> customers = customerDAO_->get(filter); // Using get from DAOBase template
    if (!customers.empty()) {
        return customers[0];
    }
    ERP::Logger::Logger::getInstance().debug("CustomerService: Customer with name " + customerName + " not found.");
    return std::nullopt;
}

std::vector<ERP::Customer::DTO::CustomerDTO> CustomerService::getAllCustomers(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CustomerService: Retrieving all customers with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Customer.ViewCustomers", "Bạn không có quyền xem tất cả khách hàng.")) {
        return {};
    }

    return customerDAO_->get(filter); // Using get from DAOBase template
}

bool CustomerService::updateCustomer(
    const ERP::Customer::DTO::CustomerDTO& customerDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CustomerService: Attempting to update customer: " + customerDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Customer.UpdateCustomer", "Bạn không có quyền cập nhật khách hàng.")) {
        return false;
    }

    std::optional<ERP::Customer::DTO::CustomerDTO> oldCustomerOpt = customerDAO_->getById(customerDTO.id); // Using getById from DAOBase
    if (!oldCustomerOpt) {
        ERP::Logger::Logger::getInstance().warning("CustomerService: Customer with ID " + customerDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy khách hàng cần cập nhật.");
        return false;
    }

    // If customer name is changed, check for uniqueness
    if (customerDTO.name != oldCustomerOpt->name) {
        std::map<std::string, std::any> filterByName;
        filterByName["name"] = customerDTO.name;
        if (customerDAO_->count(filterByName) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("CustomerService: New customer name " + customerDTO.name + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "CustomerService: New customer name " + customerDTO.name + " already exists.", "Tên khách hàng mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }

    ERP::Customer::DTO::CustomerDTO updatedCustomer = customerDTO;
    updatedCustomer.updatedAt = ERP::Utils::DateUtils::now();
    updatedCustomer.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!customerDAO_->update(updatedCustomer)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("CustomerService: Failed to update customer " + updatedCustomer.id + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::CustomerUpdatedEvent>(updatedCustomer.id, updatedCustomer.name));
            return true;
        },
        "CustomerService", "updateCustomer"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("CustomerService: Customer " + updatedCustomer.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Customer", "Customer", updatedCustomer.id, "Customer", updatedCustomer.name,
                       oldCustomerOpt->toMap(), updatedCustomer.toMap(), "Customer updated.");
        return true;
    }
    return false;
}

bool CustomerService::updateCustomerStatus(
    const std::string& customerId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CustomerService: Attempting to update status for customer: " + customerId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Customer.ChangeCustomerStatus", "Bạn không có quyền cập nhật trạng thái khách hàng.")) {
        return false;
    }

    std::optional<ERP::Customer::DTO::CustomerDTO> customerOpt = customerDAO_->getById(customerId); // Using getById from DAOBase
    if (!customerOpt) {
        ERP::Logger::Logger::getInstance().warning("CustomerService: Customer with ID " + customerId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy khách hàng để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Customer::DTO::CustomerDTO oldCustomer = *customerOpt;
    if (oldCustomer.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("CustomerService: Customer " + customerId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Customer::DTO::CustomerDTO updatedCustomer = oldCustomer;
    updatedCustomer.status = newStatus;
    updatedCustomer.updatedAt = ERP::Utils::DateUtils::now();
    updatedCustomer.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!customerDAO_->update(updatedCustomer)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("CustomerService: Failed to update status for customer " + customerId + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::CustomerStatusChangedEvent>(customerId, newStatus));
            return true;
        },
        "CustomerService", "updateCustomerStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("CustomerService: Status for customer " + customerId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Customer", "CustomerStatus", customerId, "Customer", oldCustomer.name,
                       oldCustomer.toMap(), updatedCustomer.toMap(), "Customer status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool CustomerService::deleteCustomer(
    const std::string& customerId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CustomerService: Attempting to delete customer: " + customerId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Customer.DeleteCustomer", "Bạn không có quyền xóa khách hàng.")) {
        return false;
    }

    std::optional<ERP::Customer::DTO::CustomerDTO> customerOpt = customerDAO_->getById(customerId); // Using getById from DAOBase
    if (!customerOpt) {
        ERP::Logger::Logger::getInstance().warning("CustomerService: Customer with ID " + customerId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy khách hàng cần xóa.");
        return false;
    }

    ERP::Customer::DTO::CustomerDTO customerToDelete = *customerOpt;

    // Additional checks: Prevent deletion if customer has associated sales orders, invoices, payments, or AR balances
    // This would require dependencies on SalesOrderService, SalesInvoiceService, PaymentService, AccountReceivableService
    std::map<std::string, std::any> salesOrderFilter;
    salesOrderFilter["customer_id"] = customerId;
    if (securityManager_->getSalesOrderService()->getAllSalesOrders(salesOrderFilter).size() > 0) { // Assuming SalesOrderService can query by customer
        ERP::Logger::Logger::getInstance().warning("CustomerService: Cannot delete customer " + customerId + " as it has associated sales orders.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa khách hàng có đơn hàng bán liên quan.");
        return false;
    }
    // Similar checks for Invoices, Payments, AccountReceivableBalances

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!customerDAO_->remove(customerId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("CustomerService: Failed to delete customer " + customerId + " in DAO.");
                return false;
            }
            return true;
        },
        "CustomerService", "deleteCustomer"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("CustomerService: Customer " + customerId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Customer", "Customer", customerId, "Customer", customerToDelete.name,
                       customerToDelete.toMap(), std::nullopt, "Customer deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Customer
} // namespace ERP