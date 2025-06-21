// Modules/Warehouse/Service/InventoryTransactionService.cpp
#include "InventoryTransactionService.h"
#include "InventoryTransaction.h"   // InventoryTransaction DTO
#include "Product.h"                // Product DTO
#include "Warehouse.h"              // Warehouse DTO
#include "Location.h"               // Location DTO
#include "Event.h"                  // Event DTO
#include "ConnectionPool.h"         // ConnectionPool
#include "DBConnection.h"           // DBConnection
#include "Common.h"                 // Common Enums/Constants
#include "Utils.h"                  // Utility functions
#include "DateUtils.h"              // Date utility functions
#include "AutoRelease.h"            // RAII helper
#include "ISecurityManager.h"       // Security Manager interface
#include "UserService.h"            // User Service (for audit logging)
#include "ProductService.h"         // ProductService (for product validation)
#include "WarehouseService.h"       // WarehouseService (for warehouse validation)
#include "LocationService.h"        // LocationService (for location validation)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
namespace Warehouse {
namespace Services {

InventoryTransactionService::InventoryTransactionService(
    std::shared_ptr<DAOs::InventoryTransactionDAO> inventoryTransactionDAO,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
      inventoryTransactionDAO_(inventoryTransactionDAO),
      productService_(productService),
      warehouseService_(warehouseService),
      locationService_(locationService) {
    
    if (!inventoryTransactionDAO_ || !productService_ || !warehouseService_ || !locationService_ || !securityManager_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "InventoryTransactionService: One or more injected services/DAOs are null.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ giao dịch tồn kho.");
        ERP::Logger::Logger::getInstance().critical("InventoryTransactionService: One or more injected services/DAOs are null.");
        throw std::runtime_error("InventoryTransactionService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("InventoryTransactionService: Initialized.");
}

// checkUserPermission and getUserRoleIds are now inherited from BaseService

std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> InventoryTransactionService::createInventoryTransaction(
    const ERP::Warehouse::DTO::InventoryTransactionDTO& transactionDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryTransactionService: Attempting to create transaction for product " + transactionDTO.productId + " type " + transactionDTO.getTypeString() + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.CreateInventoryTransaction", "Bạn không có quyền tạo giao dịch tồn kho.")) { // Specific permission for this service
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (transactionDTO.productId.empty() || transactionDTO.warehouseId.empty() || transactionDTO.locationId.empty() || transactionDTO.quantity == 0 || transactionDTO.unitOfMeasureId.empty() || transactionDTO.type == ERP::Warehouse::DTO::InventoryTransactionType::UNKNOWN) {
        ERP::Logger::Logger::getInstance().warning("InventoryTransactionService: Invalid input for transaction creation (missing essential fields).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin giao dịch tồn kho không đầy đủ.");
        return std::nullopt;
    }
    // Negative quantity check is typically done in InventoryService before calling this to ensure stock is valid.
    // Here, we just ensure it's not zero.

    // 2. Validate Product, Warehouse, Location existence and active status
    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(transactionDTO.productId, userRoleIds);
    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("InventoryTransactionService: Invalid or inactive Product ID: " + transactionDTO.productId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID sản phẩm không hợp lệ hoặc không hoạt động.");
        return std::nullopt;
    }
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(transactionDTO.warehouseId, userRoleIds);
    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("InventoryTransactionService: Invalid or inactive Warehouse ID: " + transactionDTO.warehouseId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc không hoạt động.");
        return std::nullopt;
    }
    std::optional<ERP::Catalog::DTO::LocationDTO> location = locationService_->getLocationById(transactionDTO.locationId, userRoleIds);
    if (!location || location->status != ERP::Common::EntityStatus::ACTIVE || location->warehouseId != transactionDTO.warehouseId) {
        ERP::Logger::Logger::getInstance().warning("InventoryTransactionService: Invalid or inactive Location ID: " + transactionDTO.locationId + " for warehouse " + transactionDTO.warehouseId + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID vị trí không hợp lệ hoặc không hoạt động.");
        return std::nullopt;
    }
    // Validate Unit of Measure
    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uom = unitOfMeasureService_->getUnitOfMeasureById(transactionDTO.unitOfMeasureId, userRoleIds);
    if (!uom || uom->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("InventoryTransactionService: Invalid or inactive Unit of Measure ID: " + transactionDTO.unitOfMeasureId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID đơn vị đo không hợp lệ hoặc không hoạt động.");
        return std::nullopt;
    }

    std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> createdTransaction = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            ERP::Warehouse::DTO::InventoryTransactionDTO newTransaction = transactionDTO;
            newTransaction.id = ERP::Utils::generateUUID();
            newTransaction.createdAt = ERP::Utils::DateUtils::now();
            newTransaction.createdBy = currentUserId;
            newTransaction.status = ERP::Common::EntityStatus::ACTIVE; // Transactions are generally active upon creation
            newTransaction.transactionDate = ERP::Utils::DateUtils::now(); // Ensure date is current

            if (!inventoryTransactionDAO_->save(newTransaction)) { // Use templated save
                ERP::Logger::Logger::getInstance().error("InventoryTransactionService: Failed to create transaction " + newTransaction.id + " in DAO.");
                return false;
            }
            createdTransaction = newTransaction;
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::InventoryTransactionCreatedEvent>(newTransaction.id)); // Example event
            return true;
        },
        "InventoryTransactionService", "createInventoryTransaction"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryTransactionService: Transaction " + createdTransaction->id + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "InventoryTransaction", createdTransaction->id, "InventoryTransaction", createdTransaction->productId,
                       std::nullopt, createdTransaction->toMap(), "Inventory transaction created.");
        return createdTransaction;
    }
    return std::nullopt;
}

std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> InventoryTransactionService::getInventoryTransactionById(
    const std::string& transactionId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("InventoryTransactionService: Retrieving transaction by ID: " + transactionId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewInventoryTransactions", "Bạn không có quyền xem giao dịch tồn kho.")) {
        return std::nullopt;
    }

    return inventoryTransactionDAO_->findById(transactionId); // Use templated findById
}

std::vector<ERP::Warehouse::DTO::InventoryTransactionDTO> InventoryTransactionService::getAllInventoryTransactions(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryTransactionService: Retrieving all transactions with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewInventoryTransactions", "Bạn không có quyền xem tất cả giao dịch tồn kho.")) {
        return {};
    }

    return inventoryTransactionDAO_->getInventoryTransactions(filter); // Use specific DAO method
}

} // namespace Services
} // namespace Warehouse
} // namespace ERP