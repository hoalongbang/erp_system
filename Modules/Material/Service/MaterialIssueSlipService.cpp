// Modules/Material/Service/MaterialIssueSlipService.cpp
#include "MaterialIssueSlipService.h" // Đã rút gọn include
#include "MaterialIssueSlip.h" // Đã rút gọn include
#include "MaterialIssueSlipDetail.h" // Đã rút gọn include
#include "InventoryTransaction.h" // For creating inventory transactions
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ProductionOrderService.h" // Đã rút gọn include
#include "ProductService.h" // Đã rút gọn include
#include "WarehouseService.h" // Đã rút gọn include
#include "InventoryManagementService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Material {
namespace Services {

MaterialIssueSlipService::MaterialIssueSlipService(
    std::shared_ptr<DAOs::MaterialIssueSlipDAO> materialIssueSlipDAO,
    std::shared_ptr<ERP::Manufacturing::Services::IProductionOrderService> productionOrderService,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      materialIssueSlipDAO_(materialIssueSlipDAO), productionOrderService_(productionOrderService),
      productService_(productService), warehouseService_(warehouseService),
      inventoryManagementService_(inventoryManagementService) {
    if (!materialIssueSlipDAO_ || !productionOrderService_ || !productService_ || !warehouseService_ || !inventoryManagementService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "MaterialIssueSlipService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ phiếu xuất vật tư sản xuất.");
        ERP::Logger::Logger::getInstance().critical("MaterialIssueSlipService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("MaterialIssueSlipService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> MaterialIssueSlipService::createMaterialIssueSlip(
    const ERP::Material::DTO::MaterialIssueSlipDTO& materialIssueSlipDTO,
    const std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO>& materialIssueSlipDetails,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Attempting to create material issue slip: " + materialIssueSlipDTO.issueNumber + " for production order: " + materialIssueSlipDTO.productionOrderId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.CreateMaterialIssueSlip", "Bạn không có quyền tạo phiếu xuất vật tư sản xuất.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (materialIssueSlipDTO.issueNumber.empty() || materialIssueSlipDTO.productionOrderId.empty() || materialIssueSlipDTO.warehouseId.empty() || materialIssueSlipDetails.empty()) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Invalid input for material issue slip creation (empty number, production order, warehouse, or no details).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "MaterialIssueSlipService: Invalid input for material issue slip creation.", "Thông tin phiếu xuất vật tư sản xuất không đầy đủ.");
        return std::nullopt;
    }

    // Check if issue number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["issue_number"] = materialIssueSlipDTO.issueNumber;
    if (materialIssueSlipDAO_->count(filterByNumber) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Material issue slip with number " + materialIssueSlipDTO.issueNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "MaterialIssueSlipService: Material issue slip with number " + materialIssueSlipDTO.issueNumber + " already exists.", "Số phiếu xuất vật tư sản xuất đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate Production Order existence
    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> productionOrder = productionOrderService_->getProductionOrderById(materialIssueSlipDTO.productionOrderId, userRoleIds);
    if (!productionOrder || (productionOrder->status != ERP::Manufacturing::DTO::ProductionOrderStatus::IN_PROGRESS && productionOrder->status != ERP::Manufacturing::DTO::ProductionOrderStatus::RELEASED)) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Invalid Production Order ID provided or order not in progress/released: " + materialIssueSlipDTO.productionOrderId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Lệnh sản xuất không hợp lệ hoặc không ở trạng thái đang thực hiện/đã phát hành.");
        return std::nullopt;
    }

    // Validate Warehouse existence
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(materialIssueSlipDTO.warehouseId, userRoleIds);
    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Invalid Warehouse ID provided or warehouse is not active: " + materialIssueSlipDTO.warehouseId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
        return std::nullopt;
    }

    // Validate details and check stock
    for (const auto& detail : materialIssueSlipDetails) {
        if (!productService_->getProductById(detail.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Detail product " + detail.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        if (detail.issuedQuantity <= 0) {
            ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Detail product " + detail.productId + " has non-positive issued quantity.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng xuất trong chi tiết phải lớn hơn 0.");
            return std::nullopt;
        }
        // Check current stock availability
        std::optional<ERP::Warehouse::DTO::InventoryDTO> inventory = inventoryManagementService_->getInventoryByProductLocation(detail.productId, materialIssueSlipDTO.warehouseId, "N/A", userRoleIds); // Location is not in detail, assume it's implicit or needs to be passed.
        // Assuming location is not directly part of MaterialIssueSlipDetail for simplicity of checking stock,
        // but in reality, you'd need a location.
        if (!inventory || inventory->quantity < detail.issuedQuantity) {
            ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Insufficient stock for product " + detail.productId + " at warehouse " + materialIssueSlipDTO.warehouseId + ". Available: " + std::to_string(inventory.value_or(ERP::Warehouse::DTO::InventoryDTO()).quantity) + ", Requested: " + std::to_string(detail.issuedQuantity) + ".");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InsufficientStock, "Không đủ tồn kho cho sản phẩm " + detail.productId + " tại kho " + materialIssueSlipDTO.warehouseId + ".");
            return std::nullopt;
        }
    }

    ERP::Material::DTO::MaterialIssueSlipDTO newMaterialIssueSlip = materialIssueSlipDTO;
    newMaterialIssueSlip.id = ERP::Utils::generateUUID();
    newMaterialIssueSlip.createdAt = ERP::Utils::DateUtils::now();
    newMaterialIssueSlip.createdBy = currentUserId;
    newMaterialIssueSlip.status = ERP::Material::DTO::MaterialIssueSlipStatus::DRAFT; // Default status

    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> createdMaterialIssueSlip = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!materialIssueSlipDAO_->create(newMaterialIssueSlip)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to create material issue slip " + newMaterialIssueSlip.issueNumber + " in DAO.");
                return false;
            }
            // Create Material Issue Slip details
            for (auto detail : materialIssueSlipDetails) {
                detail.id = ERP::Utils::generateUUID();
                detail.materialIssueSlipId = newMaterialIssueSlip.id;
                detail.createdAt = newMaterialIssueSlip.createdAt;
                detail.createdBy = newMaterialIssueSlip.createdBy;
                detail.status = ERP::Common::EntityStatus::ACTIVE; // Assuming detail has status
                
                if (!materialIssueSlipDAO_->createMaterialIssueSlipDetail(detail)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to create material issue slip detail for product " + detail.productId + ".");
                    return false;
                }
                // Record goods issue transaction in Inventory
                // This assumes `recordGoodsIssue` handles its own permissions and transaction commit, or `db_conn` is passed down.
                // The current `recordGoodsIssue` takes `currentUserId`, `userRoleIds`.
                // It does its own transaction, which might conflict with this parent transaction.
                // It should be refactored to take an optional `db_conn` to join an existing transaction.
                // For now, I'll pass it without `db_conn`, meaning it will create its own sub-transaction.
                // This is not ideal for atomicity across services.
                ERP::Warehouse::DTO::InventoryTransactionDTO issueTransaction;
                issueTransaction.id = ERP::Utils::generateUUID();
                issueTransaction.productId = detail.productId;
                issueTransaction.warehouseId = newMaterialIssueSlip.warehouseId;
                issueTransaction.locationId = "N/A"; // Needs real location if exists
                issueTransaction.type = ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE;
                issueTransaction.quantity = detail.issuedQuantity;
                issueTransaction.unitCost = 0.0; // Needs to be determined from cost layers
                issueTransaction.transactionDate = ERP::Utils::DateUtils::now();
                issueTransaction.lotNumber = detail.lotNumber;
                issueTransaction.serialNumber = detail.serialNumber;
                issueTransaction.referenceDocumentId = newMaterialIssueSlip.id;
                issueTransaction.referenceDocumentType = "MaterialIssueSlip";
                issueTransaction.notes = "Issued via Material Issue Slip " + newMaterialIssueSlip.issueNumber;
                issueTransaction.createdAt = newMaterialIssueSlip.createdAt;
                issueTransaction.createdBy = newMaterialIssueSlip.createdBy;
                issueTransaction.status = ERP::Common::EntityStatus::ACTIVE;

                if (!inventoryManagementService_->recordGoodsIssue(issueTransaction, currentUserId, userRoleIds)) {
                     ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to record goods issue for product " + detail.productId + " via inventory service.");
                     return false;
                }
            }
            createdMaterialIssueSlip = newMaterialIssueSlip;
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::MaterialIssueSlipCreatedEvent>(newMaterialIssueSlip));
            return true;
        },
        "MaterialIssueSlipService", "createMaterialIssueSlip"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Material issue slip " + newMaterialIssueSlip.issueNumber + " created successfully with " + std::to_string(materialIssueSlipDetails.size()) + " details.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Material", "MaterialIssueSlip", newMaterialIssueSlip.id, "MaterialIssueSlip", newMaterialIssueSlip.issueNumber,
                       std::nullopt, newMaterialIssueSlip.toMap(), "Material issue slip created.");
        return createdMaterialIssueSlip;
    }
    return std::nullopt;
}

std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> MaterialIssueSlipService::getMaterialIssueSlipById(
    const std::string& issueSlipId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("MaterialIssueSlipService: Retrieving material issue slip by ID: " + issueSlipId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewMaterialIssueSlips", "Bạn không có quyền xem phiếu xuất vật tư sản xuất.")) {
        return std::nullopt;
    }

    return materialIssueSlipDAO_->getById(issueSlipId); // Using getById from DAOBase template
}

std::vector<ERP::Material::DTO::MaterialIssueSlipDTO> MaterialIssueSlipService::getAllMaterialIssueSlips(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Retrieving all material issue slips with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewMaterialIssueSlips", "Bạn không có quyền xem tất cả phiếu xuất vật tư sản xuất.")) {
        return {};
    }

    return materialIssueSlipDAO_->get(filter); // Using get from DAOBase template
}

std::vector<ERP::Material::DTO::MaterialIssueSlipDTO> MaterialIssueSlipService::getMaterialIssueSlipsByProductionOrderId(
    const std::string& productionOrderId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Retrieving material issue slips for production order ID: " + productionOrderId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewMaterialIssueSlips", "Bạn không có quyền xem phiếu xuất vật tư sản xuất của lệnh sản xuất này.")) {
        return {};
    }

    std::map<std::string, std::any> filter;
    filter["production_order_id"] = productionOrderId;
    return materialIssueSlipDAO_->get(filter); // Using get from DAOBase template
}


bool MaterialIssueSlipService::updateMaterialIssueSlip(
    const ERP::Material::DTO::MaterialIssueSlipDTO& materialIssueSlipDTO,
    const std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO>& materialIssueSlipDetails,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Attempting to update material issue slip: " + materialIssueSlipDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.UpdateMaterialIssueSlip", "Bạn không có quyền cập nhật phiếu xuất vật tư sản xuất.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> oldMaterialIssueSlipOpt = materialIssueSlipDAO_->getById(materialIssueSlipDTO.id); // Using getById from DAOBase
    if (!oldMaterialIssueSlipOpt) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Material issue slip with ID " + materialIssueSlipDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu xuất vật tư sản xuất cần cập nhật.");
        return false;
    }

    // If issue number is changed, check for uniqueness
    if (materialIssueSlipDTO.issueNumber != oldMaterialIssueSlipOpt->issueNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["issue_number"] = materialIssueSlipDTO.issueNumber;
        if (materialIssueSlipDAO_->count(filterByNumber) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: New issue number " + materialIssueSlipDTO.issueNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "MaterialIssueSlipService: New issue number " + materialIssueSlipDTO.issueNumber + " already exists.", "Số phiếu xuất vật tư sản xuất mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate Production Order existence
    if (materialIssueSlipDTO.productionOrderId != oldMaterialIssueSlipOpt->productionOrderId) { // Only check if changed
        std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> productionOrder = productionOrderService_->getProductionOrderById(materialIssueSlipDTO.productionOrderId, userRoleIds);
        if (!productionOrder || (productionOrder->status != ERP::Manufacturing::DTO::ProductionOrderStatus::IN_PROGRESS && productionOrder->status != ERP::Manufacturing::DTO::ProductionOrderStatus::RELEASED)) {
            ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Invalid Production Order ID provided or order not in progress/released for update: " + materialIssueSlipDTO.productionOrderId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Lệnh sản xuất không hợp lệ hoặc không ở trạng thái đang thực hiện/đã phát hành.");
            return std::nullopt;
        }
    }
    // Validate Warehouse existence
    if (materialIssueSlipDTO.warehouseId != oldMaterialIssueSlipOpt->warehouseId) { // Only check if changed
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(materialIssueSlipDTO.warehouseId, userRoleIds);
        if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
            ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Invalid Warehouse ID provided or warehouse is not active for update: " + materialIssueSlipDTO.warehouseId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
            return std::nullopt;
        }
    }
    // Validate details (similar to create)
    for (const auto& detail : materialIssueSlipDetails) {
        if (!productService_->getProductById(detail.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Detail product " + detail.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        if (detail.issuedQuantity <= 0) {
            ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Detail product " + detail.productId + " has non-positive issued quantity.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng xuất trong chi tiết phải lớn hơn 0.");
            return std::nullopt;
        }
        // Stock check is typically done during recordIssuedQuantity, not during slip update
    }

    ERP::Material::DTO::MaterialIssueSlipDTO updatedMaterialIssueSlip = materialIssueSlipDTO;
    updatedMaterialIssueSlip.updatedAt = ERP::Utils::DateUtils::now();
    updatedMaterialIssueSlip.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!materialIssueSlipDAO_->update(updatedMaterialIssueSlip)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to update material issue slip " + updatedMaterialIssueSlip.id + " in DAO.");
                return false;
            }
            // Update details (remove all old, add new ones - simpler but less efficient for large lists)
            if (!materialIssueSlipDAO_->removeMaterialIssueSlipDetailsByIssueSlipId(updatedMaterialIssueSlip.id)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to remove old material issue slip details for slip " + updatedMaterialIssueSlip.id + ".");
                return false;
            }
            for (auto detail : materialIssueSlipDetails) {
                detail.id = ERP::Utils::generateUUID(); // New UUID for new details
                detail.materialIssueSlipId = updatedMaterialIssueSlip.id;
                detail.createdAt = updatedMaterialIssueSlip.createdAt; // Use parent creation info
                detail.createdBy = updatedMaterialIssueSlip.createdBy;
                detail.status = ERP::Common::EntityStatus::ACTIVE;
                
                if (!materialIssueSlipDAO_->createMaterialIssueSlipDetail(detail)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to create new material issue slip detail for product " + detail.productId + " for slip " + updatedMaterialIssueSlip.id + ".");
                    return false;
                }
                // When details are replaced, any corresponding inventory transactions should also be reversed/re-created
                // This is complex and usually requires careful design (e.g., allow only 'adding' details, or specific 'update quantity' ops)
                // For now, only the metadata update is handled.
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::MaterialIssueSlipUpdatedEvent>(updatedMaterialIssueSlip));
            return true;
        },
        "MaterialIssueSlipService", "updateMaterialIssueSlip"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Material issue slip " + updatedMaterialIssueSlip.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Material", "MaterialIssueSlip", updatedMaterialIssueSlip.id, "MaterialIssueSlip", updatedMaterialIssueSlip.issueNumber,
                       oldMaterialIssueSlipOpt->toMap(), updatedMaterialIssueSlip.toMap(), "Material issue slip updated.");
        return true;
    }
    return false;
}

bool MaterialIssueSlipService::updateMaterialIssueSlipStatus(
    const std::string& issueSlipId,
    ERP::Material::DTO::MaterialIssueSlipStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Attempting to update status for material issue slip: " + issueSlipId + " to " + ERP::Material::DTO::MaterialIssueSlipDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.UpdateMaterialIssueSlipStatus", "Bạn không có quyền cập nhật trạng thái phiếu xuất vật tư sản xuất.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> issueSlipOpt = materialIssueSlipDAO_->getById(issueSlipId); // Using getById from DAOBase
    if (!issueSlipOpt) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Material issue slip with ID " + issueSlipId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu xuất vật tư sản xuất để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Material::DTO::MaterialIssueSlipDTO oldIssueSlip = *issueSlipOpt;
    if (oldIssueSlip.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Material issue slip " + issueSlipId + " is already in status " + oldIssueSlip.getStatusString() + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from COMPLETED to DRAFT.

    ERP::Material::DTO::MaterialIssueSlipDTO updatedIssueSlip = oldIssueSlip;
    updatedIssueSlip.status = newStatus;
    updatedIssueSlip.updatedAt = ERP::Utils::DateUtils::now();
    updatedIssueSlip.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!materialIssueSlipDAO_->update(updatedIssueSlip)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to update status for material issue slip " + issueSlipId + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::MaterialIssueSlipStatusChangedEvent>(issueSlipId, newStatus));
            return true;
        },
        "MaterialIssueSlipService", "updateMaterialIssueSlipStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Status for material issue slip " + issueSlipId + " updated successfully to " + updatedIssueSlip.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Material", "MaterialIssueSlipStatus", issueSlipId, "MaterialIssueSlip", oldIssueSlip.issueNumber,
                       oldIssueSlip.toMap(), updatedIssueSlip.toMap(), "Material issue slip status changed to " + updatedIssueSlip.getStatusString() + ".");
        return true;
    }
    return false;
}

bool MaterialIssueSlipService::deleteMaterialIssueSlip(
    const std::string& issueSlipId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Attempting to delete material issue slip: " + issueSlipId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.DeleteMaterialIssueSlip", "Bạn không có quyền xóa phiếu xuất vật tư sản xuất.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> issueSlipOpt = materialIssueSlipDAO_->getById(issueSlipId); // Using getById from DAOBase
    if (!issueSlipOpt) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Material issue slip with ID " + issueSlipId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu xuất vật tư sản xuất cần xóa.");
        return false;
    }

    ERP::Material::DTO::MaterialIssueSlipDTO issueSlipToDelete = *issueSlipOpt;

    // Additional checks: Prevent deletion if issue slip is already completed or has associated inventory transactions
    if (issueSlipToDelete.status == ERP::Material::DTO::MaterialIssueSlipStatus::ISSUED ||
        issueSlipToDelete.status == ERP::Material::DTO::MaterialIssueSlipStatus::COMPLETED) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Cannot delete material issue slip " + issueSlipId + " as it has already issued materials or is completed.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa phiếu xuất vật tư đã xuất kho hoặc đã hoàn thành.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated details first (and potentially reverse inventory transactions if any were made)
            if (!materialIssueSlipDAO_->removeMaterialIssueSlipDetailsByIssueSlipId(issueSlipId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to remove associated material issue slip details for slip " + issueSlipId + ".");
                return false;
            }
            if (!materialIssueSlipDAO_->remove(issueSlipId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to delete material issue slip " + issueSlipId + " in DAO.");
                return false;
            }
            return true;
        },
        "MaterialIssueSlipService", "deleteMaterialIssueSlip"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Material issue slip " + issueSlipId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Material", "MaterialIssueSlip", issueSlipId, "MaterialIssueSlip", issueSlipToDelete.issueNumber,
                       issueSlipToDelete.toMap(), std::nullopt, "Material issue slip deleted.");
        return true;
    }
    return false;
}

std::optional<ERP::Material::DTO::MaterialIssueSlipDetailDTO> MaterialIssueSlipService::getMaterialIssueSlipDetailById(
    const std::string& detailId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("MaterialIssueSlipService: Retrieving material issue slip detail by ID: " + detailId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewMaterialIssueSlips", "Bạn không có quyền xem chi tiết phiếu xuất vật tư sản xuất.")) {
        return std::nullopt;
    }

    return materialIssueSlipDAO_->getMaterialIssueSlipDetailById(detailId); // Specific DAO method
}

std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> MaterialIssueSlipService::getMaterialIssueSlipDetails(
    const std::string& issueSlipId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Retrieving material issue slip details for slip ID: " + issueSlipId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewMaterialIssueSlips", "Bạn không có quyền xem chi tiết phiếu xuất vật tư sản xuất này.")) {
        return {};
    }

    // Check if parent Issue Slip exists
    if (!materialIssueSlipDAO_->getById(issueSlipId)) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Parent Material Issue Slip " + issueSlipId + " not found when getting details.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Phiếu xuất vật tư sản xuất cha không tồn tại.");
        return {};
    }

    return materialIssueSlipDAO_->getMaterialIssueSlipDetailsByIssueSlipId(issueSlipId); // Specific DAO method
}

bool MaterialIssueSlipService::recordIssuedQuantity(
    const std::string& detailId,
    double issuedQuantity,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Attempting to record issued quantity for detail: " + detailId + " with quantity: " + std::to_string(issuedQuantity) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.RecordMaterialIssueQuantity", "Bạn không có quyền ghi nhận số lượng xuất vật tư sản xuất.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::MaterialIssueSlipDetailDTO> detailOpt = materialIssueSlipDAO_->getMaterialIssueSlipDetailById(detailId); // Specific DAO method
    if (!detailOpt) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Material issue slip detail with ID " + detailId + " not found for recording issued quantity.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy chi tiết phiếu xuất vật tư sản xuất để ghi nhận số lượng.");
        return false;
    }
    
    ERP::Material::DTO::MaterialIssueSlipDetailDTO oldDetail = *detailOpt;

    // Validate new issued quantity
    if (issuedQuantity < 0 || issuedQuantity < oldDetail.issuedQuantity) { // Cannot decrease issued quantity
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipService: Invalid issued quantity for detail " + detailId + ": " + std::to_string(issuedQuantity));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng xuất không hợp lệ.");
        return false;
    }
    // No `requestedQuantity` in MaterialIssueSlipDetailDTO, so cannot validate against it.
    // This implies all `issuedQuantity` are actual. If there's a target, it needs to be added to DTO.

    ERP::Material::DTO::MaterialIssueSlipDetailDTO updatedDetail = oldDetail;
    updatedDetail.issuedQuantity = issuedQuantity;
    updatedDetail.updatedAt = ERP::Utils::DateUtils::now();
    updatedDetail.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Check current stock availability and record goods issue for the incremental quantity
            double quantityToIssue = issuedQuantity - oldDetail.issuedQuantity; // Only the new incremental issue
            if (quantityToIssue > 0) {
                // Need parent slip's warehouse and production order ID
                std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> parentSlipOpt = materialIssueSlipDAO_->getById(oldDetail.materialIssueSlipId);
                if (!parentSlipOpt) {
                    ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Parent Material Issue Slip " + oldDetail.materialIssueSlipId + " not found for detail " + detailId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu xuất vật tư sản xuất cha.");
                    return false;
                }
                std::string actualWarehouseId = parentSlipOpt->warehouseId;

                ERP::Warehouse::DTO::InventoryTransactionDTO issueTransaction;
                issueTransaction.id = ERP::Utils::generateUUID();
                issueTransaction.productId = updatedDetail.productId;
                issueTransaction.warehouseId = actualWarehouseId;
                issueTransaction.locationId = "N/A"; // Needs real location if exists
                issueTransaction.type = ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE;
                issueTransaction.quantity = quantityToIssue; // Use incremental quantity
                issueTransaction.unitCost = 0.0; // Needs to be determined from cost layers
                issueTransaction.transactionDate = ERP::Utils::DateUtils::now();
                issueTransaction.lotNumber = updatedDetail.lotNumber;
                issueTransaction.serialNumber = updatedDetail.serialNumber;
                issueTransaction.referenceDocumentId = parentSlipOpt->id;
                issueTransaction.referenceDocumentType = "MaterialIssueSlip";
                issueTransaction.notes = "Issued via Material Issue Slip " + parentSlipOpt->issueNumber;
                issueTransaction.createdAt = parentSlipOpt->createdAt;
                issueTransaction.createdBy = parentSlipOpt->createdBy;
                issueTransaction.status = ERP::Common::EntityStatus::ACTIVE;

                if (!inventoryManagementService_->recordGoodsIssue(issueTransaction, currentUserId, userRoleIds)) {
                     ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to record goods issue for product " + updatedDetail.productId + " via inventory service.");
                     return false;
                }
                updatedDetail.inventoryTransactionId = issueTransaction.id; // Link to the created transaction
            }

            if (!materialIssueSlipDAO_->updateMaterialIssueSlipDetail(updatedDetail)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to update issued quantity for detail " + detailId + " in DAO.");
                return false;
            }

            // Check if parent Material Issue Slip is fully completed (if there's a concept of target quantity)
            // Assuming completion means all details have issued quantity recorded.
            std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> allDetails = materialIssueSlipDAO_->getMaterialIssueSlipDetailsByIssueSlipId(oldDetail.materialIssueSlipId);
            bool allIssued = std::all_of(allDetails.begin(), allDetails.end(), [](const ERP::Material::DTO::MaterialIssueSlipDetailDTO& d) {
                // If MaterialIssueSlipDetailDTO had a 'requestedQuantity' field, this would be:
                // return d.issuedQuantity >= d.requestedQuantity;
                return d.issuedQuantity > 0; // For now, assume any non-zero quantity means 'issued'
            });

            std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> parentSlip = materialIssueSlipDAO_->getById(oldDetail.materialIssueSlipId);
            if (parentSlip) {
                if (allIssued && parentSlip->status != ERP::Material::DTO::MaterialIssueSlipStatus::COMPLETED) {
                    if (!updateMaterialIssueSlipStatus(parentSlip->id, ERP::Material::DTO::MaterialIssueSlipStatus::COMPLETED, currentUserId, userRoleIds)) {
                        ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to auto-update parent Material Issue Slip status to COMPLETED.");
                    }
                } else if (!allIssued && parentSlip->status == ERP::Material::DTO::MaterialIssueSlipStatus::COMPLETED) {
                    // Revert status if not fully issued anymore (e.g., if quantity was reduced or new details added)
                    if (!updateMaterialIssueSlipStatus(parentSlip->id, ERP::Material::DTO::MaterialIssueSlipStatus::ISSUED, currentUserId, userRoleIds)) {
                        ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to auto-update parent Material Issue Slip status to ISSUED.");
                    }
                } else if (parentSlip->status == ERP::Material::DTO::MaterialIssueSlipStatus::DRAFT || parentSlip->status == ERP::Material::DTO::MaterialIssueSlipStatus::PENDING_APPROVAL) {
                    if (!updateMaterialIssueSlipStatus(parentSlip->id, ERP::Material::DTO::MaterialIssueSlipStatus::ISSUED, currentUserId, userRoleIds)) {
                        ERP::Logger::Logger::getInstance().error("MaterialIssueSlipService: Failed to auto-update parent Material Issue Slip status to ISSUED from DRAFT/PENDING.");
                    }
                }
            }


            return true;
        },
        "MaterialIssueSlipService", "recordIssuedQuantity"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("MaterialIssueSlipService: Issued quantity recorded successfully for detail: " + detailId);
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Material", "MaterialIssueSlipDetail", detailId, "MaterialIssueSlipDetail", updatedDetail.productId,
                       oldDetail.toMap(), updatedDetail.toMap(), "Issued quantity recorded: " + std::to_string(issuedQuantity) + ".");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Material
} // namespace ERP