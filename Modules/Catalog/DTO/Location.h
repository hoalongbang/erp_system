// Modules/Catalog/DTO/Location.h
#ifndef MODULES_CATALOG_DTO_LOCATION_H
#define MODULES_CATALOG_DTO_LOCATION_H
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
             * @brief DTO for Location entity.
             * Represents a physical storage location within a warehouse.
             */
            struct LocationDTO : public BaseDTO {
                std::string warehouseId;                        /**< Foreign key to WarehouseDTO. */
                std::string name;                               /**< Tên vị trí (ví dụ: A1, B2-Shelf3). */
                std::optional<std::string> type;                /**< Loại vị trí (ví dụ: Pallet, Shelf, Bin) (tùy chọn). */
                std::optional<double> capacity;                 /**< Sức chứa của vị trí (ví dụ: mét khối, số pallet) (tùy chọn). */
                std::optional<std::string> unitOfCapacity;      /**< Đơn vị của sức chứa (ví dụ: CBM, Pallet) (tùy chọn). */
                std::optional<std::string> barcode;             /**< Mã vạch của vị trí (để quét nhanh) (tùy chọn). */

                // Default constructor
                LocationDTO() = default;

                // Virtual destructor for proper polymorphic cleanup
                virtual ~LocationDTO() = default;
            };
        } // namespace DTO
    } // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DTO_LOCATION_H