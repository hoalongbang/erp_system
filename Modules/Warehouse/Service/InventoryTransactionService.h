// Modules/Warehouse/Service/InventoryTransactionService.h
#ifndef MODULES_WAREHOUSE_SERVICES_INVENTORYTRANSACTIONSERVICE_H
#define MODULES_WAREHOUSE_SERVICES_INVENTORYTRANSACTIONSERVICE_H

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

// Rút gọn các include paths
#include "BaseService.h"               // NEW: Kế thừa từ BaseService
#include "InventoryTransaction.h"      // DTO
#include "InventoryTransactionDAO.h"   // DAO

// Forward declarations for services that are dependencies but might cause circular includes
namespace ERP { namespace Product { namespace Services { class IProductService; }}}
namespace ERP { namespace Catalog { namespace Services { class IWarehouseService; }}}
namespace ERP { namespace Catalog { namespace Services { class ILocationService; }}}

namespace ERP {
namespace Warehouse {
namespace Services {

/**
 * @brief IInventoryTransactionService interface defines operations for managing inventory transactions.
 * This service is primarily for recording and retrieving inventory movements.
 */
class IInventoryTransactionService {
public:
    virtual ~IInventoryTransactionService() = default;
    /**
     * @brief Creates a new inventory transaction record.
     * This method is typically called by other services (e.g., InventoryService)
     * after an inventory change has occurred.
     * @param transactionDTO DTO containing transaction details.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user.
     * @return An optional InventoryTransactionDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> createInventoryTransaction(
        const ERP::Warehouse::DTO::InventoryTransactionDTO& transactionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves inventory transaction information by ID.
     * @param transactionId ID of the transaction to retrieve.
     * @param userRoleIds Roles of the user.
     * @return An optional InventoryTransactionDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> getInventoryTransactionById(
        const std::string& transactionId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all inventory transactions matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user.
     * @return Vector of matching InventoryTransactionDTOs.
     */
    virtual std::vector<ERP::Warehouse::DTO::InventoryTransactionDTO> getAllInventoryTransactions(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
};

/**
 * @brief Default implementation of IInventoryTransactionService.
 */
class InventoryTransactionService : public IInventoryTransactionService, public ERP::Common::Services::BaseService { // Inherit BaseService
public:
    /**
     * @brief Constructor for InventoryTransactionService.
     * @param inventoryTransactionDAO Shared pointer to InventoryTransactionDAO.
     * @param productService Shared pointer to IProductService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param locationService Shared pointer to ILocationService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    InventoryTransactionService(std::shared_ptr<DAOs::InventoryTransactionDAO> inventoryTransactionDAO,
                                std::shared_ptr<ERP::Product::Services::IProductService> productService,
                                std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                                std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService,
                                std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                                std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                                std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                                std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> createInventoryTransaction(
        const ERP::Warehouse::DTO::InventoryTransactionDTO& transactionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Warehouse::DTO::InventoryTransactionDTO> getInventoryTransactionById(
        const std::string& transactionId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Warehouse::DTO::InventoryTransactionDTO> getAllInventoryTransactions(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;

private:
    std::shared_ptr<DAOs::InventoryTransactionDAO> inventoryTransactionDAO_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();

    // Removed checkUserPermission and getUserRoleIds as they are now in BaseService
};

} // namespace Services
} // namespace Warehouse
} // namespace ERP

#endif // MODULES_WAREHOUSE_SERVICES_INVENTORYTRANSACTIONSERVICE_H