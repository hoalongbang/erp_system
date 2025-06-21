// Modules/Warehouse/Service/IPickingService.h
#ifndef MODULES_WAREHOUSE_SERVICE_IPICKINGSERVICE_H
#define MODULES_WAREHOUSE_SERVICE_IPICKINGSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "BaseService.h"           // Base Service
#include "PickingRequest.h"        // PickingRequest DTO (renamed from PickingList)
#include "PickingDetail.h"         // PickingDetail DTO (renamed from PickingListItem)

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

        } // namespace Services
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_SERVICE_IPICKINGSERVICE_H