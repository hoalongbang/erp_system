// Modules/Warehouse/Service/PickingService.h
#ifndef MODULES_WAREHOUSE_SERVICE_PICKINGSERVICE_H
#define MODULES_WAREHOUSE_SERVICE_PICKINGSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

// Rút gọn các include paths
#include "BaseService.h"        // Base Service
#include "PickingRequest.h"     // PickingRequest DTO (renamed from PickingList)
#include "PickingDetail.h"      // PickingDetail DTO (renamed from PickingListItem)
#include "SalesOrder.h"         // SalesOrder DTO
#include "Product.h"            // Product DTO
#include "Warehouse.h"          // Warehouse DTO
#include "Location.h"           // Location DTO
#include "PickingRequestDAO.h"  // PickingRequest DAO (renamed from PickingListDAO)
#include "PickingDetailDAO.h"   // NEW: PickingDetailDAO (renamed from PickingDetailDAO, but for PickingDetailDTO)
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
    namespace Warehouse {
        namespace Services {

            /**
             * @brief IPickingService interface defines operations for managing picking requests.
             */
            class IPickingService {
            public:
                virtual ~IPickingService() = default;
                /**
                 * @brief Creates a new picking request.
                 * @param pickingRequestDTO DTO containing new request information.
                 * @param pickingDetails Vector of PickingDetailDTOs for the request.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional PickingRequestDTO if creation is successful, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Warehouse::DTO::PickingRequestDTO> createPickingRequest(
                    const ERP::Warehouse::DTO::PickingRequestDTO& pickingRequestDTO,
                    const std::vector<ERP::Warehouse::DTO::PickingDetailDTO>& pickingDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves picking request information by ID.
                 * @param requestId ID of the request to retrieve.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional PickingRequestDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Warehouse::DTO::PickingRequestDTO> getPickingRequestById(
                    const std::string& requestId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves all picking requests or requests matching a filter.
                 * @param filter Map of filter conditions.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return Vector of matching PickingRequestDTOs.
                 */
                virtual std::vector<ERP::Warehouse::DTO::PickingRequestDTO> getAllPickingRequests(
                    const std::map<std::string, std::any>& filter = {},
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves picking requests by sales order ID.
                 * @param salesOrderId ID of the sales order.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return Vector of matching PickingRequestDTOs.
                 */
                virtual std::vector<ERP::Warehouse::DTO::PickingRequestDTO> getPickingRequestsBySalesOrderId(
                    const std::string& salesOrderId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Updates picking request information.
                 * @param pickingRequestDTO DTO containing updated request information (must have ID).
                 * @param pickingDetails Vector of PickingDetailDTOs for the request (full replacement).
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if update is successful, false otherwise.
                 */
                virtual bool updatePickingRequest(
                    const ERP::Warehouse::DTO::PickingRequestDTO& pickingRequestDTO,
                    const std::vector<ERP::Warehouse::DTO::PickingDetailDTO>& pickingDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Updates the status of a picking request.
                 * @param requestId ID of the request.
                 * @param newStatus New status.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if status update is successful, false otherwise.
                 */
                virtual bool updatePickingRequestStatus(
                    const std::string& requestId,
                    ERP::Warehouse::DTO::PickingRequestStatus newStatus,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Deletes a picking request record by ID (soft delete).
                 * @param requestId ID of the request to delete.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if deletion is successful, false otherwise.
                 */
                virtual bool deletePickingRequest(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves a specific picking detail by ID.
                 * @param detailId ID of the detail.
                 * @param currentUserId ID of the user making the request.
                 * @param userRoleIds Roles of the user making the request.
                 * @return An optional PickingDetailDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Warehouse::DTO::PickingDetailDTO> getPickingDetailById(
                    const std::string& detailId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves all details for a specific picking request.
                 * @param requestId ID of the picking request.
                 * @param currentUserId ID of the user making the request.
                 * @param userRoleIds Roles of the user making the request.
                 * @return Vector of PickingDetailDTOs.
                 */
                virtual std::vector<ERP::Warehouse::DTO::PickingDetailDTO> getPickingDetails(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Records actual picked quantity for a specific picking detail.
                 * This also creates an inventory transaction (goods issue).
                 * @param detailId ID of the detail.
                 * @param pickedQuantity Actual quantity picked.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if successful, false otherwise.
                 */
                virtual bool recordPickedQuantity(
                    const std::string& detailId,
                    double pickedQuantity,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
            };

            /**
             * @brief Default implementation of IPickingService.
             * This class uses PickingRequestDAO, PickingDetailDAO and ISecurityManager.
             */
            class PickingService : public IPickingService, public ERP::Common::Services::BaseService {
            public:
                /**
                 * @brief Constructor for PickingService.
                 * @param pickingRequestDAO Shared pointer to PickingRequestDAO.
                 * @param pickingDetailDAO Shared pointer to PickingDetailDAO (NEW dependency).
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
                PickingService(std::shared_ptr<DAOs::PickingRequestDAO> pickingRequestDAO,
                    std::shared_ptr<DAOs::PickingDetailDAO> pickingDetailDAO, // NEW
                    std::shared_ptr<ERP::Sales::Services::ISalesOrderService> salesOrderService,
                    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
                    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                    std::shared_ptr<ERP::Product::Services::IProductService> productService,
                    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
                    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                    std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

                std::optional<ERP::Warehouse::DTO::PickingRequestDTO> createPickingRequest(
                    const ERP::Warehouse::DTO::PickingRequestDTO& pickingRequestDTO,
                    const std::vector<ERP::Warehouse::DTO::PickingDetailDTO>& pickingDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                std::optional<ERP::Warehouse::DTO::PickingRequestDTO> getPickingRequestById(
                    const std::string& requestId,
                    const std::vector<std::string>& userRoleIds = {}) override;
                std::vector<ERP::Warehouse::DTO::PickingRequestDTO> getAllPickingRequests(
                    const std::map<std::string, std::any>& filter = {},
                    const std::vector<std::string>& userRoleIds = {}) override;
                std::vector<ERP::Warehouse::DTO::PickingRequestDTO> getPickingRequestsBySalesOrderId(
                    const std::string& salesOrderId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool updatePickingRequest(
                    const ERP::Warehouse::DTO::PickingRequestDTO& pickingRequestDTO,
                    const std::vector<ERP::Warehouse::DTO::PickingDetailDTO>& pickingDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool updatePickingRequestStatus(
                    const std::string& requestId,
                    ERP::Warehouse::DTO::PickingRequestStatus newStatus,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool deletePickingRequest(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                std::optional<ERP::Warehouse::DTO::PickingDetailDTO> getPickingDetailById(
                    const std::string& detailId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                std::vector<ERP::Warehouse::DTO::PickingDetailDTO> getPickingDetails(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool recordPickedQuantity(
                    const std::string& detailId,
                    double pickedQuantity,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;

            private:
                std::shared_ptr<DAOs::PickingRequestDAO> pickingRequestDAO_;
                std::shared_ptr<DAOs::PickingDetailDAO> pickingDetailDAO_; // NEW
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
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_SERVICE_PICKINGSERVICE_H