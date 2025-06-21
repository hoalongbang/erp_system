// Modules/Catalog/Service/LocationService.cpp
#include "LocationService.h" // Đã rút gọn include
#include "Location.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "WarehouseService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Catalog {
namespace Services {

LocationService::LocationService(
    std::shared_ptr<DAOs::LocationDAO> locationDAO,
    std::shared_ptr<IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      locationDAO_(locationDAO), warehouseService_(warehouseService) {
    if (!locationDAO_ || !warehouseService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "LocationService: Initialized with null DAO or WarehouseService.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ vị trí kho.");
        ERP::Logger::Logger::getInstance().critical("LocationService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("LocationService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("LocationService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Catalog::DTO::LocationDTO> LocationService::createLocation(
    const ERP::Catalog::DTO::LocationDTO& locationDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("LocationService: Attempting to create location: " + locationDTO.name + " in warehouse " + locationDTO.warehouseId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.CreateLocation", "Bạn không có quyền tạo vị trí kho.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (locationDTO.name.empty() || locationDTO.warehouseId.empty()) {
        ERP::Logger::Logger::getInstance().warning("LocationService: Invalid input for location creation (empty name or warehouseId).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "LocationService: Invalid input for location creation.", "Thông tin vị trí kho không đầy đủ.");
        return std::nullopt;
    }

    // Check if location name already exists within the same warehouse
    std::map<std::string, std::any> filterByNameAndWarehouse;
    filterByNameAndWarehouse["name"] = locationDTO.name;
    filterByNameAndWarehouse["warehouse_id"] = locationDTO.warehouseId;
    if (locationDAO_->count(filterByNameAndWarehouse) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("LocationService: Location with name " + locationDTO.name + " already exists in warehouse " + locationDTO.warehouseId + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "LocationService: Location with name " + locationDTO.name + " already exists in this warehouse.", "Tên vị trí kho đã tồn tại trong kho này. Vui lòng chọn tên khác.");
        return std::nullopt;
    }

    // Validate Warehouse existence (assuming WarehouseService handles permission internally)
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(locationDTO.warehouseId, userRoleIds);
    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("LocationService: Invalid Warehouse ID provided or warehouse is not active: " + locationDTO.warehouseId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
        return std::nullopt;
    }

    ERP::Catalog::DTO::LocationDTO newLocation = locationDTO;
    newLocation.id = ERP::Utils::generateUUID();
    newLocation.createdAt = ERP::Utils::DateUtils::now();
    newLocation.createdBy = currentUserId;
    newLocation.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Catalog::DTO::LocationDTO> createdLocation = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Note: DAO methods already acquire/release connections internally now.
            if (!locationDAO_->create(newLocation)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("LocationService: Failed to create location " + newLocation.name + " in DAO.");
                return false;
            }
            createdLocation = newLocation;
            eventBus_.publish(std::make_shared<EventBus::LocationCreatedEvent>(newLocation.id, newLocation.name, newLocation.warehouseId));
            return true;
        },
        "LocationService", "createLocation"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("LocationService: Location " + newLocation.name + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Location", newLocation.id, "Location", newLocation.name,
                       std::nullopt, newLocation.toMap(), "Location created in warehouse: " + newLocation.warehouseId + ".");
        return createdLocation;
    }
    return std::nullopt;
}

std::optional<ERP::Catalog::DTO::LocationDTO> LocationService::getLocationById(
    const std::string& locationId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("LocationService: Retrieving location by ID: " + locationId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewLocations", "Bạn không có quyền xem vị trí kho.")) {
        return std::nullopt;
    }

    return locationDAO_->getById(locationId); // Using getById from DAOBase template
}

std::optional<ERP::Catalog::DTO::LocationDTO> LocationService::getLocationByNameAndWarehouse(
    const std::string& locationName,
    const std::string& warehouseId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("LocationService: Retrieving location by name: " + locationName + " in warehouse: " + warehouseId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewLocations", "Bạn không có quyền xem vị trí kho.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["name"] = locationName;
    filter["warehouse_id"] = warehouseId;
    std::vector<ERP::Catalog::DTO::LocationDTO> locations = locationDAO_->get(filter); // Using get from DAOBase template
    if (!locations.empty()) {
        return locations[0];
    }
    ERP::Logger::Logger::getInstance().debug("LocationService: Location with name " + locationName + " not found in warehouse " + warehouseId + ".");
    return std::nullopt;
}

std::vector<ERP::Catalog::DTO::LocationDTO> LocationService::getAllLocations(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("LocationService: Retrieving all locations with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewLocations", "Bạn không có quyền xem tất cả vị trí kho.")) {
        return {};
    }

    return locationDAO_->get(filter); // Using get from DAOBase template
}

std::vector<ERP::Catalog::DTO::LocationDTO> LocationService::getLocationsByWarehouse(
    const std::string& warehouseId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("LocationService: Retrieving locations for warehouse: " + warehouseId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewLocations", "Bạn không có quyền xem vị trí kho của kho hàng này.")) {
        return {};
    }

    std::map<std::string, std::any> filter;
    filter["warehouse_id"] = warehouseId;
    return locationDAO_->get(filter); // Using get from DAOBase template
}

bool LocationService::updateLocation(
    const ERP::Catalog::DTO::LocationDTO& locationDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("LocationService: Attempting to update location: " + locationDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.UpdateLocation", "Bạn không có quyền cập nhật vị trí kho.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::LocationDTO> oldLocationOpt = locationDAO_->getById(locationDTO.id); // Using getById from DAOBase
    if (!oldLocationOpt) {
        ERP::Logger::Logger::getInstance().warning("LocationService: Location with ID " + locationDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy vị trí kho cần cập nhật.");
        return false;
    }

    // If location name or warehouse is changed, check for uniqueness
    if (locationDTO.name != oldLocationOpt->name || locationDTO.warehouseId != oldLocationOpt->warehouseId) {
        std::map<std::string, std::any> filterByNameAndWarehouse;
        filterByNameAndWarehouse["name"] = locationDTO.name;
        filterByNameAndWarehouse["warehouse_id"] = locationDTO.warehouseId;
        if (locationDAO_->count(filterByNameAndWarehouse) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("LocationService: New location name " + locationDTO.name + " already exists in warehouse " + locationDTO.warehouseId + ".");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Tên vị trí kho mới đã tồn tại trong kho này. Vui lòng chọn tên khác.");
            return false;
        }
    }

    // Validate Warehouse existence if specified or changed
    if (locationDTO.warehouseId != oldLocationOpt->warehouseId) {
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(locationDTO.warehouseId, userRoleIds);
        if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
            ERP::Logger::Logger::getInstance().warning("LocationService: Invalid Warehouse ID provided for update or warehouse is not active: " + locationDTO.warehouseId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc kho hàng không hoạt động.");
            return false;
        }
    }

    ERP::Catalog::DTO::LocationDTO updatedLocation = locationDTO;
    updatedLocation.updatedAt = ERP::Utils::DateUtils::now();
    updatedLocation.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!locationDAO_->update(updatedLocation)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("LocationService: Failed to update location " + updatedLocation.id + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::LocationUpdatedEvent>(updatedLocation.id, updatedLocation.name, updatedLocation.warehouseId));
            return true;
        },
        "LocationService", "updateLocation"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("LocationService: Location " + updatedLocation.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Location", updatedLocation.id, "Location", updatedLocation.name,
                       oldLocationOpt->toMap(), updatedLocation.toMap(), "Location updated.");
        return true;
    }
    return false;
}

bool LocationService::updateLocationStatus(
    const std::string& locationId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("LocationService: Attempting to update status for location: " + locationId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ChangeLocationStatus", "Bạn không có quyền cập nhật trạng thái vị trí kho.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::LocationDTO> locationOpt = locationDAO_->getById(locationId); // Using getById from DAOBase
    if (!locationOpt) {
        ERP::Logger::Logger::getInstance().warning("LocationService: Location with ID " + locationId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy vị trí kho để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Catalog::DTO::LocationDTO oldLocation = *locationOpt;
    if (oldLocation.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("LocationService: Location " + locationId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Catalog::DTO::LocationDTO updatedLocation = oldLocation;
    updatedLocation.status = newStatus;
    updatedLocation.updatedAt = ERP::Utils::DateUtils::now();
    updatedLocation.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!locationDAO_->update(updatedLocation)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("LocationService: Failed to update status for location " + locationId + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::LocationStatusChangedEvent>(locationId, newStatus));
            return true;
        },
        "LocationService", "updateLocationStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("LocationService: Status for location " + locationId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "LocationStatus", locationId, "Location", oldLocation.name,
                       oldLocation.toMap(), updatedLocation.toMap(), "Location status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool LocationService::deleteLocation(
    const std::string& locationId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("LocationService: Attempting to delete location: " + locationId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.DeleteLocation", "Bạn không có quyền xóa vị trí kho.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::LocationDTO> locationOpt = locationDAO_->getById(locationId); // Using getById from DAOBase
    if (!locationOpt) {
        ERP::Logger::Logger::getInstance().warning("LocationService: Location with ID " + locationId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy vị trí kho cần xóa.");
        return false;
    }

    ERP::Catalog::DTO::LocationDTO locationToDelete = *locationOpt;

    // Additional checks: Prevent deletion if location has associated inventory
    std::map<std::string, std::any> inventoryFilter;
    inventoryFilter["location_id"] = locationId;
    if (securityManager_->getInventoryManagementService()->getInventory(inventoryFilter).size() > 0) { // Assuming InventoryManagementService can query by location
        ERP::Logger::Logger::getInstance().warning("LocationService: Cannot delete location " + locationId + " as it has associated inventory records.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa vị trí kho có tồn kho liên quan.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!locationDAO_->remove(locationId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("LocationService: Failed to delete location " + locationId + " in DAO.");
                return false;
            }
            return true;
        },
        "LocationService", "deleteLocation"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("LocationService: Location " + locationId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Location", locationId, "Location", locationToDelete.name,
                       locationToDelete.toMap(), std::nullopt, "Location deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Catalog
} // namespace ERP