// Modules/Warehouse/Service/InventoryManagementService.cpp
#include "InventoryManagementService.h" // Standard includes
#include "Inventory.h"              // Inventory DTO
#include "InventoryTransaction.h"   // InventoryTransaction DTO
#include "InventoryCostLayer.h"     // InventoryCostLayer DTO
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
#include "InventoryTransactionService.h" // InventoryTransactionService (for recording transactions)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
namespace Warehouse {
namespace Services {

InventoryManagementService::InventoryManagementService(
    std::shared_ptr<DAOs::InventoryDAO> inventoryDAO,
    std::shared_ptr<DAOs::InventoryTransactionDAO> inventoryTransactionDAO, // Note: This DAO is now used directly within InventoryTransactionService, not here.
    std::shared_ptr<DAOs::InventoryCostLayerDAO> inventoryCostLayerDAO,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService,
    std::shared_ptr<ERP::Warehouse::Services::IInventoryTransactionService> inventoryTransactionService, // NEW: Dependency on InventoryTransactionService
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
      inventoryDAO_(inventoryDAO),
      inventoryCostLayerDAO_(inventoryCostLayerDAO),
      productService_(productService),
      warehouseService_(warehouseService),
      locationService_(locationService),
      inventoryTransactionService_(inventoryTransactionService) { // NEW: Initialize InventoryTransactionService
    
    // Check for null dependencies
    if (!inventoryDAO_ || !inventoryCostLayerDAO_ || !productService_ || !warehouseService_ || !locationService_ || !inventoryTransactionService_ || !securityManager_) {
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "InventoryManagementService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ quản lý tồn kho.");
        ERP::Logger::Logger::getInstance().critical("InventoryManagementService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("InventoryManagementService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Initialized.");
}

std::optional<ERP::Warehouse::DTO::InventoryDTO> InventoryManagementService::createInventory(
    const ERP::Warehouse::DTO::InventoryDTO& inventoryDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Attempting to create inventory record for product: " + inventoryDTO.productId + " at " + inventoryDTO.warehouseId + "/" + inventoryDTO.locationId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.CreateInventory", "Bạn không có quyền tạo bản ghi tồn kho.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (inventoryDTO.productId.empty() || inventoryDTO.warehouseId.empty() || inventoryDTO.locationId.empty()) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Invalid input for inventory creation (missing product, warehouse, or location).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin tồn kho không đầy đủ.");
        return std::nullopt;
    }

    // Check for duplicate inventory record (product + warehouse + location unique constraint)
    if (getInventoryByProductLocation(inventoryDTO.productId, inventoryDTO.warehouseId, inventoryDTO.locationId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Inventory record already exists for product " + inventoryDTO.productId + " at " + inventoryDTO.warehouseId + "/" + inventoryDTO.locationId + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Bản ghi tồn kho đã tồn tại cho sản phẩm tại vị trí này.");
        return std::nullopt;
    }

    // Validate Product, Warehouse, Location existence and active status
    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(inventoryDTO.productId, userRoleIds);
    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Invalid or inactive Product ID: " + inventoryDTO.productId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID sản phẩm không hợp lệ hoặc không hoạt động.");
        return std::nullopt;
    }
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(inventoryDTO.warehouseId, userRoleIds);
    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Invalid or inactive Warehouse ID: " + inventoryDTO.warehouseId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc không hoạt động.");
        return std::nullopt;
    }
    std::optional<ERP::Catalog::DTO::LocationDTO> location = locationService_->getLocationById(inventoryDTO.locationId, userRoleIds);
    if (!location || location->status != ERP::Common::EntityStatus::ACTIVE || location->warehouseId != inventoryDTO.warehouseId) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Invalid or inactive Location ID: " + inventoryDTO.locationId + " for warehouse " + inventoryDTO.warehouseId + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID vị trí không hợp lệ hoặc không hoạt động.");
        return std::nullopt;
    }

    ERP::Warehouse::DTO::InventoryDTO newInventory = inventoryDTO;
    newInventory.id = ERP::Utils::generateUUID();
    newInventory.createdAt = ERP::Utils::DateUtils::now();
    newInventory.createdBy = currentUserId;
    newInventory.status = ERP::Common::EntityStatus::ACTIVE; // Default active
    newInventory.quantity = 0.0; // Start with zero quantity unless it's a receipt
    newInventory.reservedQuantity = 0.0;
    newInventory.availableQuantity = 0.0;
    newInventory.unitCost = 0.0; // Default

    std::optional<ERP::Warehouse::DTO::InventoryDTO> createdInventory = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!inventoryDAO_->create(newInventory)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to create inventory record in DAO.");
                return false;
            }
            createdInventory = newInventory;
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::InventoryCreatedEvent>(newInventory.id, newInventory.productId, newInventory.warehouseId, newInventory.locationId)); // Assuming such an event
            return true;
        },
        "InventoryManagementService", "createInventory"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Inventory record for product " + newInventory.productId + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "Inventory", newInventory.id, "Inventory", newInventory.productId + "/" + newInventory.warehouseId + "/" + newInventory.locationId,
                       std::nullopt, newInventory.toMap(), "Inventory record created.");
        return createdInventory;
    }
    return std::nullopt;
}

std::optional<ERP::Warehouse::DTO::InventoryDTO> InventoryManagementService::getInventoryById(
    const std::string& inventoryId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("InventoryManagementService: Retrieving inventory record by ID: " + inventoryId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewInventory", "Bạn không có quyền xem bản ghi tồn kho.")) {
        return std::nullopt;
    }

    return inventoryDAO_->findById(inventoryId); // Specific DAO method
}

std::optional<ERP::Warehouse::DTO::InventoryDTO> InventoryManagementService::getInventoryByProductLocation(
    const std::string& productId,
    const std::string& warehouseId,
    const std::string& locationId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("InventoryManagementService: Retrieving inventory for product " + productId + " at " + warehouseId + "/" + locationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewInventory", "Bạn không có quyền xem tồn kho.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["product_id"] = productId;
    filter["warehouse_id"] = warehouseId;
    filter["location_id"] = locationId;
    std::vector<ERP::Warehouse::DTO::InventoryDTO> results = inventoryDAO_->getInventory(filter);
    if (!results.empty()) {
        return results[0];
    }
    return std::nullopt;
}

std::vector<ERP::Warehouse::DTO::InventoryDTO> InventoryManagementService::getAllInventory(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Retrieving all inventory records with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewInventory", "Bạn không có quyền xem tất cả bản ghi tồn kho.")) {
        return {};
    }

    return inventoryDAO_->getInventory(filter);
}

std::vector<ERP::Warehouse::DTO::InventoryDTO> InventoryManagementService::getInventoryByProduct(
    const std::string& productId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Retrieving inventory for product: " + productId + " across all locations.");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewInventory", "Bạn không có quyền xem tồn kho theo sản phẩm.")) {
        return {};
    }

    std::map<std::string, std::any> filter;
    filter["product_id"] = productId;
    return inventoryDAO_->getInventory(filter);
}

bool InventoryManagementService::updateInventory(
    const ERP::Warehouse::DTO::InventoryDTO& inventoryDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Attempting to update inventory record: " + inventoryDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.UpdateInventory", "Bạn không có quyền cập nhật bản ghi tồn kho.")) {
        return false;
    }

    std::optional<ERP::Warehouse::DTO::InventoryDTO> oldInventoryOpt = inventoryDAO_->getInventoryById(inventoryDTO.id);
    if (!oldInventoryOpt) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Inventory record with ID " + inventoryDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy bản ghi tồn kho cần cập nhật.");
        return false;
    }
    
    // Prevent changing product/warehouse/location of existing inventory record
    if (inventoryDTO.productId != oldInventoryOpt->productId ||
        inventoryDTO.warehouseId != oldInventoryOpt->warehouseId ||
        inventoryDTO.locationId != oldInventoryOpt->locationId) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Attempted to change immutable fields (product, warehouse, location) for inventory record " + inventoryDTO.id + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Không thể thay đổi sản phẩm, kho hàng, vị trí của bản ghi tồn kho hiện có.");
        return false;
    }

    ERP::Warehouse::DTO::InventoryDTO updatedInventory = inventoryDTO;
    updatedInventory.updatedAt = ERP::Utils::DateUtils::now();
    updatedInventory.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!inventoryDAO_->update(updatedInventory)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to update inventory record " + updatedInventory.id + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::InventoryUpdatedEvent>(updatedInventory.id, updatedInventory.productId, updatedInventory.quantity)); // Assuming such an event
            return true;
        },
        "InventoryManagementService", "updateInventory"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Inventory record " + updatedInventory.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "Inventory", updatedInventory.id, "Inventory", updatedInventory.productId,
                       oldInventoryOpt->toMap(), updatedInventory.toMap(), "Inventory record updated.");
        return true;
    }
    return false;
}

bool InventoryManagementService::recordGoodsReceipt(
    const ERP::Warehouse::DTO::InventoryTransactionDTO& transactionDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Recording goods receipt for product " + transactionDTO.productId + ", quantity: " + std::to_string(transactionDTO.quantity) + " at " + transactionDTO.warehouseId + "/" + transactionDTO.locationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.RecordGoodsReceipt", "Bạn không có quyền ghi nhận nhập kho.")) {
        return false;
    }
    if (transactionDTO.type != ERP::Warehouse::DTO::InventoryTransactionType::GOODS_RECEIPT) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Invalid transaction type for goods receipt: " + transactionDTO.getTypeString());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Loại giao dịch không hợp lệ cho nhập kho.");
        return false;
    }
    if (transactionDTO.quantity <= 0) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Goods receipt quantity must be positive.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng nhập kho phải là số dương.");
        return false;
    }

    std::optional<ERP::Warehouse::DTO::InventoryDTO> inventoryOpt = getInventoryByProductLocation(transactionDTO.productId, transactionDTO.warehouseId, transactionDTO.locationId, userRoleIds);
    ERP::Warehouse::DTO::InventoryDTO currentInventory;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Step 1: Record the inventory transaction
            std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> createdTransaction = 
                inventoryTransactionService_->createInventoryTransaction(transactionDTO, currentUserId, userRoleIds); // Use the new service
            if (!createdTransaction) {
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to create goods receipt transaction.");
                return false;
            }

            // Step 2: Update (or create) the inventory record
            if (!inventoryOpt) {
                // If inventory record does not exist, create it.
                ERP::Logger::Logger::getInstance().info("InventoryManagementService: Inventory record not found, creating new for product " + transactionDTO.productId + " at " + transactionDTO.warehouseId + "/" + transactionDTO.locationId + ".");
                ERP::Warehouse::DTO::InventoryDTO newInventory;
                newInventory.id = ERP::Utils::generateUUID();
                newInventory.productId = transactionDTO.productId;
                newInventory.warehouseId = transactionDTO.warehouseId;
                newInventory.locationId = transactionDTO.locationId;
                newInventory.quantity = transactionDTO.quantity;
                newInventory.availableQuantity = newInventory.quantity - newInventory.reservedQuantity.value_or(0.0);
                newInventory.unitCost = transactionDTO.unitCost;
                newInventory.createdAt = ERP::Utils::DateUtils::now();
                newInventory.createdBy = currentUserId;
                newInventory.status = ERP::Common::EntityStatus::ACTIVE;
                // Copy other fields like lot/serial from transaction if applicable
                newInventory.lotNumber = transactionDTO.lotNumber;
                newInventory.serialNumber = transactionDTO.serialNumber;
                newInventory.manufactureDate = transactionDTO.manufactureDate;
                newInventory.expirationDate = transactionDTO.expirationDate;

                if (!inventoryDAO_->create(newInventory)) {
                    ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to create new inventory record for goods receipt.");
                    return false;
                }
                currentInventory = newInventory;
            } else {
                // Update existing inventory record
                currentInventory = *inventoryOpt;
                double oldQuantity = currentInventory.quantity;
                currentInventory.quantity += transactionDTO.quantity;
                currentInventory.availableQuantity = currentInventory.quantity - currentInventory.reservedQuantity.value_or(0.0);
                // Update unit cost (e.g., using Weighted Average Cost or FIFO/LIFO logic)
                // For simplicity, let's use a basic average cost update or just latest cost
                if (oldQuantity + transactionDTO.quantity > 0) { // Avoid division by zero
                    currentInventory.unitCost = (oldQuantity * currentInventory.unitCost + transactionDTO.quantity * transactionDTO.unitCost) / (oldQuantity + transactionDTO.quantity);
                } else {
                    currentInventory.unitCost = transactionDTO.unitCost; // If starting from 0 or negative
                }
                currentInventory.updatedAt = ERP::Utils::DateUtils::now();
                currentInventory.updatedBy = currentUserId;

                if (!inventoryDAO_->update(currentInventory)) {
                    ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to update existing inventory record for goods receipt.");
                    return false;
                }
            }

            // Step 3: Record inventory cost layer (if using costing methods like FIFO/LIFO)
            // This is crucial for accurate cost of goods sold.
            ERP::Warehouse::DTO::InventoryCostLayerDTO newCostLayer;
            newCostLayer.id = ERP::Utils::generateUUID();
            newCostLayer.productId = transactionDTO.productId;
            newCostLayer.warehouseId = transactionDTO.warehouseId;
            newCostLayer.locationId = transactionDTO.locationId;
            newCostLayer.receiptDate = transactionDTO.transactionDate;
            newCostLayer.quantity = transactionDTO.quantity;
            newCostLayer.unitCost = transactionDTO.unitCost;
            newCostLayer.remainingQuantity = transactionDTO.quantity; // Initially, all quantity remains
            newCostLayer.createdAt = ERP::Utils::DateUtils::now();
            newCostLayer.createdBy = currentUserId;
            newCostLayer.status = ERP::Common::EntityStatus::ACTIVE; // Active layer

            if (!inventoryCostLayerDAO_->create(newCostLayer)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to record inventory cost layer for goods receipt.");
                return false;
            }

            // Optionally, publish event for Inventory Level Change
            eventBus_.publish(std::make_shared<EventBus::InventoryLevelChangedEvent>(
                currentInventory.productId, currentInventory.warehouseId, currentInventory.locationId,
                inventoryOpt ? inventoryOpt->quantity : 0.0, currentInventory.quantity, "GoodsReceipt"
            ));
            return true;
        },
        "InventoryManagementService", "recordGoodsReceipt"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Goods receipt recorded successfully for product " + transactionDTO.productId + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "GoodsReceipt", currentInventory.id, "Inventory", currentInventory.productId,
                       std::nullopt, currentInventory.toMap(), "Goods receipt recorded.");
        return true;
    }
    return false;
}

bool InventoryManagementService::recordGoodsIssue(
    const ERP::Warehouse::DTO::InventoryTransactionDTO& transactionDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Recording goods issue for product " + transactionDTO.productId + ", quantity: " + std::to_string(transactionDTO.quantity) + " at " + transactionDTO.warehouseId + "/" + transactionDTO.locationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.RecordGoodsIssue", "Bạn không có quyền ghi nhận xuất kho.")) {
        return false;
    }
    if (transactionDTO.type != ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Invalid transaction type for goods issue: " + transactionDTO.getTypeString());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Loại giao dịch không hợp lệ cho xuất kho.");
        return false;
    }
    if (transactionDTO.quantity <= 0) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Goods issue quantity must be positive.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng xuất kho phải là số dương.");
        return false;
    }

    std::optional<ERP::Warehouse::DTO::InventoryDTO> inventoryOpt = getInventoryByProductLocation(transactionDTO.productId, transactionDTO.warehouseId, transactionDTO.locationId, userRoleIds);
    if (!inventoryOpt) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Inventory record not found for product " + transactionDTO.productId + " at " + transactionDTO.warehouseId + "/" + transactionDTO.locationId + " for goods issue.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy bản ghi tồn kho cho sản phẩm tại vị trí này.");
        return false;
    }
    ERP::Warehouse::DTO::InventoryDTO currentInventory = *inventoryOpt;

    // Check if enough quantity available for issue (considering reserved vs. actual quantity)
    // Here we check against total quantity, assuming reserved is handled.
    if (currentInventory.quantity < transactionDTO.quantity) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Insufficient quantity for goods issue. Product " + transactionDTO.productId + ", available: " + std::to_string(currentInventory.quantity) + ", requested: " + std::to_string(transactionDTO.quantity) + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InsufficientStock, "Không đủ số lượng tồn kho để xuất.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Step 1: Record the inventory transaction
            std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> createdTransaction = 
                inventoryTransactionService_->createInventoryTransaction(transactionDTO, currentUserId, userRoleIds); // Use the new service
            if (!createdTransaction) {
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to create goods issue transaction.");
                return false;
            }

            // Step 2: Update the inventory record
            double oldQuantity = currentInventory.quantity;
            currentInventory.quantity -= transactionDTO.quantity;
            currentInventory.availableQuantity = currentInventory.quantity - currentInventory.reservedQuantity.value_or(0.0);
            currentInventory.updatedAt = ERP::Utils::DateUtils::now();
            currentInventory.updatedBy = currentUserId;

            if (!inventoryDAO_->update(currentInventory)) {
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to update existing inventory record for goods issue.");
                return false;
            }

            // Step 3: Consume from inventory cost layers (e.g., FIFO/LIFO logic)
            if (!consumeInventoryCostLayers(transactionDTO.productId, transactionDTO.warehouseId, transactionDTO.locationId, transactionDTO.quantity, currentUserId, userRoleIds)) {
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to consume from inventory cost layers for goods issue.");
                return false;
            }

            // Optionally, publish event for Inventory Level Change
            eventBus_.publish(std::make_shared<EventBus::InventoryLevelChangedEvent>(
                currentInventory.productId, currentInventory.warehouseId, currentInventory.locationId,
                oldQuantity, currentInventory.quantity, "GoodsIssue"
            ));
            return true;
        },
        "InventoryManagementService", "recordGoodsIssue"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Goods issue recorded successfully for product " + transactionDTO.productId + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "GoodsIssue", currentInventory.id, "Inventory", currentInventory.productId,
                       std::nullopt, currentInventory.toMap(), "Goods issue recorded.");
        return true;
    }
    return false;
}

bool InventoryManagementService::adjustInventory(
    const ERP::Warehouse::DTO::InventoryTransactionDTO& transactionDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Adjusting inventory for product " + transactionDTO.productId + ", quantity: " + std::to_string(transactionDTO.quantity) + " at " + transactionDTO.warehouseId + "/" + transactionDTO.locationId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.AdjustInventoryManual", "Bạn không có quyền điều chỉnh tồn kho.")) {
        return false;
    }
    if (transactionDTO.type != ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN && transactionDTO.type != ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_OUT) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Invalid transaction type for adjustment: " + transactionDTO.getTypeString());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Loại giao dịch không hợp lệ cho điều chỉnh tồn kho.");
        return false;
    }
    if (transactionDTO.quantity < 0) { // Quantity in DTO should always be positive for adjustment, type determines In/Out
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Adjustment quantity must be non-negative.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng điều chỉnh phải là số không âm.");
        return false;
    }

    std::optional<ERP::Warehouse::DTO::InventoryDTO> inventoryOpt = getInventoryByProductLocation(transactionDTO.productId, transactionDTO.warehouseId, transactionDTO.locationId, userRoleIds);
    ERP::Warehouse::DTO::InventoryDTO currentInventory;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Step 1: Record the inventory transaction
            std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> createdTransaction = 
                inventoryTransactionService_->createInventoryTransaction(transactionDTO, currentUserId, userRoleIds); // Use the new service
            if (!createdTransaction) {
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to create inventory adjustment transaction.");
                return false;
            }

            // Step 2: Update (or create) the inventory record based on adjustment type
            if (!inventoryOpt) {
                // If inventory record does not exist, create it.
                // This usually happens for ADJ_IN, starting from zero.
                if (transactionDTO.type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_OUT) {
                     ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Cannot perform ADJUSTMENT_OUT on non-existent inventory record.");
                     ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy bản ghi tồn kho để điều chỉnh giảm.");
                     return false;
                }
                ERP::Logger::Logger::getInstance().info("InventoryManagementService: Inventory record not found, creating new for adjustment " + transactionDTO.productId + " at " + transactionDTO.warehouseId + "/" + transactionDTO.locationId + ".");
                ERP::Warehouse::DTO::InventoryDTO newInventory;
                newInventory.id = ERP::Utils::generateUUID();
                newInventory.productId = transactionDTO.productId;
                newInventory.warehouseId = transactionDTO.warehouseId;
                newInventory.locationId = transactionDTO.locationId;
                newInventory.quantity = transactionDTO.quantity;
                newInventory.availableQuantity = newInventory.quantity - newInventory.reservedQuantity.value_or(0.0);
                newInventory.unitCost = transactionDTO.unitCost;
                newInventory.createdAt = ERP::Utils::DateUtils::now();
                newInventory.createdBy = currentUserId;
                newInventory.status = ERP::Common::EntityStatus::ACTIVE;
                newInventory.lotNumber = transactionDTO.lotNumber; // Copy from transaction if applicable
                newInventory.serialNumber = transactionDTO.serialNumber;
                
                if (!inventoryDAO_->create(newInventory)) {
                    ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to create new inventory record for adjustment.");
                    return false;
                }
                currentInventory = newInventory;
            } else {
                // Update existing inventory record
                currentInventory = *inventoryOpt;
                double oldQuantity = currentInventory.quantity;

                if (transactionDTO.type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN) {
                    currentInventory.quantity += transactionDTO.quantity;
                    // Update unit cost for ADJ_IN (e.g., by average)
                    if (oldQuantity + transactionDTO.quantity > 0) {
                        currentInventory.unitCost = (oldQuantity * currentInventory.unitCost + transactionDTO.quantity * transactionDTO.unitCost) / (oldQuantity + transactionDTO.quantity);
                    } else {
                        currentInventory.unitCost = transactionDTO.unitCost;
                    }
                } else if (transactionDTO.type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_OUT) {
                    if (currentInventory.quantity < transactionDTO.quantity) {
                        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Insufficient quantity for ADJUSTMENT_OUT. Product " + transactionDTO.productId + ", available: " + std::to_string(currentInventory.quantity) + ", requested: " + std::to_string(transactionDTO.quantity) + ".");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InsufficientStock, "Không đủ số lượng tồn kho để điều chỉnh giảm.");
                        return false;
                    }
                    currentInventory.quantity -= transactionDTO.quantity;
                    // For ADJ_OUT, unitCost typically doesn't change unless it's a specific costing method.
                }
                currentInventory.availableQuantity = currentInventory.quantity - currentInventory.reservedQuantity.value_or(0.0);
                currentInventory.updatedAt = ERP::Utils::DateUtils::now();
                currentInventory.updatedBy = currentUserId;

                if (!inventoryDAO_->update(currentInventory)) {
                    ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to update existing inventory record for adjustment.");
                    return false;
                }
            }

            // Step 3: Record inventory cost layer for adjustments (simplified)
            // For ADJ_IN, create a new layer. For ADJ_OUT, consume from existing layers.
            if (transactionDTO.type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN) {
                ERP::Warehouse::DTO::InventoryCostLayerDTO newCostLayer;
                newCostLayer.id = ERP::Utils::generateUUID();
                newCostLayer.productId = transactionDTO.productId;
                newCostLayer.warehouseId = transactionDTO.warehouseId;
                newCostLayer.locationId = transactionDTO.locationId;
                newCostLayer.receiptDate = transactionDTO.transactionDate;
                newCostLayer.quantity = transactionDTO.quantity;
                newCostLayer.unitCost = transactionDTO.unitCost;
                newCostLayer.remainingQuantity = transactionDTO.quantity;
                newCostLayer.createdAt = ERP::Utils::DateUtils::now();
                newCostLayer.createdBy = currentUserId;
                newCostLayer.status = ERP::Common::EntityStatus::ACTIVE;
                if (!inventoryCostLayerDAO_->create(newCostLayer)) {
                    ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to record cost layer for adjustment in.");
                    return false;
                }
            } else if (transactionDTO.type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_OUT) {
                if (!consumeInventoryCostLayers(transactionDTO.productId, transactionDTO.warehouseId, transactionDTO.locationId, transactionDTO.quantity, currentUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to consume cost layers for adjustment out.");
                    return false;
                }
            }

            // Optionally, publish event for Inventory Level Change
            eventBus_.publish(std::make_shared<EventBus::InventoryLevelChangedEvent>(
                currentInventory.productId, currentInventory.warehouseId, currentInventory.locationId,
                inventoryOpt ? inventoryOpt->quantity : 0.0, currentInventory.quantity, transactionDTO.getTypeString()
            ));
            return true;
        },
        "InventoryManagementService", "adjustInventory"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Inventory adjustment recorded successfully for product " + transactionDTO.productId + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "InventoryAdjustment", currentInventory.id, "Inventory", currentInventory.productId,
                       inventoryOpt ? inventoryOpt->toMap() : std::nullopt, currentInventory.toMap(), "Inventory adjusted by " + std::to_string(transactionDTO.quantity));
        return true;
    }
    return false;
}

bool InventoryManagementService::reserveInventory(
    const std::string& productId,
    const std::string& warehouseId,
    const std::string& locationId,
    double quantityToReserve,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Reserving " + std::to_string(quantityToReserve) + " of product " + productId + " at " + warehouseId + "/" + locationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ReserveInventory", "Bạn không có quyền đặt trước tồn kho.")) {
        return false;
    }
    if (quantityToReserve <= 0) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Quantity to reserve must be positive.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng đặt trước phải là số dương.");
        return false;
    }

    std::optional<ERP::Warehouse::DTO::InventoryDTO> inventoryOpt = getInventoryByProductLocation(productId, warehouseId, locationId, userRoleIds);
    if (!inventoryOpt) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Inventory record not found for product " + productId + " at " + warehouseId + "/" + locationId + " for reservation.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy bản ghi tồn kho để đặt trước.");
        return false;
    }
    ERP::Warehouse::DTO::InventoryDTO currentInventory = *inventoryOpt;

    if (currentInventory.availableQuantity.value_or(0.0) < quantityToReserve) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Insufficient available quantity for reservation. Product " + productId + ", available: " + std::to_string(currentInventory.availableQuantity.value_or(0.0)) + ", requested: " + std::to_string(quantityToReserve) + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InsufficientStock, "Không đủ số lượng tồn kho khả dụng để đặt trước.");
        return false;
    }

    ERP::Warehouse::DTO::InventoryDTO updatedInventory = currentInventory;
    updatedInventory.reservedQuantity = updatedInventory.reservedQuantity.value_or(0.0) + quantityToReserve;
    updatedInventory.availableQuantity = updatedInventory.quantity - updatedInventory.reservedQuantity.value_or(0.0);
    updatedInventory.updatedAt = ERP::Utils::DateUtils::now();
    updatedInventory.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!inventoryDAO_->update(updatedInventory)) {
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to update inventory for reservation.");
                return false;
            }
            return true;
        },
        "InventoryManagementService", "reserveInventory"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Reserved " + std::to_string(quantityToReserve) + " of product " + productId + " successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "InventoryReservation", currentInventory.id, "Inventory", currentInventory.productId,
                       currentInventory.toMap(), updatedInventory.toMap(), "Inventory reserved.");
        return true;
    }
    return false;
}

bool InventoryManagementService::unreserveInventory(
    const std::string& productId,
    const std::string& warehouseId,
    const std::string& locationId,
    double quantityToUnreserve,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Unreserving " + std::to_string(quantityToUnreserve) + " of product " + productId + " at " + warehouseId + "/" + locationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.UnreserveInventory", "Bạn không có quyền hủy đặt trước tồn kho.")) {
        return false;
    }
    if (quantityToUnreserve <= 0) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Quantity to unreserve must be positive.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng hủy đặt trước phải là số dương.");
        return false;
    }

    std::optional<ERP::Warehouse::DTO::InventoryDTO> inventoryOpt = getInventoryByProductLocation(productId, warehouseId, locationId, userRoleIds);
    if (!inventoryOpt) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Inventory record not found for product " + productId + " at " + warehouseId + "/" + locationId + " for unreservation.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy bản ghi tồn kho để hủy đặt trước.");
        return false;
    }
    ERP::Warehouse::DTO::InventoryDTO currentInventory = *inventoryOpt;

    if (currentInventory.reservedQuantity.value_or(0.0) < quantityToUnreserve) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Quantity to unreserve exceeds reserved quantity for product " + productId + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng hủy đặt trước vượt quá số lượng đã đặt trước.");
        return false;
    }

    ERP::Warehouse::DTO::InventoryDTO updatedInventory = currentInventory;
    updatedInventory.reservedQuantity = updatedInventory.reservedQuantity.value_or(0.0) - quantityToUnreserve;
    updatedInventory.availableQuantity = updatedInventory.quantity - updatedInventory.reservedQuantity.value_or(0.0);
    updatedInventory.updatedAt = ERP::Utils::DateUtils::now();
    updatedInventory.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!inventoryDAO_->update(updatedInventory)) {
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to update inventory for unreservation.");
                return false;
            }
            return true;
        },
        "InventoryManagementService", "unreserveInventory"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Unreserved " + std::to_string(quantityToUnreserve) + " of product " + productId + " successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "InventoryUnreservation", currentInventory.id, "Inventory", currentInventory.productId,
                       currentInventory.toMap(), updatedInventory.toMap(), "Inventory unreserved.");
        return true;
    }
    return false;
}

bool InventoryManagementService::transferStock(
    const std::string& productId,
    const std::string& sourceWarehouseId,
    const std::string& sourceLocationId,
    const std::string& destinationWarehouseId,
    const std::string& destinationLocationId,
    double quantity,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Transferring " + std::to_string(quantity) + " of product " + productId + " from " + sourceWarehouseId + "/" + sourceLocationId + " to " + destinationWarehouseId + "/" + destinationLocationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.TransferStock", "Bạn không có quyền chuyển kho.")) {
        return false;
    }
    if (quantity <= 0) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Transfer quantity must be positive.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng chuyển kho phải là số dương.");
        return false;
    }
    if (productId.empty() || sourceWarehouseId.empty() || sourceLocationId.empty() || destinationWarehouseId.empty() || destinationLocationId.empty()) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Missing required IDs for stock transfer.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin chuyển kho không đầy đủ.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Step 1: Record Goods Issue from source
            ERP::Warehouse::DTO::InventoryTransactionDTO issueTxn;
            issueTxn.id = ERP::Utils::generateUUID();
            issueTxn.productId = productId;
            issueTxn.warehouseId = sourceWarehouseId;
            issueTxn.locationId = sourceLocationId;
            issueTxn.type = ERP::Warehouse::DTO::InventoryTransactionType::TRANSFER_OUT;
            issueTxn.quantity = quantity;
            // Get unit cost from source inventory
            std::optional<ERP::Warehouse::DTO::InventoryDTO> sourceInv = getInventoryByProductLocation(productId, sourceWarehouseId, sourceLocationId, userRoleIds);
            issueTxn.unitCost = sourceInv ? sourceInv->unitCost : 0.0;
            issueTxn.transactionDate = ERP::Utils::DateUtils::now();
            issueTxn.notes = "Stock Transfer Out";
            issueTxn.status = ERP::Common::EntityStatus::ACTIVE;
            issueTxn.createdAt = ERP::Utils::DateUtils::now();
            issueTxn.createdBy = currentUserId;

            if (!recordGoodsIssue(issueTxn, currentUserId, userRoleIds)) { // Use existing service method
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to record goods issue for transfer from source.");
                return false;
            }

            // Step 2: Record Goods Receipt at destination
            ERP::Warehouse::DTO::InventoryTransactionDTO receiptTxn;
            receiptTxn.id = ERP::Utils::generateUUID();
            receiptTxn.productId = productId;
            receiptTxn.warehouseId = destinationWarehouseId;
            receiptTxn.locationId = destinationLocationId;
            receiptTxn.type = ERP::Warehouse::DTO::InventoryTransactionType::TRANSFER_IN;
            receiptTxn.quantity = quantity;
            receiptTxn.unitCost = issueTxn.unitCost; // Use same cost as issue
            receiptTxn.transactionDate = ERP::Utils::DateUtils::now();
            receiptTxn.notes = "Stock Transfer In";
            receiptTxn.status = ERP::Common::EntityStatus::ACTIVE;
            receiptTxn.createdAt = ERP::Utils::DateUtils::now();
            receiptTxn.createdBy = currentUserId;

            if (!recordGoodsReceipt(receiptTxn, currentUserId, userRoleIds)) { // Use existing service method
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to record goods receipt for transfer at destination.");
                return false;
            }
            return true;
        },
        "InventoryManagementService", "transferStock"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Stock transfer for product " + productId + " completed successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::PROCESS_END, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "StockTransfer", productId, "Product", productId, // Entity ID and type
                       std::nullopt, std::nullopt, "Transferred " + std::to_string(quantity) + " from " + sourceWarehouseId + "/" + sourceLocationId + " to " + destinationWarehouseId + "/" + destinationLocationId + ".");
        return true;
    }
    return false;
}

bool InventoryManagementService::deleteInventory(
    const std::string& inventoryId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Attempting to delete inventory record: " + inventoryId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.DeleteInventory", "Bạn không có quyền xóa bản ghi tồn kho.")) {
        return false;
    }

    std::optional<ERP::Warehouse::DTO::InventoryDTO> inventoryOpt = inventoryDAO_->getInventoryById(inventoryId);
    if (!inventoryOpt) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Inventory record with ID " + inventoryId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy bản ghi tồn kho cần xóa.");
        return false;
    }

    ERP::Warehouse::DTO::InventoryDTO inventoryToDelete = *inventoryOpt;

    // Prevent deletion if quantity is not zero or if there are reserved quantities
    if (inventoryToDelete.quantity != 0.0 || inventoryToDelete.reservedQuantity.value_or(0.0) != 0.0) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Cannot delete inventory record " + inventoryId + " with non-zero quantity or reserved quantity.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa bản ghi tồn kho có số lượng khác không hoặc đã đặt trước.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated cost layers (if any remaining, though should be 0 quantity)
            std::map<std::string, std::any> costLayerFilter;
            costLayerFilter["product_id"] = inventoryToDelete.productId;
            costLayerFilter["warehouse_id"] = inventoryToDelete.warehouseId;
            costLayerFilter["location_id"] = inventoryToDelete.locationId;
            // Only remove if remaining_quantity is 0
            costLayerFilter["remaining_quantity"] = 0.0; 
            // if (!inventoryCostLayerDAO_->remove(costLayerFilter)) { // Need a specialized remove for filters or iterate and remove
            //     ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to remove associated cost layers for inventory " + inventoryId + ".");
            //     return false;
            // }

            if (!inventoryDAO_->remove(inventoryId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to delete inventory record " + inventoryId + " in DAO.");
                return false;
            }
            return true;
        },
        "InventoryManagementService", "deleteInventory"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Inventory record " + inventoryId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "Inventory", inventoryId, "Inventory", inventoryToDelete.productId,
                       inventoryToDelete.toMap(), std::nullopt, "Inventory record deleted.");
        return true;
    }
    return false;
}

std::optional<std::string> InventoryManagementService::getDefaultLocationForWarehouse(
    const std::string& warehouseId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("InventoryManagementService: Getting default location for warehouse: " + warehouseId + ".");
    
    // This is a common helper, might not need explicit permission check if used internally by other permitted ops.
    // If exposed via UI, "Catalog.ViewLocations" might be needed.

    // Get default location based on conventions (e.g., location named "General" or first active location)
    std::map<std::string, std::any> filters;
    filters["warehouse_id"] = warehouseId;
    filters["name"] = "Khu vực chung"; // Assuming a "General" or default location name

    std::vector<ERP::Catalog::DTO::LocationDTO> locations = warehouseService_->getLocations(filters, userRoleIds); // Use warehouseService
    if (!locations.empty()) {
        return locations[0].id;
    }
    // Fallback: get any active location in the warehouse
    locations = warehouseService_->getLocationsByWarehouse(warehouseId, userRoleIds);
    if (!locations.empty()) {
        return locations[0].id;
    }

    ERP::Logger::Logger::getInstance().warning("InventoryManagementService: No default location found for warehouse " + warehouseId + ".");
    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy vị trí mặc định cho kho hàng.");
    return std::nullopt;
}

// NEW: Inventory Cost Layer Management Implementations

bool InventoryManagementService::recordInventoryCostLayer(
    const ERP::Warehouse::DTO::InventoryCostLayerDTO& costLayerDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Recording inventory cost layer for product " + costLayerDTO.productId + ", quantity " + std::to_string(costLayerDTO.quantity) + ", cost " + std::to_string(costLayerDTO.unitCost) + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.RecordInventoryCostLayer", "Bạn không có quyền ghi nhận lớp chi phí tồn kho.")) {
        return false;
    }

    // Validate DTO (product, warehouse, location existence)
    if (costLayerDTO.productId.empty() || costLayerDTO.warehouseId.empty() || costLayerDTO.locationId.empty() || costLayerDTO.quantity <= 0 || costLayerDTO.unitCost < 0) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Invalid input for cost layer recording.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin lớp chi phí tồn kho không hợp lệ.");
        return false;
    }

    ERP::Warehouse::DTO::InventoryCostLayerDTO newCostLayer = costLayerDTO;
    newCostLayer.id = ERP::Utils::generateUUID();
    newCostLayer.createdAt = ERP::Utils::DateUtils::now();
    newCostLayer.createdBy = currentUserId;
    newCostLayer.status = ERP::Common::EntityStatus::ACTIVE; // Active layer
    newCostLayer.remainingQuantity = newCostLayer.quantity; // Initially, remaining equals quantity

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!inventoryCostLayerDAO_->create(newCostLayer)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to create inventory cost layer in DAO.");
                return false;
            }
            return true;
        },
        "InventoryManagementService", "recordInventoryCostLayer"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Inventory cost layer " + newCostLayer.id + " recorded successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "InventoryCostLayer", newCostLayer.id, "InventoryCostLayer", newCostLayer.productId,
                       std::nullopt, newCostLayer.toMap(), "Inventory cost layer recorded.");
        return true;
    }
    return false;
}

bool InventoryManagementService::consumeInventoryCostLayers(
    const std::string& productId,
    const std::string& warehouseId,
    const std::string& locationId,
    double quantityToConsume,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("InventoryManagementService: Consuming " + std::to_string(quantityToConsume) + " from cost layers for product " + productId + " at " + warehouseId + "/" + locationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ConsumeInventoryCostLayers", "Bạn không có quyền tiêu thụ lớp chi phí tồn kho.")) {
        return false;
    }
    if (quantityToConsume <= 0) {
        ERP::Logger::Logger::getInstance().warning("InventoryManagementService: Quantity to consume must be positive.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng tiêu thụ phải là số dương.");
        return false;
    }

    std::map<std::string, std::any> filters;
    filters["product_id"] = productId;
    filters["warehouse_id"] = warehouseId;
    filters["location_id"] = locationId;
    filters["remaining_quantity_gt"] = 0.0; // Only get layers with remaining quantity
    // Order by receipt_date ASC for FIFO, or DESC for LIFO
    std::vector<ERP::Warehouse::DTO::InventoryCostLayerDTO> activeLayers = inventoryCostLayerDAO_->getCostLayers(filters); // Assuming getCostLayers supports ordering

    double remainingToConsume = quantityToConsume;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            for (auto& layer : activeLayers) {
                if (remainingToConsume <= 0) break; // All consumed
                
                double consumeFromThisLayer = std::min(remainingToConsume, layer.remainingQuantity);
                
                ERP::Warehouse::DTO::InventoryCostLayerDTO updatedLayer = layer;
                updatedLayer.remainingQuantity -= consumeFromThisLayer;
                updatedLayer.updatedAt = ERP::Utils::DateUtils::now();
                updatedLayer.updatedBy = currentUserId;

                if (!inventoryCostLayerDAO_->update(updatedLayer)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("InventoryManagementService: Failed to update cost layer " + layer.id + " during consumption.");
                    return false;
                }
                remainingToConsume -= consumeFromThisLayer;
            }

            if (remainingToConsume > 0) {
                ERP::Logger::Logger::getInstance().error("InventoryManagementService: Not enough quantity in cost layers to consume. Remaining: " + std::to_string(remainingToConsume));
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InsufficientStock, "Không đủ số lượng trong lớp chi phí tồn kho để tiêu thụ.");
                return false;
            }
            return true;
        },
        "InventoryManagementService", "consumeInventoryCostLayers"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("InventoryManagementService: Consumed " + std::to_string(quantityToConsume) + " from cost layers for product " + productId + " successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Warehouse", "InventoryCostLayerConsumption", productId, "Product", productId,
                       std::nullopt, std::nullopt, "Consumed quantity from cost layers."); // Before/after data could be more specific
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Warehouse
} // namespace ERP