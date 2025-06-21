// Modules/Sales/Service/SalesShipmentService.h
#ifndef MODULES_SALES_SERVICE_SALESSHIPMENTSERVICE_H
#define MODULES_SALES_SERVICE_SALESSHIPMENTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "Shipment.h"           // Đã rút gọn include
#include "ShipmentDAO.h"        // Đã rút gọn include
#include "SalesOrderService.h"  // For SalesOrder validation
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

// Forward declare if SalesOrderService/CustomerService/WarehouseService/ProductService are only used via pointer/reference
// class ISalesOrderService;
// class ICustomerService;
// class IWarehouseService;
// class IProductService;

/**
 * @brief ISalesShipmentService interface defines operations for managing sales shipments.
 */
class ISalesShipmentService {
public:
    virtual ~ISalesShipmentService() = default;
    /**
     * @brief Creates a new sales shipment.
     * @param shipmentDTO DTO containing new shipment information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ShipmentDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::ShipmentDTO> createShipment(
        const ERP::Sales::DTO::ShipmentDTO& shipmentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves shipment information by ID.
     * @param shipmentId ID of the shipment to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ShipmentDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::ShipmentDTO> getShipmentById(
        const std::string& shipmentId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves shipment information by shipment number.
     * @param shipmentNumber Shipment number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ShipmentDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::ShipmentDTO> getShipmentByNumber(
        const std::string& shipmentNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all shipments or shipments matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ShipmentDTOs.
     */
    virtual std::vector<ERP::Sales::DTO::ShipmentDTO> getAllShipments(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates shipment information.
     * @param shipmentDTO DTO containing updated shipment information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateShipment(
        const ERP::Sales::DTO::ShipmentDTO& shipmentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a shipment.
     * @param shipmentId ID of the shipment.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateShipmentStatus(
        const std::string& shipmentId,
        ERP::Sales::DTO::ShipmentStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a shipment record by ID.
     * @param shipmentId ID of the shipment to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteShipment(
        const std::string& shipmentId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of ISalesShipmentService.
 * This class uses ShipmentDAO, and other services for validation.
 */
class SalesShipmentService : public ISalesShipmentService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for SalesShipmentService.
     * @param shipmentDAO Shared pointer to ShipmentDAO.
     * @param salesOrderService Shared pointer to ISalesOrderService.
     * @param customerService Shared pointer to ICustomerService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param productService Shared pointer to IProductService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    SalesShipmentService(std::shared_ptr<DAOs::ShipmentDAO> shipmentDAO,
                         std::shared_ptr<ISalesOrderService> salesOrderService,
                         std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
                         std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                         std::shared_ptr<ERP::Product::Services::IProductService> productService,
                         std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                         std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                         std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                         std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Sales::DTO::ShipmentDTO> createShipment(
        const ERP::Sales::DTO::ShipmentDTO& shipmentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Sales::DTO::ShipmentDTO> getShipmentById(
        const std::string& shipmentId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Sales::DTO::ShipmentDTO> getShipmentByNumber(
        const std::string& shipmentNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Sales::DTO::ShipmentDTO> getAllShipments(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateShipment(
        const ERP::Sales::DTO::ShipmentDTO& shipmentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateShipmentStatus(
        const std::string& shipmentId,
        ERP::Sales::DTO::ShipmentStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteShipment(
        const std::string& shipmentId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::ShipmentDAO> shipmentDAO_;
    std::shared_ptr<ISalesOrderService> salesOrderService_;
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Old private helper functions removed as they are now in BaseService
};
} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_SALESSHIPMENTSERVICE_H