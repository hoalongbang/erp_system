// Modules/Manufacturing/Service/BillOfMaterialService.cpp
#include "BillOfMaterialService.h" // Đã rút gọn include
#include "BillOfMaterial.h" // Đã rút gọn include
#include "BillOfMaterialItem.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ProductService.h" // Đã rút gọn include
#include "UnitOfMeasureService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Manufacturing {
namespace Services {

BillOfMaterialService::BillOfMaterialService(
    std::shared_ptr<DAOs::BillOfMaterialDAO> bomDAO,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> unitOfMeasureService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      bomDAO_(bomDAO), productService_(productService), unitOfMeasureService_(unitOfMeasureService) {
    if (!bomDAO_ || !productService_ || !unitOfMeasureService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "BillOfMaterialService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ định mức nguyên vật liệu.");
        ERP::Logger::Logger::getInstance().critical("BillOfMaterialService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("BillOfMaterialService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> BillOfMaterialService::createBillOfMaterial(
    const ERP::Manufacturing::DTO::BillOfMaterialDTO& bomDTO,
    const std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO>& bomItems,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Attempting to create Bill of Material: " + bomDTO.bomName + " for product: " + bomDTO.productId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.CreateBillOfMaterial", "Bạn không có quyền tạo định mức nguyên vật liệu.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (bomDTO.bomName.empty() || bomDTO.productId.empty() || bomDTO.baseQuantityUnitId.empty() || bomDTO.baseQuantity <= 0) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: Invalid input for BOM creation (empty name, product, unit, or non-positive base quantity).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "BillOfMaterialService: Invalid input for BOM creation.", "Thông tin định mức nguyên vật liệu không đầy đủ hoặc không hợp lệ.");
        return std::nullopt;
    }

    // Check if BOM name or product already has an active BOM
    std::map<std::string, std::any> filterByName;
    filterByName["bom_name"] = bomDTO.bomName;
    filterByName["status"] = static_cast<int>(ERP::Manufacturing::DTO::BillOfMaterialStatus::ACTIVE);
    if (bomDAO_->count(filterByName) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: Active BOM with name " + bomDTO.bomName + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "BillOfMaterialService: Active BOM with name " + bomDTO.bomName + " already exists.", "Tên định mức nguyên vật liệu đã tồn tại và đang hoạt động. Vui lòng chọn tên khác hoặc vô hiệu hóa BOM cũ.");
        return std::nullopt;
    }
    // Check if product already has an active BOM
    std::map<std::string, std::any> filterByProduct;
    filterByProduct["product_id"] = bomDTO.productId;
    filterByProduct["status"] = static_cast<int>(ERP::Manufacturing::DTO::BillOfMaterialStatus::ACTIVE);
    if (bomDAO_->count(filterByProduct) > 0) { // Using count from DAOBase
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: Product " + bomDTO.productId + " already has an active BOM.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "BillOfMaterialService: Product " + bomDTO.productId + " already has an active BOM.", "Sản phẩm này đã có định mức nguyên vật liệu đang hoạt động. Vui lòng vô hiệu hóa BOM cũ trước.");
        return std::nullopt;
    }

    // Validate Product existence
    if (!productService_->getProductById(bomDTO.productId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: Product " + bomDTO.productId + " not found for BOM.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm không tồn tại.");
        return std::nullopt;
    }
    // Validate Base Quantity Unit existence
    if (!unitOfMeasureService_->getUnitOfMeasureById(bomDTO.baseQuantityUnitId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: Base quantity unit " + bomDTO.baseQuantityUnitId + " not found for BOM.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Đơn vị tính cơ sở không tồn tại.");
        return std::nullopt;
    }

    // Validate BOM items
    if (bomItems.empty()) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: No BOM items provided for BOM creation.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Định mức nguyên vật liệu phải có ít nhất một thành phần.");
        return std::nullopt;
    }
    for (const auto& item : bomItems) {
        if (!productService_->getProductById(item.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM item product " + item.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Thành phần sản phẩm không tồn tại.");
            return std::nullopt;
        }
        if (!unitOfMeasureService_->getUnitOfMeasureById(item.unitOfMeasureId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM item unit of measure " + item.unitOfMeasureId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Đơn vị tính của thành phần không tồn tại.");
            return std::nullopt;
        }
        if (item.quantity <= 0) {
            ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM item " + item.productId + " has non-positive quantity.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng thành phần phải lớn hơn 0.");
            return std::nullopt;
        }
    }

    ERP::Manufacturing::DTO::BillOfMaterialDTO newBom = bomDTO;
    newBom.id = ERP::Utils::generateUUID();
    newBom.createdAt = ERP::Utils::DateUtils::now();
    newBom.createdBy = currentUserId;
    newBom.status = ERP::Manufacturing::DTO::BillOfMaterialStatus::DRAFT; // Default status

    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> createdBom = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!bomDAO_->create(newBom)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("BillOfMaterialService: Failed to create BOM " + newBom.bomName + " in DAO.");
                return false;
            }
            // Create BOM items
            for (auto item : bomItems) {
                item.id = ERP::Utils::generateUUID(); // Ensure item has its own UUID
                if (!bomDAO_->createBomItem(item, newBom.id)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("BillOfMaterialService: Failed to create BOM item " + item.productId + " for BOM " + newBom.id + ".");
                    return false;
                }
            }
            createdBom = newBom;
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::BillOfMaterialCreatedEvent>(newBom));
            return true;
        },
        "BillOfMaterialService", "createBillOfMaterial"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Bill of Material " + newBom.bomName + " created successfully with " + std::to_string(bomItems.size()) + " items.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "BillOfMaterial", newBom.id, "BillOfMaterial", newBom.bomName,
                       std::nullopt, newBom.toMap(), "Bill of Material created.");
        return createdBom;
    }
    return std::nullopt;
}

std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> BillOfMaterialService::getBillOfMaterialById(
    const std::string& bomId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("BillOfMaterialService: Retrieving BOM by ID: " + bomId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewBillOfMaterial", "Bạn không có quyền xem định mức nguyên vật liệu.")) {
        return std::nullopt;
    }

    return bomDAO_->getById(bomId); // Using getById from DAOBase template
}

std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> BillOfMaterialService::getBillOfMaterialByNameOrProductId(
    const std::string& bomNameOrProductId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("BillOfMaterialService: Retrieving BOM by name or product ID: " + bomNameOrProductId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewBillOfMaterial", "Bạn không có quyền xem định mức nguyên vật liệu.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filterByName;
    filterByName["bom_name"] = bomNameOrProductId;
    std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> boms = bomDAO_->get(filterByName); // Using get from DAOBase
    if (!boms.empty()) {
        return boms[0];
    }

    std::map<std::string, std::any> filterByProductId;
    filterByProductId["product_id"] = bomNameOrProductId;
    boms = bomDAO_->get(filterByProductId); // Using get from DAOBase
    if (!boms.empty()) {
        return boms[0];
    }

    ERP::Logger::Logger::getInstance().debug("BillOfMaterialService: BOM with name or product ID " + bomNameOrProductId + " not found.");
    return std::nullopt;
}

std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> BillOfMaterialService::getAllBillOfMaterials(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Retrieving all Bills of Material with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewBillOfMaterial", "Bạn không có quyền xem tất cả định mức nguyên vật liệu.")) {
        return {};
    }

    return bomDAO_->get(filter); // Using get from DAOBase template
}

bool BillOfMaterialService::updateBillOfMaterial(
    const ERP::Manufacturing::DTO::BillOfMaterialDTO& bomDTO,
    const std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO>& bomItems,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Attempting to update Bill of Material: " + bomDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.UpdateBillOfMaterial", "Bạn không có quyền cập nhật định mức nguyên vật liệu.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> oldBomOpt = bomDAO_->getById(bomDTO.id); // Using getById from DAOBase
    if (!oldBomOpt) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM with ID " + bomDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy định mức nguyên vật liệu cần cập nhật.");
        return false;
    }

    // If BOM name or product is changed, check for uniqueness (excluding self)
    if (bomDTO.bomName != oldBomOpt->bomName) {
        std::map<std::string, std::any> filterByName;
        filterByName["bom_name"] = bomDTO.bomName;
        std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> existingBoms = bomDAO_->get(filterByName);
        if (!existingBoms.empty() && existingBoms[0].id != bomDTO.id) {
            ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: New BOM name " + bomDTO.bomName + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Tên định mức nguyên vật liệu mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }
    if (bomDTO.productId != oldBomOpt->productId) {
        std::map<std::string, std::any> filterByProduct;
        filterByProduct["product_id"] = bomDTO.productId;
        std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> existingBoms = bomDAO_->get(filterByProduct);
        if (!existingBoms.empty() && existingBoms[0].id != bomDTO.id) {
            ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: Product " + bomDTO.productId + " already has a BOM associated with it.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Sản phẩm đã có định mức nguyên vật liệu. Vui lòng chọn sản phẩm khác hoặc cập nhật BOM hiện có.");
            return false;
        }
    }

    // Validate Product existence
    if (!productService_->getProductById(bomDTO.productId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: Product " + bomDTO.productId + " not found for BOM update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm không tồn tại.");
        return false;
    }
    // Validate Base Quantity Unit existence
    if (!unitOfMeasureService_->getUnitOfMeasureById(bomDTO.baseQuantityUnitId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: Base quantity unit " + bomDTO.baseQuantityUnitId + " not found for BOM update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Đơn vị tính cơ sở không tồn tại.");
        return std::nullopt;
    }

    // Validate BOM items
    if (bomItems.empty()) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: No BOM items provided for BOM update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Định mức nguyên vật liệu phải có ít nhất một thành phần.");
        return std::nullopt;
    }
    for (const auto& item : bomItems) {
        if (!productService_->getProductById(item.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM item product " + item.productId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Thành phần sản phẩm không tồn tại.");
            return std::nullopt;
        }
        if (!unitOfMeasureService_->getUnitOfMeasureById(item.unitOfMeasureId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM item unit of measure " + item.unitOfMeasureId + " not found.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Đơn vị tính của thành phần không tồn tại.");
            return std::nullopt;
        }
        if (item.quantity <= 0) {
            ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM item " + item.productId + " has non-positive quantity.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng thành phần phải lớn hơn 0.");
            return std::nullopt;
        }
    }

    ERP::Manufacturing::DTO::BillOfMaterialDTO updatedBom = bomDTO;
    updatedBom.updatedAt = ERP::Utils::DateUtils::now();
    updatedBom.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!bomDAO_->update(updatedBom)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("BillOfMaterialService: Failed to update BOM " + updatedBom.id + " in DAO.");
                return false;
            }
            // Remove old BOM items and add new ones (full replacement)
            if (!bomDAO_->removeBomItemsByBomId(updatedBom.id)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("BillOfMaterialService: Failed to remove old BOM items for BOM " + updatedBom.id + ".");
                return false;
            }
            for (auto item : bomItems) {
                item.id = ERP::Utils::generateUUID(); // Ensure new UUID for each item
                if (!bomDAO_->createBomItem(item, updatedBom.id)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("BillOfMaterialService: Failed to create new BOM item " + item.productId + " for BOM " + updatedBom.id + ".");
                    return false;
                }
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::BillOfMaterialUpdatedEvent>(updatedBom));
            return true;
        },
        "BillOfMaterialService", "updateBillOfMaterial"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Bill of Material " + updatedBom.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "BillOfMaterial", updatedBom.id, "BillOfMaterial", updatedBom.bomName,
                       oldBomOpt->toMap(), updatedBom.toMap(), "Bill of Material updated.");
        return true;
    }
    return false;
}

bool BillOfMaterialService::updateBillOfMaterialStatus(
    const std::string& bomId,
    ERP::Manufacturing::DTO::BillOfMaterialStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Attempting to update status for BOM: " + bomId + " to " + ERP::Manufacturing::DTO::BillOfMaterialDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.UpdateBillOfMaterialStatus", "Bạn không có quyền cập nhật trạng thái định mức nguyên vật liệu.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> bomOpt = bomDAO_->getById(bomId); // Using getById from DAOBase
    if (!bomOpt) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM with ID " + bomId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy định mức nguyên vật liệu để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Manufacturing::DTO::BillOfMaterialDTO oldBom = *bomOpt;
    if (oldBom.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("BillOfMaterialService: BOM " + bomId + " is already in status " + oldBom.getStatusString() + ".");
        return true; // Already in desired status
    }

    ERP::Manufacturing::DTO::BillOfMaterialDTO updatedBom = oldBom;
    updatedBom.status = newStatus;
    updatedBom.updatedAt = ERP::Utils::DateUtils::now();
    updatedBom.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!bomDAO_->update(updatedBom)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("BillOfMaterialService: Failed to update status for BOM " + bomId + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::BillOfMaterialStatusChangedEvent>(bomId, newStatus));
            return true;
        },
        "BillOfMaterialService", "updateBillOfMaterialStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Status for BOM " + bomId + " updated successfully to " + updatedBom.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "BillOfMaterialStatus", bomId, "BillOfMaterial", oldBom.bomName,
                       oldBom.toMap(), updatedBom.toMap(), "Bill of Material status changed to " + updatedBom.getStatusString() + ".");
        return true;
    }
    return false;
}

bool BillOfMaterialService::deleteBillOfMaterial(
    const std::string& bomId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Attempting to delete Bill of Material: " + bomId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.DeleteBillOfMaterial", "Bạn không có quyền xóa định mức nguyên vật liệu.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> bomOpt = bomDAO_->getById(bomId); // Using getById from DAOBase
    if (!bomOpt) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM with ID " + bomId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy định mức nguyên vật liệu cần xóa.");
        return false;
    }

    ERP::Manufacturing::DTO::BillOfMaterialDTO bomToDelete = *bomOpt;

    // Additional checks: Prevent deletion if BOM is used in any active production orders
    std::map<std::string, std::any> productionOrderFilter;
    productionOrderFilter["bom_id"] = bomId;
    if (securityManager_->getProductionOrderService()->getAllProductionOrders(productionOrderFilter).size() > 0) { // Assuming ProductionOrderService can query by BOM
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: Cannot delete BOM " + bomId + " as it is used in active production orders.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa định mức nguyên vật liệu đang được sử dụng trong lệnh sản xuất.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated items first
            if (!bomDAO_->removeBomItemsByBomId(bomId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("BillOfMaterialService: Failed to remove associated BOM items for BOM " + bomId + ".");
                return false;
            }
            if (!bomDAO_->remove(bomId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("BillOfMaterialService: Failed to delete BOM " + bomId + " in DAO.");
                return false;
            }
            return true;
        },
        "BillOfMaterialService", "deleteBillOfMaterial"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Bill of Material " + bomId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "BillOfMaterial", bomId, "BillOfMaterial", bomToDelete.bomName,
                       bomToDelete.toMap(), std::nullopt, "Bill of Material deleted.");
        return true;
    }
    return false;
}

std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> BillOfMaterialService::getBillOfMaterialItems(
    const std::string& bomId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialService: Retrieving BOM items for BOM ID: " + bomId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewBillOfMaterial", "Bạn không có quyền xem các thành phần định mức nguyên vật liệu.")) {
        return {};
    }

    // Check if BOM exists
    if (!bomDAO_->getById(bomId)) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialService: BOM " + bomId + " not found when getting items.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Định mức nguyên vật liệu không tồn tại.");
        return {};
    }

    return bomDAO_->getBomItemsByBomId(bomId); // Specific DAO method
}

} // namespace Services
} // namespace Manufacturing
} // namespace ERP