// Modules/Sales/Service/SalesReturnService.h
#ifndef MODULES_SALES_SERVICE_SALESRETURNSERVICE_H
#define MODULES_SALES_SERVICE_SALESRETURNSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

// Rút gọn các include paths
#include "BaseService.h"        // Base Service
#include "Return.h"             // Return DTO (includes ReturnDetail)
#include "SalesOrder.h"         // SalesOrder DTO
#include "Product.h"            // Product DTO
#include "Warehouse.h"          // Warehouse DTO
#include "Location.h"           // Location DTO
#include "ReturnDAO.h"          // Return DAO
#include "SalesOrderService.h"  // SalesOrder Service interface (dependency)
#include "CustomerService.h"    // Customer Service interface (dependency)
#include "WarehouseService.h"   // Warehouse Service interface (dependency)
#include "ProductService.h"     // Product Service interface (dependency)
#include "InventoryManagementService.h" // Inventory Management Service interface (dependency)
#include "ISecurityManager.h"   // Security Manager interface
#include "EventBus.h"           // EventBus
#include "Logger.h"             // Logger
#include "ErrorHandler.h"       // ErrorHandler
#include "Common.h"             // Common enums/constants
#include "Utils.h"              // Utilities
#include "DateUtils.h"          // Date utilities

// Forward declarations for services that are dependencies but might cause circular includes
namespace ERP { namespace Customer { namespace Services { class ICustomerService; } } }
namespace ERP { namespace Sales { namespace Services { class ISalesOrderService; } } }
namespace ERP { namespace Catalog { namespace Services { class IWarehouseService; } } }
namespace ERP { namespace Product { namespace Services { class IProductService; } } }
namespace ERP { namespace Warehouse { namespace Services { class IInventoryManagementService; } } }

namespace ERP {
    namespace Sales {
        namespace Services {

            /**
             * @brief IReturnService interface defines operations for managing sales returns.
             */
            class IReturnService {
            public:
                virtual ~IReturnService() = default;
                /**
                 * @brief Creates a new sales return.
                 * @param returnDTO DTO containing new return information.
                 * @param returnDetails Vector of ReturnDetailDTOs for the return.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional ReturnDTO if creation is successful, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Sales::DTO::ReturnDTO> createReturn(
                    const ERP::Sales::DTO::ReturnDTO& returnDTO,
                    const std::vector<ERP::Sales::DTO::ReturnDetailDTO>& returnDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves return information by ID.
                 * @param returnId ID of the return to retrieve.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional ReturnDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Sales::DTO::ReturnDTO> getReturnById(
                    const std::string& returnId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves all returns or returns matching a filter.
                 * @param filter Map of filter conditions.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return Vector of matching ReturnDTOs.
                 */
                virtual std::vector<ERP::Sales::DTO::ReturnDTO> getAllReturns(
                    const std::map<std::string, std::any>& filter = {},
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Updates return information.
                 * @param returnDTO DTO containing updated return information (must have ID).
                 * @param returnDetails Vector of ReturnDetailDTOs for the return (full replacement).
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if update is successful, false otherwise.
                 */
                virtual bool updateReturn(
                    const ERP::Sales::DTO::ReturnDTO& returnDTO,
                    const std::vector<ERP::Sales::DTO::ReturnDetailDTO>& returnDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Updates the status of a return.
                 * @param returnId ID of the return.
                 * @param newStatus New status.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if status update is successful, false otherwise.
                 */
                virtual bool updateReturnStatus(
                    const std::string& returnId,
                    ERP::Sales::DTO::ReturnStatus newStatus,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Deletes a return record by ID (soft delete).
                 * @param returnId ID of the return to delete.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if deletion is successful, false otherwise.
                 */
                virtual bool deleteReturn(
                    const std::string& returnId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves all details for a specific return.
                 * @param returnId ID of the return.
                 * @param userRoleIds Roles of the user making the request.
                 * @return Vector of ReturnDetailDTOs.
                 */
                virtual std::vector<ERP::Sales::DTO::ReturnDetailDTO> getReturnDetails(
                    const std::string& returnId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
            };

            /**
             * @brief Default implementation of IReturnService.
             * This class uses ReturnDAO and ISecurityManager.
             */
            class SalesReturnService : public IReturnService, public ERP::Common::Services::BaseService {
            public:
                /**
                 * @brief Constructor for SalesReturnService.
                 * @param returnDAO Shared pointer to ReturnDAO.
                 * @param salesOrderService Shared pointer to ISalesOrderService (dependency).
                 * @param customerService Shared pointer to ICustomerService (dependency).
                 * @param warehouseService Shared pointer to IWarehouseService (dependency).
                 * @param productService Shared pointer to IProductService (dependency).
                 * @param inventoryManagementService Shared pointer to IInventoryManagementService (dependency).
                 * @param authorizationService Shared pointer to IAuthorizationService.
                 * @param auditLogService Shared pointer to IAuditLogService.
                 * @param connectionPool Shared pointer to ConnectionPool.
                 * @param securityManager Shared pointer to ISecurityManager.
                 */
                SalesReturnService(std::shared_ptr<DAOs::ReturnDAO> returnDAO,
                    std::shared_ptr<ERP::Sales::Services::ISalesOrderService> salesOrderService,
                    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
                    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                    std::shared_ptr<ERP::Product::Services::IProductService> productService,
                    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
                    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                    std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

                std::optional<ERP::Sales::DTO::ReturnDTO> createReturn(
                    const ERP::Sales::DTO::ReturnDTO& returnDTO,
                    const std::vector<ERP::Sales::DTO::ReturnDetailDTO>& returnDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                std::optional<ERP::Sales::DTO::ReturnDTO> getReturnById(
                    const std::string& returnId,
                    const std::vector<std::string>& userRoleIds = {}) override;
                std::vector<ERP::Sales::DTO::ReturnDTO> getAllReturns(
                    const std::map<std::string, std::any>& filter = {},
                    const std::vector<std::string>& userRoleIds = {}) override;
                bool updateReturn(
                    const ERP::Sales::DTO::ReturnDTO& returnDTO,
                    const std::vector<ERP::Sales::DTO::ReturnDetailDTO>& returnDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool updateReturnStatus(
                    const std::string& returnId,
                    ERP::Sales::DTO::ReturnStatus newStatus,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool deleteReturn(
                    const std::string& returnId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                std::vector<ERP::Sales::DTO::ReturnDetailDTO> getReturnDetails(
                    const std::string& returnId,
                    const std::vector<std::string>& userRoleIds = {}) override;

            private:
                std::shared_ptr<DAOs::ReturnDAO> returnDAO_;
                std::shared_ptr<ERP::Sales::Services::ISalesOrderService> salesOrderService_;
                std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService_;
                std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
                std::shared_ptr<ERP::Product::Services::IProductService> productService_;
                std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
                // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

                // EventBus is typically accessed as a singleton.
                ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
            };
        } // namespace Services
    } // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_SALESRETURNSERVICE_H