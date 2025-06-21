// Modules/Warehouse/DTO/PickingRequest.h
#ifndef MODULES_WAREHOUSE_DTO_PICKINGREQUEST_H
#define MODULES_WAREHOUSE_DTO_PICKINGREQUEST_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include <vector>       // For nested DTOs (PickingDetailDTO)

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)
#include "PickingDetail.h" // NEW: Include PickingDetail DTO (previously PickingListItem)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
    namespace Warehouse {
        namespace DTO {
            /**
             * @brief Enum defining Picking Request Status.
             */
            enum class PickingRequestStatus {
                PENDING = 0,        /**< Yêu cầu lấy hàng đã được tạo, chờ xử lý. */
                IN_PROGRESS = 1,    /**< Đang trong quá trình lấy hàng. */
                COMPLETED = 2,      /**< Tất cả các mặt hàng đã được lấy. */
                CANCELLED = 3,      /**< Yêu cầu lấy hàng đã bị hủy. */
                PARTIALLY_PICKED = 4, /**< Một phần các mặt hàng đã được lấy. */
                UNKNOWN = 99        /**< Trạng thái không xác định. */
            };

            /**
             * @brief DTO for Picking Request entity.
             * Represents a request to pick items from inventory for a sales order or internal transfer.
             * (Previously named PickingListDTO, now unified as PickingRequestDTO)
             */
            struct PickingRequestDTO : public BaseDTO {
                std::string salesOrderId;           /**< Foreign key to SalesOrderDTO (nếu là lấy hàng cho đơn bán). */
                std::string warehouseId;            /**< Kho hàng nơi lấy hàng. */
                std::string requestNumber;          /**< Số yêu cầu lấy hàng duy nhất. */
                std::chrono::system_clock::time_point requestDate; /**< Ngày yêu cầu lấy hàng. */
                std::optional<std::string> requestedByUserId; /**< ID người dùng yêu cầu lấy hàng (tùy chọn). */
                std::optional<std::string> assignedToUserId; /**< ID người dùng/nhân viên được giao lấy hàng (tùy chọn). */
                PickingRequestStatus status;         /**< Trạng thái của yêu cầu lấy hàng. */
                std::optional<std::string> pickStartTime; /**< Thời gian bắt đầu lấy hàng (tùy chọn). */
                std::optional<std::string> pickEndTime; /**< Thời gian kết thúc lấy hàng (tùy chọn). */
                std::optional<std::string> notes;   /**< Ghi chú về yêu cầu lấy hàng (tùy chọn). */

                std::vector<PickingDetailDTO> details; /**< NEW: Nested: Details of the items to pick. */

                // Default constructor
                PickingRequestDTO() : BaseDTO(), status(PickingRequestStatus::PENDING) {}

                // Virtual destructor for proper polymorphic cleanup
                virtual ~PickingRequestDTO() = default;

                /**
                 * @brief Converts a PickingRequestStatus enum value to its string representation.
                 * @return The string representation of the picking request status.
                 */
                std::string getStatusString() const {
                    switch (status) {
                    case PickingRequestStatus::PENDING: return "Pending";
                    case PickingRequestStatus::IN_PROGRESS: return "In Progress";
                    case PickingRequestStatus::COMPLETED: return "Completed";
                    case PickingRequestStatus::CANCELLED: return "Cancelled";
                    case PickingRequestStatus::PARTIALLY_PICKED: return "Partially Picked";
                    case PickingRequestStatus::UNKNOWN: return "Unknown";
                    default: return "Unknown";
                    }
                }
            };
        } // namespace DTO
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DTO_PICKINGREQUEST_H