// Modules/Sales/Service/SalesShipmentService.cpp
#include "SalesShipmentService.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "SalesOrderService.h" // Đã rút gọn include
#include "CustomerService.h" // Đã rút gọn include
#include "WarehouseService.h" // Đã rút gọn include
#include "ProductService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Sales {
namespace Services {

SalesShipmentService::SalesShipmentService(
    std::shared_ptr<DAOs::ShipmentDAO> shipmentDAO,
    std::shared_ptr<ISalesOrderService> salesOrderService,
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      shipmentDAO_(shipmentDAO), salesOrderService_(salesOrderService),
      customerService_(customerService), warehouseService_(warehouseService),
      productService_(productService) {
    if (!shipmentDAO_ || !salesOrderService_ || !customerService_ || !warehouseService_ || !productService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "SalesShipmentService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ vận chuyển bán hàng.");
        ERP::Logger::Logger::getInstance().critical("SalesShipmentService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("SalesShipmentService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("SalesShipmentService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Sales::DTO::ShipmentDTO> SalesShipmentService::createShipment(
    const ERP::Sales::DTO::ShipmentDTO& shipmentDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesShipmentService: Attempting to create shipment: " + shipmentDTO.shipmentNumber + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.CreateShipment", "Bạn không có quyền tạo đơn vận chuyển.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (shipmentDTO.shipmentNumber.empty() || shipmentDTO.salesOrderId.empty() || shipmentDTO.customerId.empty()) {
        ERP::Logger::Logger::getInstance().warning("SalesShipmentService: Invalid input for shipment creation (empty number, salesOrderId, or customerId).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesShipmentService: Invalid input for shipment creation.", "Thông tin vận chuyển không đầy đủ.");
        return std::nullopt;
    }

    // Check if shipment number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["shipment_number"] = shipmentDTO.shipmentNumber;
    if (shipmentDAO_->count(filterByNumber) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("SalesShipmentService: Shipment with number " + shipmentDTO.shipmentNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesShipmentService: Shipment with number " + shipmentDTO.shipmentNumber + " already exists.", "Số đơn vận chuyển đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate Sales Order existence (assuming SalesOrderService handles permission internally)
    std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrder = salesOrderService_->getSalesOrderById(shipmentDTO.salesOrderId, userRoleIds);
    if (!salesOrder || salesOrder->status == ERP::Sales::DTO::SalesOrderStatus::CANCELLED || salesOrder->status == ERP::Sales::DTO::SalesOrderStatus::REJECTED) {
        ERP::Logger::Logger::getInstance().warning("SalesShipmentService: Invalid Sales Order ID provided or sales order is not valid for shipment: " + shipmentDTO.salesOrderId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID đơn hàng bán không hợp lệ hoặc đơn hàng không còn hiệu lực để vận chuyển.");
        return std::nullopt;
    }

    // Validate Customer existence (assuming CustomerService handles permission internally)
    std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(shipmentDTO.customerId); // No userRoleIds passed here, review
    if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("SalesShipmentService: Invalid Customer ID provided or customer is not active: " + shipmentDTO.customerId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc khách hàng không hoạt động.");
        return std::nullopt;
    }

    ERP::Sales::DTO::ShipmentDTO newShipment = shipmentDTO;
    newShipment.id = ERP::Utils::generateUUID();
    newShipment.createdAt = ERP::Utils::DateUtils::now();
    newShipment.createdBy = currentUserId;
    newShipment.status = ERP::Sales::DTO::ShipmentStatus::PENDING; // Default status

    std::optional<ERP::Sales::DTO::ShipmentDTO> createdShipment = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!shipmentDAO_->create(newShipment)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesShipmentService: Failed to create shipment " + newShipment.shipmentNumber + " in DAO.");
                return false;
            }
            createdShipment = newShipment;
            // Optionally, update sales order status or create shipment details here
            return true;
        },
        "SalesShipmentService", "createShipment"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesShipmentService: Shipment " + newShipment.shipmentNumber + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Shipment", newShipment.id, "Shipment", newShipment.shipmentNumber,
                       std::nullopt, newShipment.toMap(), "Shipment created.");
        return createdShipment;
    }
    return std::nullopt;
}

std::optional<ERP::Sales::DTO::ShipmentDTO> SalesShipmentService::getShipmentById(
    const std::string& shipmentId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesShipmentService: Retrieving shipment by ID: " + shipmentId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewShipments", "Bạn không có quyền xem đơn vận chuyển.")) {
        return std::nullopt;
    }

    return shipmentDAO_->getById(shipmentId); // Using getById from DAOBase template
}

std::optional<ERP::Sales::DTO::ShipmentDTO> SalesShipmentService::getShipmentByNumber(
    const std::string& shipmentNumber,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SalesShipmentService: Retrieving shipment by number: " + shipmentNumber + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewShipments", "Bạn không có quyền xem đơn vận chuyển.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["shipment_number"] = shipmentNumber;
    std::vector<ERP::Sales::DTO::ShipmentDTO> shipments = shipmentDAO_->get(filter); // Using get from DAOBase template
    if (!shipments.empty()) {
        return shipments[0];
    }
    ERP::Logger::Logger::getInstance().debug("SalesShipmentService: Shipment with number " + shipmentNumber + " not found.");
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::ShipmentDTO> SalesShipmentService::getAllShipments(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesShipmentService: Retrieving all shipments with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewShipments", "Bạn không có quyền xem tất cả đơn vận chuyển.")) {
        return {};
    }

    return shipmentDAO_->get(filter); // Using get from DAOBase template
}

bool SalesShipmentService::updateShipment(
    const ERP::Sales::DTO::ShipmentDTO& shipmentDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesShipmentService: Attempting to update shipment: " + shipmentDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateShipment", "Bạn không có quyền cập nhật đơn vận chuyển.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::ShipmentDTO> oldShipmentOpt = shipmentDAO_->getById(shipmentDTO.id); // Using getById from DAOBase
    if (!oldShipmentOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesShipmentService: Shipment with ID " + shipmentDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy đơn vận chuyển cần cập nhật.");
        return false;
    }

    // If shipment number is changed, check for uniqueness
    if (shipmentDTO.shipmentNumber != oldShipmentOpt->shipmentNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["shipment_number"] = shipmentDTO.shipmentNumber;
        if (shipmentDAO_->count(filterByNumber) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("SalesShipmentService: New shipment number " + shipmentDTO.shipmentNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SalesShipmentService: New shipment number " + shipmentDTO.shipmentNumber + " already exists.", "Số đơn vận chuyển mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate Sales Order existence
    std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrder = salesOrderService_->getSalesOrderById(shipmentDTO.salesOrderId, userRoleIds);
    if (!salesOrder || salesOrder->status == ERP::Sales::DTO::SalesOrderStatus::CANCELLED || salesOrder->status == ERP::Sales::DTO::SalesOrderStatus::REJECTED) {
        ERP::Logger::Logger::getInstance().warning("SalesShipmentService: Invalid Sales Order ID provided or sales order is not valid for update: " + shipmentDTO.salesOrderId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID đơn hàng bán không hợp lệ hoặc đơn hàng không còn hiệu lực.");
        return false;
    }

    // Validate Customer existence
    std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(shipmentDTO.customerId); // No userRoleIds passed here, review
    if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("SalesShipmentService: Invalid Customer ID provided or customer is not active for update: " + shipmentDTO.customerId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không hợp lệ hoặc khách hàng không hoạt động.");
        return false;
    }

    ERP::Sales::DTO::ShipmentDTO updatedShipment = shipmentDTO;
    updatedShipment.updatedAt = ERP::Utils::DateUtils::now();
    updatedShipment.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!shipmentDAO_->update(updatedShipment)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SalesShipmentService: Failed to update shipment " + updatedShipment.id + " in DAO.");
                return false;
            }
            // Optionally, update shipment details if they are part of the update operation
            return true;
        },
        "SalesShipmentService", "updateShipment"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SalesShipmentService: Shipment " + updatedShipment.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Sales", "Shipment", updatedShipment.id, "Shipment", updatedShipment.shipmentNumber,
                       oldShipmentOpt->toMap(), updatedShipment.toMap(), "Shipment updated.");
        return true;
    }
    return false;
}

bool SalesShipmentService::updateShipmentStatus(
    const std::string& shipmentId,
    ERP::Sales::DTO::ShipmentStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SalesShipmentService: Attempting to update status for shipment: " + shipmentId + " to " + ERP::Sales::DTO::ShipmentDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateShipment", "Bạn không có quyền cập nhật trạng thái đơn vận chuyển.")) {
        return false;
    }

    std::optional<ERP::Sales::DTO::ShipmentDTO> shipmentOpt = shipmentDAO_->getById(shipmentId); // Using getById from DAOBase
    if (!shipmentOpt) {
        ERP::Logger::Logger::getInstance().warning("SalesShipmentService: Shipment with ID " + shipmentId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy đơn vận chuyển để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Sales::DTO::ShipmentDTO oldShipment = *shipmentOpt;
    if (oldShipment.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("SalesShipmentService: Shipment " + shipmentId + " is already in status " + ERP::Sales::DTO::ShipmentDTO().getStatusString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Sales::DTO::ShipmentDTO updatedShipment = oldShipment;
    updated