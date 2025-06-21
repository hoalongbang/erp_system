// Modules/Sales/DTO/ReturnDetail.h
#ifndef MODULES_SALES_DTO_RETURNDETAIL_H
#define MODULES_SALES_DTO_RETURNDETAIL_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
    namespace Sales {
        namespace DTO {
            /**
             * @brief DTO for Return Detail entity.
             * Represents a single item line within a sales return.
             */
            struct ReturnDetailDTO : public BaseDTO {
                std::string returnId;               /**< Foreign key to ReturnDTO. */
                std::string productId;              /**< Foreign key to ProductDTO. */
                double quantity = 0.0;              /**< Số lượng sản phẩm được trả lại. */
                std::string unitOfMeasureId;        /**< Đơn vị đo của số lượng. */
                double unitPrice = 0.0;             /**< Đơn giá của sản phẩm được trả lại. */
                double refundedAmount = 0.0;        /**< Số tiền đã hoàn lại cho mặt hàng này. */
                std::optional<std::string> condition; /**< Tình trạng của mặt hàng được trả lại (ví dụ: "Mới", "Hư hỏng") (tùy chọn). */
                std::optional<std::string> notes;   /**< Ghi chú cho chi tiết trả hàng (tùy chọn). */
                std::optional<std::string> salesOrderDetailId; /**< ID chi tiết đơn hàng bán gốc (nếu có) (tùy chọn). */
                std::optional<std::string> inventoryTransactionId; /**< ID giao dịch tồn kho liên quan (tùy chọn). */

                // Default constructor
                ReturnDetailDTO() : BaseDTO(), quantity(0.0), unitPrice(0.0), refundedAmount(0.0) {}

                // Virtual destructor for proper polymorphic cleanup
                virtual ~ReturnDetailDTO() = default;
            };
        } // namespace DTO
    } // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DTO_RETURNDETAIL_H