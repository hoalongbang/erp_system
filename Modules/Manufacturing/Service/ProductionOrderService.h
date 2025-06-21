// Modules/Manufacturing/Service/ProductionOrderService.h
#ifndef MODULES_MANUFACTURING_SERVICE_PRODUCTIONORDERSERVICE_H
#define MODULES_MANUFACTURING_SERVICE_PRODUCTIONORDERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "ProductionOrder.h"    // Đã rút gọn include
#include "ProductionOrderDAO.h" // Đã rút gọn include
#include "ProductService.h"     // For Product validation
#include "BillOfMaterialService.h" // For BOM validation
#include "ProductionLineService.h" // For ProductionLine validation
#include "ISecurityManager.h"   // Đã rút gọn include
#include "EventBus.h"           // Đã rút gọn include
#include "Logger.h"             // Đã rút gọn include
#include "ErrorHandler.h"       // Đã rút gọn include
#include "Common.h"             // Đã rút gọn include
#include "Utils.h"              // Đã rút gọn include
#include "DateUtils.h"          // Đã rút gọn include

namespace ERP {
namespace Manufacturing {
namespace Services {

// Forward declare if other services are only used via pointer/reference
// class IProductService;
// class IBillOfMaterialService;
// class IProductionLineService;

/**
 * @brief IProductionOrderService interface defines operations for managing production orders.
 */
class IProductionOrderService {
public:
    virtual ~IProductionOrderService() = default;
    /**
     * @brief Creates a new production order.
     * @param productionOrderDTO DTO containing new order information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionOrderDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> createProductionOrder(
        const ERP::Manufacturing::DTO::ProductionOrderDTO& productionOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves production order information by ID.
     * @param orderId ID of the production order to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionOrderDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrderById(
        const std::string& orderId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves production order information by order number.
     * @param orderNumber Order number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionOrderDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrderByNumber(
        const std::string& orderNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all production orders or orders matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ProductionOrderDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getAllProductionOrders(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves production orders by status.
     * @param status The status to filter by.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ProductionOrderDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrdersByStatus(
        ERP::Manufacturing::DTO::ProductionOrderStatus status,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves production orders by production line.
     * @param productionLineId The ID of the production line.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ProductionOrderDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrdersByProductionLine(
        const std::string& productionLineId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates production order information.
     * @param productionOrderDTO DTO containing updated order information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateProductionOrder(
        const ERP::Manufacturing::DTO::ProductionOrderDTO& productionOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a production order.
     * @param orderId ID of the order.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateProductionOrderStatus(
        const std::string& orderId,
        ERP::Manufacturing::DTO::ProductionOrderStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a production order record by ID (soft delete).
     * @param orderId ID of the order to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteProductionOrder(
        const std::string& orderId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records actual quantity produced for a production order.
     * @param orderId ID of the production order.
     * @param actualQuantityProduced Actual quantity produced.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if successful, false otherwise.
     */
    virtual bool recordActualQuantityProduced(
        const std::string& orderId,
        double actualQuantityProduced,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of IProductionOrderService.
 * This class uses ProductionOrderDAO, and other services for validation.
 */
class ProductionOrderService : public IProductionOrderService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for ProductionOrderService.
     * @param productionOrderDAO Shared pointer to ProductionOrderDAO.
     * @param productService Shared pointer to IProductService.
     * @param billOfMaterialService Shared pointer to IBillOfMaterialService.
     * @param productionLineService Shared pointer to IProductionLineService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    ProductionOrderService(std::shared_ptr<DAOs::ProductionOrderDAO> productionOrderDAO,
                           std::shared_ptr<ERP::Product::Services::IProductService> productService,
                           std::shared_ptr<IBillOfMaterialService> billOfMaterialService,
                           std::shared_ptr<IProductionLineService> productionLineService,
                           std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                           std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                           std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                           std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> createProductionOrder(
        const ERP::Manufacturing::DTO::ProductionOrderDTO& productionOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrderById(
        const std::string& orderId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrderByNumber(
        const std::string& orderNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getAllProductionOrders(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrdersByStatus(
        ERP::Manufacturing::DTO::ProductionOrderStatus status,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrdersByProductionLine(
        const std::string& productionLineId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateProductionOrder(
        const ERP::Manufacturing::DTO::ProductionOrderDTO& productionOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateProductionOrderStatus(
        const std::string& orderId,
        ERP::Manufacturing::DTO::ProductionOrderStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteProductionOrder(
        const std::string& orderId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool recordActualQuantityProduced(
        const std::string& orderId,
        double actualQuantityProduced,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::ProductionOrderDAO> productionOrderDAO_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    std::shared_ptr<IBillOfMaterialService> billOfMaterialService_;
    std::shared_ptr<IProductionLineService> productionLineService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_SERVICE_PRODUCTIONORDERSERVICE_H