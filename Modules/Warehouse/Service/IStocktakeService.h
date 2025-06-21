// Modules/Warehouse/Service/IStocktakeService.h
#ifndef MODULES_WAREHOUSE_SERVICE_ISTOCKTAKESERVICE_H
#define MODULES_WAREHOUSE_SERVICE_ISTOCKTAKESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "BaseService.h"        // Base Service
#include "StocktakeRequest.h"   // StocktakeRequest DTO
#include "StocktakeDetail.h"    // StocktakeDetail DTO

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

        } // namespace Services
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_SERVICE_ISTOCKTAKESERVICE_H