// Modules/Supplier/Service/SupplierService.cpp
#include "SupplierService.h" // Đã rút gọn include
#include "Supplier.h" // Đã rút gọn include
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
namespace Supplier {
namespace Services {

SupplierService::SupplierService(
    std::shared_ptr<DAOs::SupplierDAO> supplierDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      supplierDAO_(supplierDAO) {
    if (!supplierDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "SupplierService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ nhà cung cấp.");
        ERP::Logger::Logger::getInstance().critical("SupplierService: Injected SupplierDAO is null.");
        throw std::runtime_error("SupplierService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("SupplierService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Supplier::DTO::SupplierDTO> SupplierService::createSupplier(
    const ERP::Supplier::DTO::SupplierDTO& supplierDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SupplierService: Attempting to create supplier: " + supplierDTO.name + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Supplier.CreateSupplier", "Bạn không có quyền tạo nhà cung cấp.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (supplierDTO.name.empty()) {
        ERP::Logger::Logger::getInstance().warning("SupplierService: Invalid input for supplier creation (empty name).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SupplierService: Invalid input for supplier creation.", "Tên nhà cung cấp không được để trống.");
        return std::nullopt;
    }

    // Check if supplier name already exists
    std::map<std::string, std::any> filterByName;
    filterByName["name"] = supplierDTO.name;
    if (supplierDAO_->count(filterByName) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("SupplierService: Supplier with name " + supplierDTO.name + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SupplierService: Supplier with name " + supplierDTO.name + " already exists.", "Tên nhà cung cấp đã tồn tại. Vui lòng chọn tên khác.");
        return std::nullopt;
    }

    ERP::Supplier::DTO::SupplierDTO newSupplier = supplierDTO;
    newSupplier.id = ERP::Utils::generateUUID();
    newSupplier.createdAt = ERP::Utils::DateUtils::now();
    newSupplier.createdBy = currentUserId;
    newSupplier.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Supplier::DTO::SupplierDTO> createdSupplier = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!supplierDAO_->create(newSupplier)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("SupplierService: Failed to create supplier " + newSupplier.name + " in DAO.");
                return false;
            }
            createdSupplier = newSupplier;
            eventBus_.publish(std::make_shared<EventBus::SupplierCreatedEvent>(newSupplier.id, newSupplier.name));
            return true;
        },
        "SupplierService", "createSupplier"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SupplierService: Supplier " + newSupplier.name + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Supplier", "Supplier", newSupplier.id, "Supplier", newSupplier.name,
                       std::nullopt, newSupplier.toMap(), "Supplier created.");
        return createdSupplier;
    }
    return std::nullopt;
}

std::optional<ERP::Supplier::DTO::SupplierDTO> SupplierService::getSupplierById(
    const std::string& supplierId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SupplierService: Retrieving supplier by ID: " + supplierId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Supplier.ViewSuppliers", "Bạn không có quyền xem nhà cung cấp.")) {
        return std::nullopt;
    }

    return supplierDAO_->getById(supplierId); // Using getById from DAOBase template
}

std::optional<ERP::Supplier::DTO::SupplierDTO> SupplierService::getSupplierByName(
    const std::string& supplierName,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("SupplierService: Retrieving supplier by name: " + supplierName + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Supplier.ViewSuppliers", "Bạn không có quyền xem nhà cung cấp.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["name"] = supplierName;
    std::vector<ERP::Supplier::DTO::SupplierDTO> suppliers = supplierDAO_->get(filter); // Using get from DAOBase template
    if (!suppliers.empty()) {
        return suppliers[0];
    }
    ERP::Logger::Logger::getInstance().debug("SupplierService: Supplier with name " + supplierName + " not found.");
    return std::nullopt;
}

std::vector<ERP::Supplier::DTO::SupplierDTO> SupplierService::getAllSuppliers(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SupplierService: Retrieving all suppliers with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Supplier.ViewSuppliers", "Bạn không có quyền xem tất cả nhà cung cấp.")) {
        return {};
    }

    return supplierDAO_->get(filter); // Using get from DAOBase template
}

bool SupplierService::updateSupplier(
    const ERP::Supplier::DTO::SupplierDTO& supplierDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SupplierService: Attempting to update supplier: " + supplierDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Supplier.UpdateSupplier", "Bạn không có quyền cập nhật nhà cung cấp.")) {
        return false;
    }

    std::optional<ERP::Supplier::DTO::SupplierDTO> oldSupplierOpt = supplierDAO_->getById(supplierDTO.id); // Using getById from DAOBase
    if (!oldSupplierOpt) {
        ERP::Logger::Logger::getInstance().warning("SupplierService: Supplier with ID " + supplierDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy nhà cung cấp cần cập nhật.");
        return false;
    }

    // If supplier name is changed, check for uniqueness
    if (supplierDTO.name != oldSupplierOpt->name) {
        std::map<std::string, std::any> filterByName;
        filterByName["name"] = supplierDTO.name;
        if (supplierDAO_->count(filterByName) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("SupplierService: New supplier name " + supplierDTO.name + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SupplierService: New supplier name " + supplierDTO.name + " already exists.", "Tên nhà cung cấp mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }

    ERP::Supplier::DTO::SupplierDTO updatedSupplier = supplierDTO;
    updatedSupplier.updatedAt = ERP::Utils::DateUtils::now();
    updatedSupplier.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!supplierDAO_->update(updatedSupplier)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SupplierService: Failed to update supplier " + updatedSupplier.id + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::SupplierUpdatedEvent>(updatedSupplier.id, updatedSupplier.name));
            return true;
        },
        "SupplierService", "updateSupplier"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SupplierService: Supplier " + updatedSupplier.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Supplier", "Supplier", updatedSupplier.id, "Supplier", updatedSupplier.name,
                       oldSupplierOpt->toMap(), updatedSupplier.toMap(), "Supplier updated.");
        return true;
    }
    return false;
}

bool SupplierService::updateSupplierStatus(
    const std::string& supplierId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SupplierService: Attempting to update status for supplier: " + supplierId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Supplier.UpdateSupplierStatus", "Bạn không có quyền cập nhật trạng thái nhà cung cấp.")) {
        return false;
    }

    std::optional<ERP::Supplier::DTO::SupplierDTO> supplierOpt = supplierDAO_->getById(supplierId); // Using getById from DAOBase
    if (!supplierOpt) {
        ERP::Logger::Logger::getInstance().warning("SupplierService: Supplier with ID " + supplierId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy nhà cung cấp để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Supplier::DTO::SupplierDTO oldSupplier = *supplierOpt;
    if (oldSupplier.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("SupplierService: Supplier " + supplierId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Supplier::DTO::SupplierDTO updatedSupplier = oldSupplier;
    updatedSupplier.status = newStatus;
    updatedSupplier.updatedAt = ERP::Utils::DateUtils::now();
    updatedSupplier.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!supplierDAO_->update(updatedSupplier)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("SupplierService: Failed to update status for supplier " + supplierId + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::SupplierStatusChangedEvent>(supplierId, newStatus));
            return true;
        },
        "SupplierService", "updateSupplierStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SupplierService: Status for supplier " + supplierId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Supplier", "SupplierStatus", supplierId, "Supplier", oldSupplier.name,
                       oldSupplier.toMap(), updatedSupplier.toMap(), "Supplier status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool SupplierService::deleteSupplier(
    const std::string& supplierId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("SupplierService: Attempting to delete supplier: " + supplierId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Supplier.DeleteSupplier", "Bạn không có quyền xóa nhà cung cấp.")) {
        return false;
    }

    std::optional<ERP::Supplier::DTO::SupplierDTO> supplierOpt = supplierDAO_->getById(supplierId); // Using getById from DAOBase
    if (!supplierOpt) {
        ERP::Logger::Logger::getInstance().warning("SupplierService: Supplier with ID " + supplierId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy nhà cung cấp cần xóa.");
        return false;
    }

    ERP::Supplier::DTO::SupplierDTO supplierToDelete = *supplierOpt;

    // Additional checks: Prevent deletion if supplier has associated products or purchase orders
    // This would require dependencies on ProductService, PurchaseOrderService (if exists)
    std::map<std::string, std::any> productFilter;
    productFilter["supplier_id"] = supplierId;
    if (securityManager_->getProductService()->getAllProducts(productFilter).size() > 0) { // Assuming ProductService can query by supplier
        ERP::Logger::Logger::getInstance().warning("SupplierService: Cannot delete supplier " + supplierId + " as it has associated products.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa nhà cung cấp có sản phẩm liên quan.");
        return false;
    }
    // Check for purchase orders if applicable

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!supplierDAO_->remove(supplierId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("SupplierService: Failed to delete supplier " + supplierId + " in DAO.");
                return false;
            }
            return true;
        },
        "SupplierService", "deleteSupplier"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("SupplierService: Supplier " + supplierId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Supplier", "Supplier", supplierId, "Supplier", supplierToDelete.name,
                       supplierToDelete.toMap(), std::nullopt, "Supplier deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Supplier
} // namespace ERP