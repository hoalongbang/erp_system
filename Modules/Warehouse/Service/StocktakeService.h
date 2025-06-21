// Modules/Warehouse/Service/StocktakeService.h
#ifndef MODULES_WAREHOUSE_SERVICE_STOCKTAKESERVICE_H
#define MODULES_WAREHOUSE_SERVICE_STOCKTAKESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

// Rút gọn các include paths
#include "BaseService.h"        // Base Service
#include "StocktakeRequest.h"   // StocktakeRequest DTO
#include "StocktakeDetail.h"    // StocktakeDetail DTO
#include "Product.h"            // Product DTO
#include "Warehouse.h"          // Warehouse DTO
#include "Location.h"           // Location DTO
#include "StocktakeRequestDAO.h"// StocktakeRequest DAO
#include "StocktakeDetailDAO.h" // NEW: StocktakeDetailDAO
#include "InventoryManagementService.h" // Inventory Management Service interface (dependency)
#include "WarehouseService.h"   // Warehouse Service interface (dependency)
#include "ProductService.h"     // Product Service interface (dependency)
#include "ISecurityManager.h"   // Security Manager interface
#include "EventBus.h"           // EventBus
#include "Logger.h"             // Logger
#include "ErrorHandler.h"       // ErrorHandler
#include "Common.h"             // Common enums/constants
#include "Utils.h"              // Utilities
#include "DateUtils.h"          // Date utilities

// Forward declarations for services that are dependencies but might cause circular includes
namespace ERP { namespace Catalog { namespace Services { class IWarehouseService; } } }
namespace ERP { namespace Product { namespace Services { class IProductService; } } }
namespace ERP { namespace Warehouse { namespace Services { class IInventoryManagementService; } } }

namespace ERP {
    namespace Warehouse {
        namespace Services {

            /**
             * @brief IStocktakeService interface defines operations for managing stocktake requests.
             */
            class IStocktakeService {
            public:
                virtual ~IStocktakeService() = default;
                /**
                 * @brief Creates a new stocktake request.
                 * @param stocktakeRequestDTO DTO containing new request information.
                 * @param stocktakeDetails Vector of StocktakeDetailDTOs to pre-populate (optional).
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional StocktakeRequestDTO if creation is successful, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> createStocktakeRequest(
                    const ERP::Warehouse::DTO::StocktakeRequestDTO& stocktakeRequestDTO,
                    const std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO>& stocktakeDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves stocktake request information by ID.
                 * @param requestId ID of the request to retrieve.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional StocktakeRequestDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> getStocktakeRequestById(
                    const std::string& requestId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves all stocktake requests or requests matching a filter.
                 * @param filter Map of filter conditions.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return Vector of matching StocktakeRequestDTOs.
                 */
                virtual std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> getAllStocktakeRequests(
                    const std::map<std::string, std::any>& filter = {},
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves stocktake requests by warehouse or location.
                 * @param warehouseId ID of the warehouse.
                 * @param locationId Optional: ID of the location within the warehouse.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return Vector of matching StocktakeRequestDTOs.
                 */
                virtual std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> getStocktakeRequestsByWarehouseLocation(
                    const std::string& warehouseId,
                    const std::optional<std::string>& locationId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Updates stocktake request information.
                 * @param stocktakeRequestDTO DTO containing updated request information (must have ID).
                 * @param stocktakeDetails Vector of StocktakeDetailDTOs for the request (full replacement).
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if update is successful, false otherwise.
                 */
                virtual bool updateStocktakeRequest(
                    const ERP::Warehouse::DTO::StocktakeRequestDTO& stocktakeRequestDTO,
                    const std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO>& stocktakeDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Updates the status of a stocktake request.
                 * @param requestId ID of the request.
                 * @param newStatus New status.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if status update is successful, false otherwise.
                 */
                virtual bool updateStocktakeRequestStatus(
                    const std::string& requestId,
                    ERP::Warehouse::DTO::StocktakeRequestStatus newStatus,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Deletes a stocktake request record by ID (soft delete).
                 * @param requestId ID of the request to delete.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if deletion is successful, false otherwise.
                 */
                virtual bool deleteStocktakeRequest(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves a specific stocktake detail by ID.
                 * @param detailId ID of the detail.
                 * @param currentUserId ID of the user making the request.
                 * @param userRoleIds Roles of the user making the request.
                 * @return An optional StocktakeDetailDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Warehouse::DTO::StocktakeDetailDTO> getStocktakeDetailById(
                    const std::string& detailId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves all details for a specific stocktake request.
                 * @param requestId ID of the stocktake request.
                 * @param currentUserId ID of the user making the request.
                 * @param userRoleIds Roles of the user making the request.
                 * @return Vector of StocktakeDetailDTOs.
                 */
                virtual std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> getStocktakeDetails(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Records actual counted quantity for a specific stocktake detail.
                 * @param detailId ID of the detail.
                 * @param countedQuantity Actual quantity counted.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if successful, false otherwise.
                 */
                virtual bool recordCountedQuantity(
                    const std::string& detailId,
                    double countedQuantity,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Reconciles a completed stocktake request, posting inventory adjustments based on differences.
                 * @param requestId ID of the stocktake request to reconcile.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if reconciliation is successful, false otherwise.
                 */
                virtual bool reconcileStocktake(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
            };

            /**
             * @brief Default implementation of IStocktakeService.
             * This class uses StocktakeRequestDAO, StocktakeDetailDAO and ISecurityManager.
             */
            class StocktakeService : public IStocktakeService, public ERP::Common::Services::BaseService {
            public:
                /**
                 * @brief Constructor for StocktakeService.
                 * @param stocktakeRequestDAO Shared pointer to StocktakeRequestDAO.
                 * @param stocktakeDetailDAO Shared pointer to StocktakeDetailDAO (NEW dependency).
                 * @param inventoryManagementService Shared pointer to IInventoryManagementService (dependency).
                 * @param warehouseService Shared pointer to IWarehouseService (dependency).
                 * @param productService Shared pointer to IProductService (dependency).
                 * @param authorizationService Shared pointer to IAuthorizationService.
                 * @param auditLogService Shared pointer to IAuditLogService.
                 * @param connectionPool Shared pointer to ConnectionPool.
                 * @param securityManager Shared pointer to ISecurityManager.
                 */
                StocktakeService(std::shared_ptr<DAOs::StocktakeRequestDAO> stocktakeRequestDAO,
                    std::shared_ptr<DAOs::StocktakeDetailDAO> stocktakeDetailDAO, // NEW
                    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
                    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                    std::shared_ptr<ERP::Product::Services::IProductService> productService,
                    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                    std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> createStocktakeRequest(
                    const ERP::Warehouse::DTO::StocktakeRequestDTO& stocktakeRequestDTO,
                    const std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO>& stocktakeDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> getStocktakeRequestById(
                    const std::string& requestId,
                    const std::vector<std::string>& userRoleIds = {}) override;
                std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> getAllStocktakeRequests(
                    const std::map<std::string, std::any>& filter = {},
                    const std::vector<std::string>& userRoleIds = {}) override;
                std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> getStocktakeRequestsByWarehouseLocation(
                    const std::string& warehouseId,
                    const std::optional<std::string>& locationId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool updateStocktakeRequest(
                    const ERP::Warehouse::DTO::StocktakeRequestDTO& stocktakeRequestDTO,
                    const std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO>& stocktakeDetails,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool updateStocktakeRequestStatus(
                    const std::string& requestId,
                    ERP::Warehouse::DTO::StocktakeRequestStatus newStatus,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool deleteStocktakeRequest(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                std::optional<ERP::Warehouse::DTO::StocktakeDetailDTO> getStocktakeDetailById(
                    const std::string& detailId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> getStocktakeDetails(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool recordCountedQuantity(
                    const std::string& detailId,
                    double countedQuantity,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;
                bool reconcileStocktake(
                    const std::string& requestId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) override;

            private:
                std::shared_ptr<DAOs::StocktakeRequestDAO> stocktakeRequestDAO_;
                std::shared_ptr<DAOs::StocktakeDetailDAO> stocktakeDetailDAO_; // NEW
                std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
                std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
                std::shared_ptr<ERP::Product::Services::IProductService> productService_;
                // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

                // EventBus is typically accessed as a singleton.
                ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
            };
        } // namespace Services
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_SERVICE_STOCKTAKESERVICE_H