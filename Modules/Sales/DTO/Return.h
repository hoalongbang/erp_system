// Modules/Sales/DTO/Return.h
#ifndef MODULES_SALES_DTO_RETURN_H
#define MODULES_SALES_DTO_RETURN_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include <vector>       // For nested DTOs (ReturnDetailDTO)

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)
#include "ReturnDetail.h" // NEW: Include ReturnDetail DTO for nested usage

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
    namespace Sales {
        namespace DTO {
            /**
             * @brief Enum defining return status.
             */
            enum class ReturnStatus {
                PENDING = 0,    /**< Yêu cầu trả hàng đang chờ xử lý. */
                RECEIVED = 1,   /**< Hàng hóa đã được nhận lại. */
                PROCESSED = 2,  /**< Yêu cầu trả hàng đã được xử lý (hoàn tiền/đổi hàng). */
                CANCELLED = 3,  /**< Yêu cầu trả hàng đã bị hủy. */
                UNKNOWN = 99    /**< Trạng thái không xác định. */
            };

            /**
             * @brief DTO for Return entity (Sales Return).
             * Represents a customer return of goods.
             */
            struct ReturnDTO : public BaseDTO {
                std::string salesOrderId;                   /**< Foreign key to original SalesOrderDTO. */
                std::string customerId;                     /**< Foreign key to CustomerDTO. */
                std::string returnNumber;                   /**< Số phiếu trả hàng duy nhất. */
                std::chrono::system_clock::time_point returnDate; /**< Ngày trả hàng. */
                std::optional<std::string> reason;          /**< Lý do trả hàng (tùy chọn). */
                double totalAmount = 0.0;                   /**< Tổng số tiền cần hoàn lại hoặc ghi có. */
                ReturnStatus status;                        /**< Trạng thái của yêu cầu trả hàng. */
                std::optional<std::string> warehouseId;     /**< Kho hàng nơi hàng hóa được trả về (tùy chọn). */
                std::optional<std::string> notes;           /**< Ghi chú về việc trả hàng (tùy chọn). */

                std::vector<ReturnDetailDTO> details;       /**< NEW: Nested: Details of the returned items. */

                // Default constructor
                ReturnDTO() : BaseDTO(), totalAmount(0.0), status(ReturnStatus::PENDING) {} // Default status to PENDING

                // Virtual destructor for proper polymorphic cleanup
                virtual ~ReturnDTO() = default;

                /**
                 * @brief Converts a ReturnStatus enum value to its string representation.
                 * @return The string representation of the return status.
                 */
                std::string getStatusString() const {
                    switch (status) {
                    case ReturnStatus::PENDING: return "Pending";
                    case ReturnStatus::RECEIVED: return "Received";
                    case ReturnStatus::PROCESSED: return "Processed";
                    case ReturnStatus::CANCELLED: return "Cancelled";
                    case ReturnStatus::UNKNOWN: return "Unknown";
                    default: return "Unknown";
                    }
                }
            };
        } // namespace DTO
    } // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_RETURN_H