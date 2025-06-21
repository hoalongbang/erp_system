// Modules/Catalog/DTO/UnitOfMeasure.h
#ifndef MODULES_CATALOG_DTO_UNITOFMEASURE_H
#define MODULES_CATALOG_DTO_UNITOFMEASURE_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên class cơ sở

namespace ERP {
    namespace Catalog {
        namespace DTO {
            /**
             * @brief DTO for UnitOfMeasure entity.
             * Represents a unit of measure (e.g., "Pcs", "Kg", "Meter").
             */
            struct UnitOfMeasureDTO : public BaseDTO {
                std::string name;                               /**< Tên đơn vị đo (ví dụ: "Kilogram"). */
                std::string symbol;                             /**< Ký hiệu đơn vị đo (ví dụ: "Kg"). */
                std::optional<std::string> description;         /**< Mô tả đơn vị đo (tùy chọn). */

                // Default constructor
                UnitOfMeasureDTO() = default;

                // Virtual destructor for proper polymorphic cleanup
                virtual ~UnitOfMeasureDTO() = default;
            };
        } // namespace DTO
    } // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DTO_UNITOFMEASURE_H