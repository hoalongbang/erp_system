// Modules/Catalog/DTO/Warehouse.h
#ifndef MODULES_CATALOG_DTO_WAREHOUSE_H
#define MODULES_CATALOG_DTO_WAREHOUSE_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)
#include "AddressDTO.h" // For Address (optional, could be string or a complex DTO)
#include "ContactPersonDTO.h" // For ContactPerson (optional, could be string or a complex DTO)


using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên class cơ sở
// using ERP::DataObjects::AddressDTO; // If AddressDTO is common and used directly
// using ERP::DataObjects::ContactPersonDTO; // If ContactPersonDTO is common and used directly

namespace ERP {
    namespace Catalog {
        namespace DTO {
            /**
             * @brief DTO for Warehouse entity.
             */
            struct WarehouseDTO : public BaseDTO {
                std::string name;                               /**< Tên kho hàng. */
                std::optional<std::string> location;            /**< Địa điểm vật lý của kho hàng (địa chỉ, khu vực) (tùy chọn). */
                std::optional<std::string> contactPerson;       /**< Người liên hệ chính của kho hàng (tên) (tùy chọn). */
                std::optional<std::string> contactPhone;        /**< Số điện thoại liên hệ của kho hàng (tùy chọn). */
                std::optional<std::string> email;               /**< Email liên hệ của kho hàng (tùy chọn). */
                // std::optional<ERP::DataObjects::AddressDTO> address; // Example for complex address DTO
                // std::optional<ERP::DataObjects::ContactPersonDTO> primaryContact; // Example for complex contact DTO

                // Default constructor
                WarehouseDTO() = default;

                // Virtual destructor for proper polymorphic cleanup
                virtual ~WarehouseDTO() = default;
            };
        } // namespace DTO
    } // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DTO_WAREHOUSE_H