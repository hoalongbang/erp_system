// Modules/Sales/Service/SalesOrderService.h
#ifndef MODULES_SALES_SERVICE_SALESORDERSERVICE_H
#define MODULES_SALES_SERVICE_SALESORDERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "SalesOrder.h"         // Đã rút gọn include
#include "SalesOrderDAO.h"      // Đã rút gọn include
#include "CustomerService.h"    // For Customer validation
#include "WarehouseService.h"   // For Warehouse validation
#include "ProductService.h"     // For Product validation
#include "ISecurityManager.h"   // Đã rút gọn include
#include "EventBus.h"           // Đã rút gọn include
#include "Logger.h"             // Đã rút gọn include
#include "ErrorHandler.h"       // Đã rút gọn include
#include "Common.h"             // Đã rút gọn include
#include "Utils.h"              // Đã rút gọn include
#include "DateUtils.h"          // Đã rút gọn include

namespace ERP {
namespace Sales {
namespace Services {

// Forward declare if CustomerService/WarehouseService/ProductService are only used via pointer/reference
// class ICustomerService;
// class IWarehouseService;
// class IProductService;

/**
 * @brief ISalesOrderService interface defines operations for managing sales orders.
 */
class ISalesOrderService {
public:
    virtual ~ISalesOrderService() = default;
    /**
     * @brief Creates a new sales order.
     * @param salesOrderDTO DTO containing new sales order information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SalesOrderDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::SalesOrderDTO> createSalesOrder(
        const ERP::Sales::DTO::SalesOrderDTO& salesOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves sales order information by ID.
     * @param salesOrderId ID of the sales order to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SalesOrderDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::SalesOrderDTO> getSalesOrderById(
        const std::string& salesOrderId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves sales order information by order number.
     * @param orderNumber Order number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SalesOrderDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::SalesOrderDTO> getSalesOrderByNumber(
        const std::string& orderNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all sales orders or sales orders matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching SalesOrderDTOs.
     */
    virtual std::vector<ERP::Sales::DTO::SalesOrderDTO> getAllSalesOrders(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates sales order information.
     * @param salesOrderDTO DTO containing updated sales order information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateSalesOrder(
        const ERP::Sales::DTO::SalesOrderDTO& salesOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a sales order.
     * @param salesOrderId ID of the sales order.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateSalesOrderStatus(
        const std::string& salesOrderId,
        ERP::Sales::DTO::SalesOrderStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a sales order record by ID.
     * @param salesOrderId ID of the sales order to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteSalesOrder(
        const std::string& salesOrderId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of ISalesOrderService.
 * This class uses SalesOrderDAO, and other services for validation.
 */
class SalesOrderService : public ISalesOrderService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for SalesOrderService.
     * @param salesOrderDAO Shared pointer to SalesOrderDAO.
     * @param customerService Shared pointer to ICustomerService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param productService Shared pointer to IProductService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    SalesOrderService(std::shared_ptr<DAOs::SalesOrderDAO> salesOrderDAO,
                      std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
                      std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                      std::shared_ptr<ERP::Product::Services::IProductService> productService,
                      std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                      std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                      std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                      std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Sales::DTO::SalesOrderDTO> createSalesOrder(
        const ERP::Sales::DTO::SalesOrderDTO& salesOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Sales::DTO::SalesOrderDTO> getSalesOrderById(
        const std::string& salesOrderId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Sales::DTO::SalesOrderDTO> getSalesOrderByNumber(
        const std::string& orderNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Sales::DTO::SalesOrderDTO> getAllSalesOrders(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateSalesOrder(
        const ERP::Sales::DTO::SalesOrderDTO& salesOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateSalesOrderStatus(
        const std::string& salesOrderId,
        ERP::Sales::DTO::SalesOrderStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteSalesOrder(
        const std::string& salesOrderId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::SalesOrderDAO> salesOrderDAO_;
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Old private helper functions removed as they are now in BaseService
};
} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_SALESORDERSERVICE_H