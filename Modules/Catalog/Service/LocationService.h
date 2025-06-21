// Modules/Catalog/Service/LocationService.h
#ifndef MODULES_CATALOG_SERVICE_LOCATIONSERVICE_H
#define MODULES_CATALOG_SERVICE_LOCATIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Location.h"         // Đã rút gọn include
#include "LocationDAO.h"      // Đã rút gọn include
#include "WarehouseService.h" // For Warehouse validation
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

namespace ERP {
namespace Catalog {
namespace Services {

// Forward declare if WarehouseService is only used via pointer/reference
// class IWarehouseService;

/**
 * @brief ILocationService interface defines operations for managing warehouse locations.
 */
class ILocationService {
public:
    virtual ~ILocationService() = default;
    /**
     * @brief Creates a new warehouse location.
     * @param locationDTO DTO containing new location information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional LocationDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::LocationDTO> createLocation(
        const ERP::Catalog::DTO::LocationDTO& locationDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves location information by ID.
     * @param locationId ID of the location to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional LocationDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::LocationDTO> getLocationById(
        const std::string& locationId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves location information by name and warehouse ID.
     * @param locationName Name of the location to retrieve.
     * @param warehouseId ID of the warehouse the location belongs to.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional LocationDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::LocationDTO> getLocationByNameAndWarehouse(
        const std::string& locationName,
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all locations or locations matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching LocationDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::LocationDTO> getAllLocations(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all locations within a specific warehouse.
     * @param warehouseId ID of the warehouse.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching LocationDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::LocationDTO> getLocationsByWarehouse(
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates location information.
     * @param locationDTO DTO containing updated location information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateLocation(
        const ERP::Catalog::DTO::LocationDTO& locationDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a location.
     * @param locationId ID of the location.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateLocationStatus(
        const std::string& locationId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a location record by ID (soft delete).
     * @param locationId ID of the location to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteLocation(
        const std::string& locationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of ILocationService.
 * This class uses LocationDAO and ISecurityManager.
 */
class LocationService : public ILocationService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for LocationService.
     * @param locationDAO Shared pointer to LocationDAO.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    LocationService(std::shared_ptr<DAOs::LocationDAO> locationDAO,
                    std::shared_ptr<IWarehouseService> warehouseService,
                    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                    std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Catalog::DTO::LocationDTO> createLocation(
        const ERP::Catalog::DTO::LocationDTO& locationDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Catalog::DTO::LocationDTO> getLocationById(
        const std::string& locationId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Catalog::DTO::LocationDTO> getLocationByNameAndWarehouse(
        const std::string& locationName,
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Catalog::DTO::LocationDTO> getAllLocations(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Catalog::DTO::LocationDTO> getLocationsByWarehouse(
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateLocation(
        const ERP::Catalog::DTO::LocationDTO& locationDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateLocationStatus(
        const std::string& locationId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteLocation(
        const std::string& locationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::LocationDAO> locationDAO_;
    std::shared_ptr<IWarehouseService> warehouseService_; // For warehouse validation
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_LOCATIONSERVICE_H