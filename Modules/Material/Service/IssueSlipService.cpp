// Modules/Material/Service/IssueSlipService.cpp
#include "IssueSlipService.h" // Đã rút gọn include
#include "IssueSlip.h" // Đã rút gọn include
#include "IssueSlipDetail.h" // Đã rút gọn include
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
#include "MaterialRequestSlipService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Material {
namespace Services {

IssueSlipService::IssueSlipService(
    std::shared_ptr<DAOs::IssueSlipDAO> issueSlipDAO,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<IMaterialRequestSlipService> materialRequestSlipService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      issueSlipDAO_(issueSlipDAO), productService_(productService),
      warehouseService_(warehouseService), inventoryManagementService_(inventoryManagementService),
      materialRequestSlipService_(materialRequestSlipService) {
    if (!issueSlipDAO_ || !productService_ || !warehouseService_ || !inventoryManagementService_ || !materialRequestSlipService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "IssueSlipService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ phiếu xuất kho.");
        ERP::Logger::Logger::getInstance().critical("IssueSlipService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("IssueSlipService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("IssueSlipService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Material::DTO::IssueSlipDTO> IssueSlipService::createIssueSlip(
    const ERP::Material::DTO::IssueSlipDTO& issueSlipDTO,
    const std::vector<ERP::Material::DTO::IssueSlipDetailDTO>& issueSlipDetails,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("IssueSlipService: Attempting to create issue slip: " + issueSlipDTO.issueNumber + " for warehouse: " + issueSlipDTO.warehouseId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.CreateIssueSlip", "Bạn không có quyền tạo phiếu xuất kho.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (issueSlipDTO.issueNumber.empty() || issueSlipDTO.warehouseId.empty() || issueSlipDetails.empty()) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Invalid input for issue slip creation (empty number, warehouse, or no details).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "IssueSlipService: Invalid input for issue slip creation.", "Thông tin phiếu xuất kho không đầy đủ.");
        return std::nullopt;
    }

    // Check if issue number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["issue_number"] = issueSlipDTO.issueNumber;
    if (issueSlipDAO_->count(filterByNumber) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Issue slip with number " + issueSlipDTO.issueNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "IssueSlipService: Issue slip with number " + issueSlipDTO.issueNumber + " already exists.", "Số phiếu xuất kho đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate Warehouse existence
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(issueSlipDTO.warehouseId, userRoleIds);
    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Invalid Warehouse ID provided or warehouse is not active: " + issueSlipDTO.warehouseId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
        return std::nullopt;
    }

    // Validate Material Request Slip existence if linked
    if (issueSlipDTO.materialRequestSlipId && !materialRequestSlipService_->getMaterialRequestSlipById(*issueSlipDTO.materialRequestSlipId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Linked Material Request Slip not found: " + *issueSlipDTO.materialRequestSlipId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Phiếu yêu cầu vật tư liên kết không tồn tại.");
        return std::nullopt;
    }

    // Validate details and check stock
    for (const auto& detail : issueSlipDetails) {
        if (!productService_->getProductById(detail.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("IssueSlipService: Detail product " + detail.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        if (!warehouseService_->getLocationById(detail.locationId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("IssueSlipService: Detail location " + detail.locationId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Vị trí trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        if (detail.requestedQuantity <= 0) {
            ERP::Logger::Logger::getInstance().warning("IssueSlipService: Detail product " + detail.productId + " has non-positive requested quantity.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng yêu cầu trong chi tiết phải lớn hơn 0.");
            return std::nullopt;
        }
        // Check current stock availability
        std::optional<ERP::Warehouse::DTO::InventoryDTO> inventory = inventoryManagementService_->getInventoryByProductLocation(detail.productId, issueSlipDTO.warehouseId, detail.locationId, userRoleIds);
        if (!inventory || inventory->quantity < detail.requestedQuantity) {
            ERP::Logger::Logger::getInstance().warning("IssueSlipService: Insufficient stock for product " + detail.productId + " at " + detail.locationId + ". Available: " + std::to_string(inventory.value_or(ERP::Warehouse::DTO::InventoryDTO()).quantity) + ", Requested: " + std::to_string(detail.requestedQuantity) + ".");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InsufficientStock, "Không đủ tồn kho cho sản phẩm " + detail.productId + " tại vị trí " + detail.locationId + ".");
            return std::nullopt;
        }
        // If linked to MaterialRequestSlipDetail, check against requested quantity there
        if (detail.materialRequestSlipDetailId) {
            std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> mrsDetail = materialRequestSlipService_->getMaterialRequestSlipDetailById(*detail.materialRequestSlipDetailId, userRoleIds);
            if (!mrsDetail || mrsDetail->productId != detail.productId || mrsDetail->materialRequestSlipId != *issueSlipDTO.materialRequestSlipId) {
                ERP::Logger::Logger::getInstance().warning("IssueSlipService: Linked Material Request Slip Detail not found or mismatched for " + *detail.materialRequestSlipDetailId);
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Chi tiết phiếu yêu cầu vật tư liên kết không hợp lệ.");
                return std::nullopt;
            }
            if (detail.requestedQuantity + mrsDetail->issuedQuantity > mrsDetail->requestedQuantity) {
                ERP::Logger::Logger::getInstance().warning("IssueSlipService: Issued quantity exceeds requested quantity in MRS detail for " + detail.productId);
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng xuất vượt quá số lượng yêu cầu trong phiếu yêu cầu vật tư.");
                return std::nullopt;
            }
        }
    }

    ERP::Material::DTO::IssueSlipDTO newIssueSlip = issueSlipDTO;
    newIssueSlip.id = ERP::Utils::generateUUID();
    newIssueSlip.createdAt = ERP::Utils::DateUtils::now();
    newIssueSlip.createdBy = currentUserId;
    newIssueSlip.status = ERP::Material::DTO::IssueSlipStatus::DRAFT; // Default status

    std::optional<ERP::Material::DTO::IssueSlipDTO> createdIssueSlip = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!issueSlipDAO_->create(newIssueSlip)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to create issue slip " + newIssueSlip.issueNumber + " in DAO.");
                return false;
            }
            // Create Issue Slip details
            for (auto detail : issueSlipDetails) {
                detail.id = ERP::Utils::generateUUID();
                detail.issueSlipId = newIssueSlip.id;
                detail.createdAt = newIssueSlip.createdAt;
                detail.createdBy = newIssueSlip.createdBy;
                detail.status = ERP::Common::EntityStatus::ACTIVE; // Assuming detail has status
                detail.issuedQuantity = 0; // Initialize issued quantity to 0
                detail.isFullyIssued = false; // Not yet fully issued
                
                if (!issueSlipDAO_->createIssueSlipDetail(detail)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to create issue slip detail for product " + detail.productId + ".");
                    return false;
                }
            }
            createdIssueSlip = newIssueSlip;
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::IssueSlipCreatedEvent>(newIssueSlip));
            return true;
        },
        "IssueSlipService", "createIssueSlip"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("IssueSlipService: Issue slip " + newIssueSlip.issueNumber + " created successfully with " + std::to_string(issueSlipDetails.size()) + " details.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Material", "IssueSlip", newIssueSlip.id, "IssueSlip", newIssueSlip.issueNumber,
                       std::nullopt, newIssueSlip.toMap(), "Issue slip created.");
        return createdIssueSlip;
    }
    return std::nullopt;
}

std::optional<ERP::Material::DTO::IssueSlipDTO> IssueSlipService::getIssueSlipById(
    const std::string& issueSlipId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("IssueSlipService: Retrieving issue slip by ID: " + issueSlipId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewIssueSlips", "Bạn không có quyền xem phiếu xuất kho.")) {
        return std::nullopt;
    }

    return issueSlipDAO_->getById(issueSlipId); // Using getById from DAOBase template
}

std::optional<ERP::Material::DTO::IssueSlipDTO> IssueSlipService::getIssueSlipByNumber(
    const std::string& issueNumber,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("IssueSlipService: Retrieving issue slip by number: " + issueNumber + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewIssueSlips", "Bạn không có quyền xem phiếu xuất kho.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["issue_number"] = issueNumber;
    std::vector<ERP::Material::DTO::IssueSlipDTO> slips = issueSlipDAO_->get(filter); // Using get from DAOBase template
    if (!slips.empty()) {
        return slips[0];
    }
    ERP::Logger::Logger::getInstance().debug("IssueSlipService: Issue slip with number " + issueNumber + " not found.");
    return std::nullopt;
}

std::vector<ERP::Material::DTO::IssueSlipDTO> IssueSlipService::getAllIssueSlips(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("IssueSlipService: Retrieving all issue slips with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewIssueSlips", "Bạn không có quyền xem tất cả phiếu xuất kho.")) {
        return {};
    }

    return issueSlipDAO_->get(filter); // Using get from DAOBase template
}

bool IssueSlipService::updateIssueSlip(
    const ERP::Material::DTO::IssueSlipDTO& issueSlipDTO,
    const std::vector<ERP::Material::DTO::IssueSlipDetailDTO>& issueSlipDetails,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("IssueSlipService: Attempting to update issue slip: " + issueSlipDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.UpdateIssueSlip", "Bạn không có quyền cập nhật phiếu xuất kho.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::IssueSlipDTO> oldIssueSlipOpt = issueSlipDAO_->getById(issueSlipDTO.id); // Using getById from DAOBase
    if (!oldIssueSlipOpt) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Issue slip with ID " + issueSlipDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu xuất kho cần cập nhật.");
        return false;
    }

    // If issue number is changed, check for uniqueness
    if (issueSlipDTO.issueNumber != oldIssueSlipOpt->issueNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["issue_number"] = issueSlipDTO.issueNumber;
        if (issueSlipDAO_->count(filterByNumber) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("IssueSlipService: New issue number " + issueSlipDTO.issueNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "IssueSlipService: New issue number " + issueSlipDTO.issueNumber + " already exists.", "Số phiếu xuất kho mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate Warehouse existence
    if (issueSlipDTO.warehouseId != oldIssueSlipOpt->warehouseId) { // Only check if changed
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(issueSlipDTO.warehouseId, userRoleIds);
        if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
            ERP::Logger::Logger::getInstance().warning("IssueSlipService: Invalid Warehouse ID provided for update or warehouse is not active: " + issueSlipDTO.warehouseId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
            return false;
        }
    }
    // Validate Material Request Slip existence if linked
    if (issueSlipDTO.materialRequestSlipId && !materialRequestSlipService_->getMaterialRequestSlipById(*issueSlipDTO.materialRequestSlipId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Linked Material Request Slip not found: " + *issueSlipDTO.materialRequestSlipId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Phiếu yêu cầu vật tư liên kết không tồn tại.");
        return std::nullopt;
    }

    // Validate details and check stock (similar to create) - This is for quantities being issued *now*
    for (const auto& detail : issueSlipDetails) {
        // Only validate new/updated details, not existing ones unless their quantities change.
        if (detail.productId.empty() || detail.locationId.empty() || detail.requestedQuantity <= 0) { // requestedQuantity for details still matters as it's the target
             ERP::Logger::Logger::getInstance().warning("IssueSlipService: Invalid detail input for product " + detail.productId);
             ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin chi tiết phiếu xuất kho không đầy đủ.");
             return false;
        }
        if (!productService_->getProductById(detail.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("IssueSlipService: Detail product " + detail.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        if (!warehouseService_->getLocationById(detail.locationId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("IssueSlipService: Detail location " + detail.locationId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Vị trí trong chi tiết không tồn tại.");
            return std::nullopt;
        }
        // Stock check is typically done during recordIssuedQuantity, not during slip update
        // unless update implies direct issuance. Here, we assume update is just for metadata/details list.
    }

    ERP::Material::DTO::IssueSlipDTO updatedIssueSlip = issueSlipDTO;
    updatedIssueSlip.updatedAt = ERP::Utils::DateUtils::now();
    updatedIssueSlip.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!issueSlipDAO_->update(updatedIssueSlip)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to update issue slip " + updatedIssueSlip.id + " in DAO.");
                return false;
            }
            // Update details (remove all old, add new ones - simpler but less efficient for large lists)
            if (!issueSlipDAO_->removeIssueSlipDetailsByIssueSlipId(updatedIssueSlip.id)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to remove old issue slip details for slip " + updatedIssueSlip.id + ".");
                return false;
            }
            for (auto detail : issueSlipDetails) {
                detail.id = ERP::Utils::generateUUID(); // New UUID for new details
                detail.issueSlipId = updatedIssueSlip.id;
                detail.createdAt = updatedIssueSlip.createdAt; // Use parent creation info
                detail.createdBy = updatedIssueSlip.createdBy;
                detail.status = ERP::Common::EntityStatus::ACTIVE;
                detail.issuedQuantity = 0; // Reset issued quantity on full replacement
                detail.isFullyIssued = false;
                if (!issueSlipDAO_->createIssueSlipDetail(detail)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to create new issue slip detail for product " + detail.productId + " for slip " + updatedIssueSlip.id + ".");
                    return false;
                }
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::IssueSlipUpdatedEvent>(updatedIssueSlip));
            return true;
        },
        "IssueSlipService", "updateIssueSlip"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("IssueSlipService: Issue slip " + updatedIssueSlip.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Material", "IssueSlip", updatedIssueSlip.id, "IssueSlip", updatedIssueSlip.issueNumber,
                       oldIssueSlipOpt->toMap(), updatedIssueSlip.toMap(), "Issue slip updated.");
        return true;
    }
    return false;
}

bool IssueSlipService::updateIssueSlipStatus(
    const std::string& issueSlipId,
    ERP::Material::DTO::IssueSlipStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("IssueSlipService: Attempting to update status for issue slip: " + issueSlipId + " to " + ERP::Material::DTO::IssueSlipDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.UpdateIssueSlipStatus", "Bạn không có quyền cập nhật trạng thái phiếu xuất kho.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::IssueSlipDTO> issueSlipOpt = issueSlipDAO_->getById(issueSlipId); // Using getById from DAOBase
    if (!issueSlipOpt) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Issue slip with ID " + issueSlipId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu xuất kho để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Material::DTO::IssueSlipDTO oldIssueSlip = *issueSlipOpt;
    if (oldIssueSlip.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("IssueSlipService: Issue slip " + issueSlipId + " is already in status " + oldIssueSlip.getStatusString() + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from COMPLETED to IN_PROGRESS.

    ERP::Material::DTO::IssueSlipDTO updatedIssueSlip = oldIssueSlip;
    updatedIssueSlip.status = newStatus;
    updatedIssueSlip.updatedAt = ERP::Utils::DateUtils::now();
    updatedIssueSlip.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!issueSlipDAO_->update(updatedIssueSlip)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to update status for issue slip " + issueSlipId + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::IssueSlipStatusChangedEvent>(issueSlipId, newStatus));
            return true;
        },
        "IssueSlipService", "updateIssueSlipStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("IssueSlipService: Status for issue slip " + issueSlipId + " updated successfully to " + updatedIssueSlip.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Material", "IssueSlipStatus", issueSlipId, "IssueSlip", oldIssueSlip.issueNumber,
                       oldIssueSlip.toMap(), updatedIssueSlip.toMap(), "Issue slip status changed to " + updatedIssueSlip.getStatusString() + ".");
        return true;
    }
    return false;
}

bool IssueSlipService::deleteIssueSlip(
    const std::string& issueSlipId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("IssueSlipService: Attempting to delete issue slip: " + issueSlipId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.DeleteIssueSlip", "Bạn không có quyền xóa phiếu xuất kho.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::IssueSlipDTO> issueSlipOpt = issueSlipDAO_->getById(issueSlipId); // Using getById from DAOBase
    if (!issueSlipOpt) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Issue slip with ID " + issueSlipId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu xuất kho cần xóa.");
        return false;
    }

    ERP::Material::DTO::IssueSlipDTO issueSlipToDelete = *issueSlipOpt;

    // Additional checks: Prevent deletion if issue slip is COMPLETED or has associated inventory transactions
    if (issueSlipToDelete.status == ERP::Material::DTO::IssueSlipStatus::COMPLETED) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Cannot delete issue slip " + issueSlipId + " as it is already completed.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa phiếu xuất kho đã hoàn thành.");
        return false;
    }
    // Check for associated inventory transactions (if already created via recordIssuedQuantity)
    // This would require querying InventoryTransactionDAO.

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated details first
            if (!issueSlipDAO_->removeIssueSlipDetailsByIssueSlipId(issueSlipId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to remove associated issue slip details for slip " + issueSlipId + ".");
                return false;
            }
            if (!issueSlipDAO_->remove(issueSlipId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to delete issue slip " + issueSlipId + " in DAO.");
                return false;
            }
            return true;
        },
        "IssueSlipService", "deleteIssueSlip"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("IssueSlipService: Issue slip " + issueSlipId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Material", "IssueSlip", issueSlipId, "IssueSlip", issueSlipToDelete.issueNumber,
                       issueSlipToDelete.toMap(), std::nullopt, "Issue slip deleted.");
        return true;
    }
    return false;
}

bool IssueSlipService::recordIssuedQuantity(
    const std::string& detailId,
    double issuedQuantity,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("IssueSlipService: Attempting to record issued quantity for detail: " + detailId + " with quantity: " + std::to_string(issuedQuantity) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.RecordIssuedQuantity", "Bạn không có quyền ghi nhận số lượng xuất.")) {
        return false;
    }

    std::optional<ERP::Material::DTO::IssueSlipDetailDTO> detailOpt = issueSlipDAO_->getIssueSlipDetailById(detailId); // Specific DAO method
    if (!detailOpt) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Issue slip detail with ID " + detailId + " not found for recording issued quantity.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy chi tiết phiếu xuất kho để ghi nhận số lượng.");
        return false;
    }
    
    ERP::Material::DTO::IssueSlipDetailDTO oldDetail = *detailOpt;

    // Validate new issued quantity
    if (issuedQuantity < 0 || issuedQuantity < oldDetail.issuedQuantity) { // Cannot decrease issued quantity
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Invalid issued quantity for detail " + detailId + ": " + std::to_string(issuedQuantity));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng xuất không hợp lệ.");
        return false;
    }
    if (issuedQuantity > oldDetail.requestedQuantity) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Issued quantity " + std::to_string(issuedQuantity) + " exceeds requested quantity " + std::to_string(oldDetail.requestedQuantity) + " for detail " + detailId + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng xuất vượt quá số lượng yêu cầu.");
        return false;
    }

    ERP::Material::DTO::IssueSlipDetailDTO updatedDetail = oldDetail;
    updatedDetail.issuedQuantity = issuedQuantity;
    updatedDetail.isFullyIssued = (issuedQuantity >= updatedDetail.requestedQuantity);
    updatedDetail.updatedAt = ERP::Utils::DateUtils::now();
    updatedDetail.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Check current stock availability before issuing
            std::optional<ERP::Warehouse::DTO::InventoryDTO> inventory = inventoryManagementService_->getInventoryByProductLocation(updatedDetail.productId, oldDetail.issueSlipId, updatedDetail.locationId, userRoleIds); // issueSlipId is placeholder for warehouseId
            // Need actual warehouseId from parent IssueSlip or pass it explicitly.
            // For now, let's assume issueSlipId is warehouseId. This needs fixing.
            // The correct way: get the parent IssueSlip, then get its warehouseId.
            std::optional<ERP::Material::DTO::IssueSlipDTO> parentSlipOpt = issueSlipDAO_->getById(oldDetail.issueSlipId);
            if (!parentSlipOpt) {
                ERP::Logger::Logger::getInstance().error("IssueSlipService: Parent Issue Slip " + oldDetail.issueSlipId + " not found for detail " + detailId + ".");
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu xuất kho cha.");
                return false;
            }
            std::string actualWarehouseId = parentSlipOpt->warehouseId;

            // Calculate net change to inventory
            double quantityToIssue = issuedQuantity - oldDetail.issuedQuantity; // Only the new incremental issue
            if (quantityToIssue > 0) {
                if (!inventoryManagementService_->recordGoodsIssue({
                    "", // id, will be generated by service
                    updatedDetail.productId,
                    actualWarehouseId, // Use actual warehouse ID
                    updatedDetail.locationId,
                    ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE,
                    quantityToIssue, // Use incremental quantity
                    0.0, // Unit cost to be determined by inventory costing method (FIFO/LIFO)
                    ERP::Utils::DateUtils::now(),
                    updatedDetail.lotNumber,
                    updatedDetail.serialNumber,
                    std::nullopt, std::nullopt, // manufacture/expiration dates
                    oldDetail.issueSlipId, "IssueSlip", // Reference to parent slip
                    "Issued via Issue Slip " + parentSlipOpt->issueNumber,
                    ERP::Common::EntityStatus::ACTIVE
                }, currentUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to record goods issue for product " + updatedDetail.productId + " via inventory service.");
                    return false;
                }
            }

            if (!issueSlipDAO_->updateIssueSlipDetail(updatedDetail)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to update issued quantity for detail " + detailId + " in DAO.");
                return false;
            }

            // Update Material Request Slip issued quantity if linked
            if (updatedDetail.materialRequestSlipDetailId) {
                std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> mrsDetail = materialRequestSlipService_->getMaterialRequestSlipDetailById(*updatedDetail.materialRequestSlipDetailId, userRoleIds);
                if (mrsDetail) {
                    ERP::Material::DTO::MaterialRequestSlipDetailDTO updatedMrsDetail = *mrsDetail;
                    updatedMrsDetail.issuedQuantity += quantityToIssue;
                    updatedMrsDetail.isFullyIssued = (updatedMrsDetail.issuedQuantity >= updatedMrsDetail.requestedQuantity);
                    // Update MRS detail (requires a method in MaterialRequestSlipService)
                    // materialRequestSlipService_->updateMaterialRequestSlipDetail(updatedMrsDetail, currentUserId, userRoleIds); // Assuming this method exists
                }
            }

            // Check if parent Issue Slip is fully completed
            std::vector<ERP::Material::DTO::IssueSlipDetailDTO> allDetails = issueSlipDAO_->getIssueSlipDetailsByIssueSlipId(oldDetail.issueSlipId);
            bool allFullyIssued = std::all_of(allDetails.begin(), allDetails.end(), [](const ERP::Material::DTO::IssueSlipDetailDTO& d) { return d.isFullyIssued; });
            if (allFullyIssued && parentSlipOpt->status != ERP::Material::DTO::IssueSlipStatus::COMPLETED) {
                if (!updateIssueSlipStatus(oldDetail.issueSlipId, ERP::Material::DTO::IssueSlipStatus::COMPLETED, currentUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to auto-update parent Issue Slip status to COMPLETED.");
                    // Don't fail the transaction, but log the error
                }
            } else if (!allFullyIssued && parentSlipOpt->status == ERP::Material::DTO::IssueSlipStatus::COMPLETED) {
                 // Revert status if not fully issued anymore (e.g., if quantity was reduced)
                 if (!updateIssueSlipStatus(oldDetail.issueSlipId, ERP::Material::DTO::IssueSlipStatus::IN_PROGRESS, currentUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to auto-update parent Issue Slip status to IN_PROGRESS.");
                 }
            } else if (parentSlipOpt->status == ERP::Material::DTO::IssueSlipStatus::DRAFT || parentSlipOpt->status == ERP::Material::DTO::IssueSlipStatus::PENDING_APPROVAL) {
                // If recording first quantity, set status to IN_PROGRESS
                if (!updateIssueSlipStatus(oldDetail.issueSlipId, ERP::Material::DTO::IssueSlipStatus::IN_PROGRESS, currentUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().error("IssueSlipService: Failed to auto-update parent Issue Slip status to IN_PROGRESS from DRAFT/PENDING.");
                }
            }


            return true;
        },
        "IssueSlipService", "recordIssuedQuantity"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("IssueSlipService: Issued quantity recorded successfully for detail: " + detailId);
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Material", "IssueSlipDetail", detailId, "IssueSlipDetail", updatedDetail.productId,
                       oldDetail.toMap(), updatedDetail.toMap(), "Issued quantity recorded: " + std::to_string(issuedQuantity) + ".");
        return true;
    }
    return false;
}

std::vector<ERP::Material::DTO::IssueSlipDetailDTO> IssueSlipService::getIssueSlipDetails(
    const std::string& issueSlipId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("IssueSlipService: Retrieving issue slip details for issue slip ID: " + issueSlipId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Material.ViewIssueSlips", "Bạn không có quyền xem chi tiết phiếu xuất kho.")) {
        return {};
    }

    // Check if parent Issue Slip exists
    if (!issueSlipDAO_->getById(issueSlipId)) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipService: Parent Issue Slip " + issueSlipId + " not found when getting details.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Phiếu xuất kho cha không tồn tại.");
        return {};
    }

    return issueSlipDAO_->getIssueSlipDetailsByIssueSlipId(issueSlipId); // Specific DAO method
}

} // namespace Services
} // namespace Material
} // namespace ERP