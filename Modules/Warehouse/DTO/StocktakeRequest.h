// Modules/Warehouse/DTO/StocktakeRequest.h
#ifndef MODULES_WAREHOUSE_DTO_STOCKTAKEREQUEST_H
#define MODULES_WAREHOUSE_DTO_STOCKTAKEREQUEST_H
#include <string>
#include <optional>
#include <chrono>
#include "BaseDTO.h"   // Đã rút gọn include
#include "Common.h"    // Đã rút gọn include
#include "Utils.h"     // Đã rút gọn include
namespace ERP {
    namespace Warehouse {
        namespace DTO {
            /**
             * @brief Enum for Stocktake Request Status.
             */
            enum class StocktakeRequestStatus {
                PENDING = 0,    /**< Đang chờ xử lý */
                IN_PROGRESS = 1,/**< Đang thực hiện kiểm kê */
                COUNTED = 2,    /**< Đã hoàn thành đếm, đang chờ đối chiếu */
                RECONCILED = 3, /**< Đã đối chiếu xong sự khác biệt */
                COMPLETED = 4,  /**< Đã hoàn thành (điều chỉnh tồn kho nếu cần) */
                CANCELLED = 5   /**< Đã hủy */
            };
            /**
             * @brief DTO for Stocktake Request entity.
             * Represents a request to perform a physical inventory count.
             */
            struct StocktakeRequestDTO : public BaseDTO {
                std::string warehouseId;        // Foreign key to WarehouseDTO
                std::optional<std::string> locationId; // Optional: specific location for the stocktake
                std::string requestedByUserId;  // User who requested the stocktake
                std::optional<std::string> countedByUserId; // User who performed the count
                std::chrono::system_clock::time_point countDate; // Date of the physical count
                StocktakeRequestStatus status;
                std::optional<std::string> notes;

                StocktakeRequestDTO() : BaseDTO(), warehouseId(""), requestedByUserId(""),
                                        countDate(std::chrono::system_clock::now()), status(StocktakeRequestStatus::PENDING) {} // Default constructor
                virtual ~StocktakeRequestDTO() = default;

                // Utility methods
                std::string getStatusString() const {
                    switch (status) {
                        case StocktakeRequestStatus::PENDING: return "Pending";
                        case StocktakeRequestStatus::IN_PROGRESS: return "In Progress";
                        case StocktakeRequestStatus::COUNTED: return "Counted";
                        case StocktakeRequestStatus::RECONCILED: return "Reconciled";
                        case StocktakeRequestStatus::COMPLETED: return "Completed";
                        case StocktakeRequestStatus::CANCELLED: return "Cancelled";
                        default: return "Unknown";
                    }
                }
            };
        } // namespace DTO
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DTO_STOCKTAKEREQUEST_H