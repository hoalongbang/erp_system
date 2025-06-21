// Modules/Material/Service/ReceiptSlipService.cpp
#include "ReceiptSlipService.h" // Đã rút gọn include
#include "ReceiptSlip.h" // Đã rút gọn include
#include "ReceiptSlipDetail.h" // Đã rút gọn include
#include "InventoryTransaction.h" // For creating inventory transactions
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
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

ReceiptSlipService::ReceiptSlipService(
    std::shared_ptr<DAOs::ReceiptSlipDAO> receiptSlipDAO,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      receiptSlipDAO_(receiptSlipDAO), productService_(productService),
      warehouseService_(warehouseService), inventoryManagementService_(inventoryManagementService) {
    if (!receiptSlipDAO_ || !productService_ || !warehouseService_ || !inventoryManagementService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ReceiptSlipService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ phiếu nhập kho.");
        ERP::Logger::Logger::getInstance().critical("ReceiptSlipService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("ReceiptSlipService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Material::DTO::ReceiptSlipDTO> ReceiptSlipService::createReceiptSlip(
    const ERP::Material::DTO::ReceiptSlipDTO& receiptSlipDTO,
    const std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO>& receiptSlipDetails,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Attempting to create receipt slip: " + receiptSlipDTO.receiptNumber + " for warehouse: " + receiptSlipDTO.warehouseId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.CreateReceiptSlip", "Bạn không có quyền tạo phiếu nhập kho.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (receiptSlipDTO.receiptNumber.empty() || receiptSlipDTO.warehouseId.empty() || receiptSlipDetails.empty()) {
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Invalid input for receipt slip creation (empty number, warehouse, or no details).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ReceiptSlipService: Invalid input for receipt slip creation.", "Thông tin phiếu nhập kho không đầy đủ.");
        return std::nullopt;
    }

    // Check if receipt number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["receipt_number"] = receiptSlipDTO.receiptNumber;
    if (receiptSlipDAO_->count(filterByNumber) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Receipt slip with number " + receiptSlipDTO.receiptNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ReceiptSlipService: Receipt slip with number " + receiptSlipDTO.receiptNumber + " already exists.", "Số phiếu nhập kho đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate Warehouse existence
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(receiptSlipDTO.warehouseId, userRoleIds);
    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Invalid Warehouse ID provided or warehouse is not active: " + receiptSlipDTO.warehouseId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
        return std::nullopt;
    }

    // Validate details
    for (const auto& detail : receiptSlipDetails) {
        if (!productService_->getProductById(detail.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Detail product " + detail.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        if (!warehouseService_->getLocationById(detail.locationId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Detail location " + detail.locationId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Vị trí trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        if (detail.expectedQuantity <= 0) {
            ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Detail product " + detail.productId + " has non-positive expected quantity.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng dự kiến trong chi tiết phải lớn hơn 0.");
            return std::nullopt;
        }
    }

    ERP::Material::DTO::ReceiptSlipDTO newReceiptSlip = receiptSlipDTO;
    newReceiptSlip.id = ERP::Utils::generateUUID();
    newReceiptSlip.createdAt = ERP::Utils::DateUtils::now();
    newReceiptSlip.createdBy = currentUserId;
    newReceiptSlip.status = ERP::Material::DTO::ReceiptSlipStatus::DRAFT; // Default status

    std::optional<ERP::Material::DTO::ReceiptSlipDTO> createdReceiptSlip = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!receiptSlipDAO_->create(newReceiptSlip)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to create receipt slip " + newReceiptSlip.receiptNumber + " in DAO.");
                return false;
            }
            // Create Receipt Slip details
            for (auto detail : receiptSlipDetails) {
                detail.id = ERP::Utils::generateUUID();
                detail.receiptSlipId = newReceiptSlip.id;
                detail.createdAt = newReceiptSlip.createdAt;
                detail.createdBy = newReceiptSlip.createdBy;
                detail.status = ERP::Common::EntityStatus::ACTIVE; // Assuming detail has status
                detail.receivedQuantity = 0; // Initialize received quantity to 0
                detail.isFullyReceived = false; // Not yet fully received
                
                if (!receiptSlipDAO_->createReceiptSlipDetail(detail)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to create receipt slip detail for product " + detail.productId + ".");
                    return false;
                }
            }
            createdReceiptSlip = newReceiptSlip;
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::ReceiptSlipCreatedEvent>(newReceiptSlip));
            return true;
        },
        "ReceiptSlipService", "createReceiptSlip"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Receipt slip " + newReceiptSlip.receiptNumber + " created successfully with " + std::to_string(receiptSlipDetails.size()) + " details.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Material", "ReceiptSlip", newReceiptSlip.id, "ReceiptSlip", newReceiptSlip.receiptNumber,
                       std::nullopt, newReceiptSlip.toMap(), "Receipt slip created.");
        return createdReceiptSlip;
    }
    return std::nullopt;
}

std::optional<ERP::Material::DTO::ReceiptSlipDTO> ReceiptSlipService::getReceiptSlipById(
    const std::string& receiptSlipId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ReceiptSlipService: Retrieving receipt slip by ID: " + receiptSlipId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewReceiptSlips", "Bạn không có quyền xem phiếu nhập kho.")) {
        return std::nullopt;
    }

    return receiptSlipDAO_->getById(receiptSlipId); // Using getById from DAOBase template
}

std::optional<ERP::Material::DTO::ReceiptSlipDTO> ReceiptSlipService::getReceiptSlipByNumber(
    const std::string& receiptNumber,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ReceiptSlipService: Retrieving receipt slip by number: " + receiptNumber + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewReceiptSlips", "Bạn không có quyền xem phiếu nhập kho.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["receipt_number"] = receiptNumber;
    std::vector<ERP::Material::DTO::ReceiptSlipDTO> slips = receiptSlipDAO_->get(filter); // Using get from DAOBase template
    if (!slips.empty()) {
        return slips[0];
    }
    ERP::Logger::Logger::getInstance().debug("ReceiptSlipService: Receipt slip with number " + receiptNumber + " not found.");
    return std::nullopt;
}

std::vector<ERP::Material::DTO::ReceiptSlipDTO> ReceiptSlipService::getAllReceiptSlips(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Retrieving all receipt slips with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewReceiptSlips", "Bạn không có quyền xem tất cả phiếu nhập kho.")) {
        return {};
    }

    return receiptSlipDAO_->get(filter); // Using get from DAOBase template
}

bool ReceiptSlipService::updateReceiptSlip(
    const ERP::Material::DTO::ReceiptSlipDTO& receiptSlipDTO,
    const std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO>& receiptSlipDetails,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Attempting to update receipt slip: " + receiptSlipDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.UpdateReceiptSlip", "Bạn không có quyền cập nhật phiếu nhập kho.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::ReceiptSlipDTO> oldReceiptSlipOpt = receiptSlipDAO_->getById(receiptSlipDTO.id); // Using getById from DAOBase
    if (!oldReceiptSlipOpt) {
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Receipt slip with ID " + receiptSlipDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu nhập kho cần cập nhật.");
        return false;
    }

    // If receipt number is changed, check for uniqueness
    if (receiptSlipDTO.receiptNumber != oldReceiptSlipOpt->receiptNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["receipt_number"] = receiptSlipDTO.receiptNumber;
        if (receiptSlipDAO_->count(filterByNumber) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: New receipt number " + receiptSlipDTO.receiptNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ReceiptSlipService: New receipt number " + receiptSlipDTO.receiptNumber + " already exists.", "Số phiếu nhập kho mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate Warehouse existence
    if (receiptSlipDTO.warehouseId != oldReceiptSlipOpt->warehouseId) { // Only check if changed
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(receiptSlipDTO.warehouseId, userRoleIds);
        if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
            ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Invalid Warehouse ID provided for update or warehouse is not active: " + receiptSlipDTO.warehouseId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
            return false;
        }
    }

    // Validate details (similar to create)
    for (const auto& detail : receiptSlipDetails) {
        if (detail.productId.empty() || detail.locationId.empty() || detail.expectedQuantity <= 0) {
             ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Invalid detail input for product " + detail.productId);
             ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin chi tiết phiếu nhập kho không đầy đủ.");
             return false;
        }
        if (!productService_->getProductById(detail.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Detail product " + detail.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        if (!warehouseService_->getLocationById(detail.locationId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Detail location " + detail.locationId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Vị trí trong chi tiết không tồn tại.");
            return std::nullopt;
        }
    }

    ERP::Material::DTO::ReceiptSlipDTO updatedReceiptSlip = receiptSlipDTO;
    updatedReceiptSlip.updatedAt = ERP::Utils::DateUtils::now();
    updatedReceiptSlip.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!receiptSlipDAO_->update(updatedReceiptSlip)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to update receipt slip " + updatedReceiptSlip.id + " in DAO.");
                return false;
            }
            // Update details (remove all old, add new ones - simpler but less efficient for large lists)
            if (!receiptSlipDAO_->removeReceiptSlipDetailsByReceiptSlipId(updatedReceiptSlip.id)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to remove old receipt slip details for slip " + updatedReceiptSlip.id + ".");
                return false;
            }
            for (auto detail : receiptSlipDetails) {
                detail.id = ERP::Utils::generateUUID(); // New UUID for new details
                detail.receiptSlipId = updatedReceiptSlip.id;
                detail.createdAt = updatedReceiptSlip.createdAt; // Use parent creation info
                detail.createdBy = updatedReceiptSlip.createdBy;
                detail.status = ERP::Common::EntityStatus::ACTIVE;
                detail.receivedQuantity = 0; // Reset received quantity on full replacement
                detail.isFullyReceived = false;
                if (!receiptSlipDAO_->createReceiptSlipDetail(detail)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to create new receipt slip detail for product " + detail.productId + " for slip " + updatedReceiptSlip.id + ".");
                    return false;
                }
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::ReceiptSlipUpdatedEvent>(updatedReceiptSlip));
            return true;
        },
        "ReceiptSlipService", "updateReceiptSlip"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Receipt slip " + updatedReceiptSlip.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Material", "ReceiptSlip", updatedReceiptSlip.id, "ReceiptSlip", updatedReceiptSlip.receiptNumber,
                       oldReceiptSlipOpt->toMap(), updatedReceiptSlip.toMap(), "Receipt slip updated.");
        return true;
    }
    return false;
}

bool ReceiptSlipService::updateReceiptSlipStatus(
    const std::string& receiptSlipId,
    ERP::Material::DTO::ReceiptSlipStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Attempting to update status for receipt slip: " + receiptSlipId + " to " + ERP::Material::DTO::ReceiptSlipDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.UpdateReceiptSlipStatus", "Bạn không có quyền cập nhật trạng thái phiếu nhập kho.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::ReceiptSlipDTO> receiptSlipOpt = receiptSlipDAO_->getById(receiptSlipId); // Using getById from DAOBase
    if (!receiptSlipOpt) {
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Receipt slip with ID " + receiptSlipId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu nhập kho để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Material::DTO::ReceiptSlipDTO oldReceiptSlip = *receiptSlipOpt;
    if (oldReceiptSlip.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Receipt slip " + receiptSlipId + " is already in status " + oldReceiptSlip.getStatusString() + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from COMPLETED to IN_PROGRESS.

    ERP::Material::DTO::ReceiptSlipDTO updatedReceiptSlip = oldReceiptSlip;
    updatedReceiptSlip.status = newStatus;
    updatedReceiptSlip.updatedAt = ERP::Utils::DateUtils::now();
    updatedReceiptSlip.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!receiptSlipDAO_->update(updatedReceiptSlip)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to update status for receipt slip " + receiptSlipId + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::ReceiptSlipStatusChangedEvent>(receiptSlipId, newStatus));
            return true;
        },
        "ReceiptSlipService", "updateReceiptSlipStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Status for receipt slip " + receiptSlipId + " updated successfully to " + updatedReceiptSlip.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Material", "ReceiptSlipStatus", receiptSlipId, "ReceiptSlip", oldReceiptSlip.receiptNumber,
                       oldReceiptSlip.toMap(), updatedReceiptSlip.toMap(), "Receipt slip status changed to " + updatedReceiptSlip.getStatusString() + ".");
        return true;
    }
    return false;
}

bool ReceiptSlipService::deleteReceiptSlip(
    const std::string& receiptSlipId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Attempting to delete receipt slip: " + receiptSlipId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.DeleteReceiptSlip", "Bạn không có quyền xóa phiếu nhập kho.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::ReceiptSlipDTO> receiptSlipOpt = receiptSlipDAO_->getById(receiptSlipId); // Using getById from DAOBase
    if (!receiptSlipOpt) {
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Receipt slip with ID " + receiptSlipId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu nhập kho cần xóa.");
        return false;
    }

    ERP::Material::DTO::ReceiptSlipDTO receiptSlipToDelete = *receiptSlipOpt;

    // Additional checks: Prevent deletion if receipt slip is already completed or has associated inventory transactions
    if (receiptSlipToDelete.status == ERP::Material::DTO::ReceiptSlipStatus::COMPLETED) {
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Cannot delete receipt slip " + receiptSlipId + " as it is already completed.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa phiếu nhập kho đã hoàn thành.");
        return false;
    }
    // Check for associated inventory transactions (if already created via recordReceivedQuantity)
    // This would require querying InventoryTransactionDAO.

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated details first
            if (!receiptSlipDAO_->removeReceiptSlipDetailsByReceiptSlipId(receiptSlipId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to remove associated receipt slip details for slip " + receiptSlipId + ".");
                return false;
            }
            if (!receiptSlipDAO_->remove(receiptSlipId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to delete receipt slip " + receiptSlipId + " in DAO.");
                return false;
            }
            return true;
        },
        "ReceiptSlipService", "deleteReceiptSlip"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Receipt slip " + receiptSlipId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Material", "ReceiptSlip", receiptSlipId, "ReceiptSlip", receiptSlipToDelete.receiptNumber,
                       receiptSlipToDelete.toMap(), std::nullopt, "Receipt slip deleted.");
        return true;
    }
    return false;
}

bool ReceiptSlipService::recordReceivedQuantity(
    const std::string& detailId,
    double receivedQuantity,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Attempting to record received quantity for detail: " + detailId + " with quantity: " + std::to_string(receivedQuantity) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.RecordReceivedQuantity", "Bạn không có quyền ghi nhận số lượng nhận.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::ReceiptSlipDetailDTO> detailOpt = receiptSlipDAO_->getReceiptSlipDetailById(detailId); // Specific DAO method
    if (!detailOpt) {
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Receipt slip detail with ID " + detailId + " not found for recording received quantity.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy chi tiết phiếu nhập kho để ghi nhận số lượng.");
        return false;
    }
    
    ERP::Material::DTO::ReceiptSlipDetailDTO oldDetail = *detailOpt;

    // Validate new received quantity
    if (receivedQuantity < 0 || receivedQuantity < oldDetail.receivedQuantity) { // Cannot decrease received quantity
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Invalid received quantity for detail " + detailId + ": " + std::to_string(receivedQuantity));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng nhận không hợp lệ.");
        return false;
    }
    if (receivedQuantity > oldDetail.expectedQuantity) {
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Received quantity " + std::to_string(receivedQuantity) + " exceeds expected quantity " + std::to_string(oldDetail.expectedQuantity) + " for detail " + detailId + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng nhận vượt quá số lượng dự kiến.");
        return false;
    }

    ERP::Material::DTO::ReceiptSlipDetailDTO updatedDetail = oldDetail;
    updatedDetail.receivedQuantity = receivedQuantity;
    updatedDetail.isFullyReceived = (receivedQuantity >= updatedDetail.expectedQuantity);
    updatedDetail.updatedAt = ERP::Utils::DateUtils::now();
    updatedDetail.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Check current stock availability and record goods receipt for the incremental quantity
            double quantityToReceive = receivedQuantity - oldDetail.receivedQuantity; // Only the new incremental receipt
            if (quantityToReceive > 0) {
                // Need parent slip's warehouse ID
                std::optional<ERP::Material::DTO::ReceiptSlipDTO> parentSlipOpt = receiptSlipDAO_->getById(oldDetail.receiptSlipId);
                if (!parentSlipOpt) {
                    ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Parent Receipt Slip " + oldDetail.receiptSlipId + " not found for detail " + detailId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu nhập kho cha.");
                    return false;
                }
                std::string actualWarehouseId = parentSlipOpt->warehouseId;

                ERP::Warehouse::DTO::InventoryTransactionDTO receiptTransaction;
                receiptTransaction.id = ERP::Utils::generateUUID();
                receiptTransaction.productId = updatedDetail.productId;
                receiptTransaction.warehouseId = actualWarehouseId; // Use actual warehouse ID
                receiptTransaction.locationId = updatedDetail.locationId;
                receiptTransaction.type = ERP::Warehouse::DTO::InventoryTransactionType::GOODS_RECEIPT;
                receiptTransaction.quantity = quantityToReceive; // Use incremental quantity
                receiptTransaction.unitCost = updatedDetail.unitCost; // Use unit cost from detail
                receiptTransaction.transactionDate = ERP::Utils::DateUtils::now();
                receiptTransaction.lotNumber = updatedDetail.lotNumber;
                receiptTransaction.serialNumber = updatedDetail.serialNumber;
                receiptTransaction.manufactureDate = updatedDetail.manufactureDate;
                receiptTransaction.expirationDate = updatedDetail.expirationDate;
                receiptTransaction.referenceDocumentId = parentSlipOpt->id;
                receiptTransaction.referenceDocumentType = "ReceiptSlip";
                receiptTransaction.notes = "Received via Receipt Slip " + parentSlipOpt->receiptNumber;
                receiptTransaction.createdAt = parentSlipOpt->createdAt;
                receiptTransaction.createdBy = parentSlipOpt->createdBy;
                receiptTransaction.status = ERP::Common::EntityStatus::ACTIVE;

                if (!inventoryManagementService_->recordGoodsReceipt(receiptTransaction, currentUserId, userRoleIds)) {
                     ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to record goods receipt for product " + updatedDetail.productId + " via inventory service.");
                     return false;
                }
                updatedDetail.inventoryTransactionId = receiptTransaction.id; // Link to the created transaction
            }

            if (!receiptSlipDAO_->updateReceiptSlipDetail(updatedDetail)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to update received quantity for detail " + detailId + " in DAO.");
                return false;
            }

            // Check if parent Receipt Slip is fully completed
            std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> allDetails = receiptSlipDAO_->getReceiptSlipDetailsByReceiptSlipId(oldDetail.receiptSlipId);
            bool allFullyReceived = std::all_of(allDetails.begin(), allDetails.end(), [](const ERP::Material::DTO::ReceiptSlipDetailDTO& d) { return d.isFullyReceived; });
            
            std::optional<ERP::Material::DTO::ReceiptSlipDTO> parentSlip = receiptSlipDAO_->getById(oldDetail.receiptSlipId);
            if (parentSlip) {
                if (allFullyReceived && parentSlip->status != ERP::Material::DTO::ReceiptSlipStatus::COMPLETED) {
                    if (!updateReceiptSlipStatus(parentSlip->id, ERP::Material::DTO::ReceiptSlipStatus::COMPLETED, currentUserId, userRoleIds)) {
                        ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to auto-update parent Receipt Slip status to COMPLETED.");
                        // Don't fail the transaction, but log the error
                    }
                } else if (!allFullyReceived && parentSlip->status == ERP::Material::DTO::ReceiptSlipStatus::COMPLETED) {
                     // Revert status if not fully received anymore (e.g., if quantity was reduced)
                     if (!updateReceiptSlipStatus(parentSlip->id, ERP::Material::DTO::ReceiptSlipStatus::IN_PROGRESS, currentUserId, userRoleIds)) {
                        ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to auto-update parent Receipt Slip status to IN_PROGRESS.");
                     }
                } else if (parentSlip->status == ERP::Material::DTO::ReceiptSlipStatus::DRAFT || parentSlip->status == ERP::Material::DTO::ReceiptSlipStatus::PENDING_APPROVAL) {
                    // If recording first quantity, set status to IN_PROGRESS
                    if (!updateReceiptSlipStatus(parentSlip->id, ERP::Material::DTO::ReceiptSlipStatus::IN_PROGRESS, currentUserId, userRoleIds)) {
                        ERP::Logger::Logger::getInstance().error("ReceiptSlipService: Failed to auto-update parent Receipt Slip status to IN_PROGRESS from DRAFT/PENDING.");
                    }
                }
            }
            return true;
        },
        "ReceiptSlipService", "recordReceivedQuantity"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Received quantity recorded successfully for detail: " + detailId);
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Material", "ReceiptSlipDetail", detailId, "ReceiptSlipDetail", updatedDetail.productId,
                       oldDetail.toMap(), updatedDetail.toMap(), "Received quantity recorded: " + std::to_string(receivedQuantity) + ".");
        return true;
    }
    return false;
}

std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> ReceiptSlipService::getReceiptSlipDetails(
    const std::string& receiptSlipId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ReceiptSlipService: Retrieving receipt slip details for receipt slip ID: " + receiptSlipId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewReceiptSlips", "Bạn không có quyền xem chi tiết phiếu nhập kho.")) {
        return {};
    }

    // Check if parent Receipt Slip exists
    if (!receiptSlipDAO_->getById(receiptSlipId)) {
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipService: Parent Receipt Slip " + receiptSlipId + " not found when getting details.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Phiếu nhập kho cha không tồn tại.");
        return {};
    }

    return receiptSlipDAO_->getReceiptSlipDetailsByReceiptSlipId(receiptSlipId); // Specific DAO method
}

} // namespace Services
} // namespace Material
} // namespace ERP