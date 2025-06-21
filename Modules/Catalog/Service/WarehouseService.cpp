// Modules/Catalog/Service/WarehouseService.cpp
#include "WarehouseService.h" // Đã rút gọn include
#include "Warehouse.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "LocationService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Catalog {
namespace Services {

WarehouseService::WarehouseService(
    std::shared_ptr<DAOs::WarehouseDAO> warehouseDAO,
    std::shared_ptr<ILocationService> locationService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      warehouseDAO_(warehouseDAO), locationService_(locationService) {
    if (!warehouseDAO_ || !locationService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "WarehouseService: Initialized with null DAO or LocationService.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ kho hàng.");
        ERP::Logger::Logger::getInstance().critical("WarehouseService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("WarehouseService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("WarehouseService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Catalog::DTO::WarehouseDTO> WarehouseService::createWarehouse(
    const ERP::Catalog::DTO::WarehouseDTO& warehouseDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("WarehouseService: Attempting to create warehouse: " + warehouseDTO.name + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.CreateWarehouse", "Bạn không có quyền tạo kho hàng.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (warehouseDTO.name.empty()) {
        ERP::Logger::Logger::getInstance().warning("WarehouseService: Invalid input for warehouse creation (empty name).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "WarehouseService: Invalid input for warehouse creation.", "Tên kho hàng không được để trống.");
        return std::nullopt;
    }

    // Check if warehouse name already exists
    std::map<std::string, std::any> filterByName;
    filterByName["name"] = warehouseDTO.name;
    if (warehouseDAO_->count(filterByName) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("WarehouseService: Warehouse with name " + warehouseDTO.name + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "WarehouseService: Warehouse with name " + warehouseDTO.name + " already exists.", "Tên kho hàng đã tồn tại. Vui lòng chọn tên khác.");
        return std::nullopt;
    }

    ERP::Catalog::DTO::WarehouseDTO newWarehouse = warehouseDTO;
    newWarehouse.id = ERP::Utils::generateUUID();
    newWarehouse.createdAt = ERP::Utils::DateUtils::now();
    newWarehouse.createdBy = currentUserId;
    newWarehouse.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Catalog::DTO::WarehouseDTO> createdWarehouse = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!warehouseDAO_->create(newWarehouse)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("WarehouseService: Failed to create warehouse " + newWarehouse.name + " in DAO.");
                return false;
            }
            createdWarehouse = newWarehouse;
            eventBus_.publish(std::make_shared<EventBus::WarehouseCreatedEvent>(newWarehouse.id, newWarehouse.name));
            return true;
        },
        "WarehouseService", "createWarehouse"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("WarehouseService: Warehouse " + newWarehouse.name + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Warehouse", newWarehouse.id, "Warehouse", newWarehouse.name,
                       std::nullopt, newWarehouse.toMap(), "Warehouse created.");
        return createdWarehouse;
    }
    return std::nullopt;
}

std::optional<ERP::Catalog::DTO::WarehouseDTO> WarehouseService::getWarehouseById(
    const std::string& warehouseId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("WarehouseService: Retrieving warehouse by ID: " + warehouseId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewWarehouses", "Bạn không có quyền xem kho hàng.")) {
        return std::nullopt;
    }

    return warehouseDAO_->getById(warehouseId); // Using getById from DAOBase template
}

std::optional<ERP::Catalog::DTO::WarehouseDTO> WarehouseService::getWarehouseByName(
    const std::string& warehouseName,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("WarehouseService: Retrieving warehouse by name: " + warehouseName + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewWarehouses", "Bạn không có quyền xem kho hàng.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["name"] = warehouseName;
    std::vector<ERP::Catalog::DTO::WarehouseDTO> warehouses = warehouseDAO_->get(filter); // Using get from DAOBase template
    if (!warehouses.empty()) {
        return warehouses[0];
    }
    ERP::Logger::Logger::getInstance().debug("WarehouseService: Warehouse with name " + warehouseName + " not found.");
    return std::nullopt;
}

std::vector<ERP::Catalog::DTO::WarehouseDTO> WarehouseService::getAllWarehouses(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("WarehouseService: Retrieving all warehouses with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewWarehouses", "Bạn không có quyền xem tất cả kho hàng.")) {
        return {};
    }

    return warehouseDAO_->get(filter); // Using get from DAOBase template
}

bool WarehouseService::updateWarehouse(
    const ERP::Catalog::DTO::WarehouseDTO& warehouseDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("WarehouseService: Attempting to update warehouse: " + warehouseDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.UpdateWarehouse", "Bạn không có quyền cập nhật kho hàng.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::WarehouseDTO> oldWarehouseOpt = warehouseDAO_->getById(warehouseDTO.id); // Using getById from DAOBase
    if (!oldWarehouseOpt) {
        ERP::Logger::Logger::getInstance().warning("WarehouseService: Warehouse with ID " + warehouseDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy kho hàng cần cập nhật.");
        return false;
    }

    // If warehouse name is changed, check for uniqueness
    if (warehouseDTO.name != oldWarehouseOpt->name) {
        std::map<std::string, std::any> filterByName;
        filterByName["name"] = warehouseDTO.name;
        if (warehouseDAO_->count(filterByName) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("WarehouseService: New warehouse name " + warehouseDTO.name + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "WarehouseService: New warehouse name " + warehouseDTO.name + " already exists.", "Tên kho hàng mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }

    ERP::Catalog::DTO::WarehouseDTO updatedWarehouse = warehouseDTO;
    updatedWarehouse.updatedAt = ERP::Utils::DateUtils::now();
    updatedWarehouse.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!warehouseDAO_->update(updatedWarehouse)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("WarehouseService: Failed to update warehouse " + updatedWarehouse.id + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::WarehouseUpdatedEvent>(updatedWarehouse.id, updatedWarehouse.name));
            return true;
        },
        "WarehouseService", "updateWarehouse"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("WarehouseService: Warehouse " + updatedWarehouse.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Warehouse", updatedWarehouse.id, "Warehouse", updatedWarehouse.name,
                       oldWarehouseOpt->toMap(), updatedWarehouse.toMap(), "Warehouse updated.");
        return true;
    }
    return false;
}

bool WarehouseService::updateWarehouseStatus(
    const std::string& warehouseId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("WarehouseService: Attempting to update status for warehouse: " + warehouseId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ChangeWarehouseStatus", "Bạn không có quyền cập nhật trạng thái kho hàng.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouseOpt = warehouseDAO_->getById(warehouseId); // Using getById from DAOBase
    if (!warehouseOpt) {
        ERP::Logger::Logger::getInstance().warning("WarehouseService: Warehouse with ID " + warehouseId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy kho hàng để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Catalog::DTO::WarehouseDTO oldWarehouse = *warehouseOpt;
    if (oldWarehouse.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("WarehouseService: Warehouse " + warehouseId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Catalog::DTO::WarehouseDTO updatedWarehouse = oldWarehouse;
    updatedWarehouse.status = newStatus;
    updatedWarehouse.updatedAt = ERP::Utils::DateUtils::now();
    updatedWarehouse.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!warehouseDAO_->update(updatedWarehouse)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("WarehouseService: Failed to update status for warehouse " + warehouseId + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::WarehouseStatusChangedEvent>(warehouseId, newStatus));
            return true;
        },
        "WarehouseService", "updateWarehouseStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("WarehouseService: Status for warehouse " + warehouseId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "WarehouseStatus", warehouseId, "Warehouse", oldWarehouse.name,
                       oldWarehouse.toMap(), updatedWarehouse.toMap(), "Warehouse status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool WarehouseService::deleteWarehouse(
    const std::string& warehouseId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("WarehouseService: Attempting to delete warehouse: " + warehouseId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.DeleteWarehouse", "Bạn không có quyền xóa kho hàng.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouseOpt = warehouseDAO_->getById(warehouseId); // Using getById from DAOBase
    if (!warehouseOpt) {
        ERP::Logger::Logger::getInstance().warning("WarehouseService: Warehouse with ID " + warehouseId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy kho hàng cần xóa.");
        return false;
    }

    ERP::Catalog::DTO::WarehouseDTO warehouseToDelete = *warehouseOpt;

    // Additional checks: Prevent deletion if warehouse has associated locations or inventory
    std::map<std::string, std::any> locationFilter;
    locationFilter["warehouse_id"] = warehouseId;
    if (locationService_->getAllLocations(locationFilter).size() > 0) { // Assuming LocationService can query by warehouse
        ERP::Logger::Logger::getInstance().warning("WarehouseService: Cannot delete warehouse " + warehouseId + " as it has associated locations.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa kho hàng có vị trí liên quan.");
        return false;
    }
    std::map<std::string, std::any> inventoryFilter;
    inventoryFilter["warehouse_id"] = warehouseId;
    if (securityManager_->getInventoryManagementService()->getInventory(inventoryFilter).size() > 0) { // Assuming InventoryManagementService can query by warehouse
        ERP::Logger::Logger::getInstance().warning("WarehouseService: Cannot delete warehouse " + warehouseId + " as it has associated inventory records.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa kho hàng có tồn kho liên quan.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!warehouseDAO_->remove(warehouseId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("WarehouseService: Failed to delete warehouse " + warehouseId + " in DAO.");
                return false;
            }
            return true;
        },
        "WarehouseService", "deleteWarehouse"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("WarehouseService: Warehouse " + warehouseId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Warehouse", warehouseId, "Warehouse", warehouseToDelete.name,
                       warehouseToDelete.toMap(), std::nullopt, "Warehouse deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Catalog
} // namespace ERP