// Modules/Product/DTO/ProductUnitConversion.h
#ifndef MODULES_PRODUCT_DTO_PRODUCTUNITCONVERSION_H
#define MODULES_PRODUCT_DTO_PRODUCTUNITCONVERSION_H

#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
    namespace Product {
        namespace DTO {

            /**
             * @brief DTO for Product Unit Conversion entity.
             * Defines conversion rules between a product's base unit of measure and an alternative unit.
             */
            struct ProductUnitConversionDTO : public BaseDTO {
                std::string productId;                      /**< Foreign key to ProductDTO (sản phẩm áp dụng quy tắc). */
                std::string fromUnitOfMeasureId;            /**< Foreign key to UnitOfMeasureDTO (đơn vị gốc). */
                std::string toUnitOfMeasureId;              /**< Foreign key to UnitOfMeasureDTO (đơn vị thay thế). */
                double conversionFactor = 0.0;              /**< Hệ số chuyển đổi từ đơn vị gốc sang đơn vị thay thế (ví dụ: 1 hộp = 10 cái, factor = 10). */
                std::optional<std::string> notes;           /**< Ghi chú về quy tắc chuyển đổi (tùy chọn). */

                // Default constructor
                ProductUnitConversionDTO() : conversionFactor(0.0) {} // Initialize conversionFactor

                // Virtual destructor for proper polymorphic cleanup
                virtual ~ProductUnitConversionDTO() = default;
            };

        } // namespace DTO
    } // namespace Product
} // namespace ERP

#endif // MODULES_PRODUCT_DTO_PRODUCTUNITCONVERSION_H