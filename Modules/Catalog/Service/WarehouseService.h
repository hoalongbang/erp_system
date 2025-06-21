// Modules/Catalog/Service/WarehouseService.h
#ifndef MODULES_CATALOG_SERVICE_WAREHOUSESERVICE_H
#define MODULES_CATALOG_SERVICE_WAREHOUSESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Warehouse.h"        // Đã rút gọn include
#include "WarehouseDAO.h"     // Đã rút gọn include
#include "LocationService.h"  // For Location validation
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

// Forward declare if LocationService is only used via pointer/reference
// class ILocationService;

/**
 * @brief IWarehouseService interface defines operations for managing warehouses.
 */
class IWarehouseService {
public:
    virtual ~IWarehouseService() = default;
    /**
     * @brief Creates a new warehouse.
     * @param warehouseDTO DTO containing new warehouse information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional WarehouseDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::WarehouseDTO> createWarehouse(
        const ERP::Catalog::DTO::WarehouseDTO& warehouseDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves warehouse information by ID.
     * @param warehouseId ID of the warehouse to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional WarehouseDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::WarehouseDTO> getWarehouseById(
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves warehouse information by name.
     * @param warehouseName Name of the warehouse to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional WarehouseDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::WarehouseDTO> getWarehouseByName(
        const std::string& warehouseName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all warehouses or warehouses matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching WarehouseDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::WarehouseDTO> getAllWarehouses(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates warehouse information.
     * @param warehouseDTO DTO containing updated warehouse information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateWarehouse(
        const ERP::Catalog::DTO::WarehouseDTO& warehouseDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a warehouse.
     * @param warehouseId ID of the warehouse.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateWarehouseStatus(
        const std::string& warehouseId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a warehouse record by ID (soft delete).
     * @param warehouseId ID of the warehouse to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteWarehouse(
        const std::string& warehouseId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of IWarehouseService.
 * This class uses WarehouseDAO and ISecurityManager.
 */
class WarehouseService : public IWarehouseService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for WarehouseService.
     * @param warehouseDAO Shared pointer to WarehouseDAO.
     * @param locationService Shared pointer to ILocationService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    WarehouseService(std::shared_ptr<DAOs::WarehouseDAO> warehouseDAO,
                     std::shared_ptr<ILocationService> locationService, // Use interface
                     std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                     std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                     std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                     std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Catalog::DTO::WarehouseDTO> createWarehouse(
        const ERP::Catalog::DTO::WarehouseDTO& warehouseDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Catalog::DTO::WarehouseDTO> getWarehouseById(
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Catalog::DTO::WarehouseDTO> getWarehouseByName(
        const std::string& warehouseName,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Catalog::DTO::WarehouseDTO> getAllWarehouses(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateWarehouse(
        const ERP::Catalog::DTO::WarehouseDTO& warehouseDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateWarehouseStatus(
        const std::string& warehouseId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteWarehouse(
        const std::string& warehouseId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::WarehouseDAO> warehouseDAO_;
    std::shared_ptr<ILocationService> locationService_; // For location validation
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_WAREHOUSESERVICE_H