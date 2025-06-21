// Modules/Catalog/DTO/Permission.h
#ifndef MODULES_CATALOG_DTO_PERMISSION_H
#define MODULES_CATALOG_DTO_PERMISSION_H
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
             * @brief DTO for Permission entity.
             * Represents a specific permission in the system (e.g., "User.Create", "Product.View").
             */
            struct PermissionDTO : public BaseDTO {
                std::string name;                               /**< Tên quyền hạn (ví dụ: "Sales.CreateOrder"). */
                std::string module;                             /**< Module mà quyền hạn thuộc về (ví dụ: "Sales", "Inventory"). */
                std::string action;                             /**< Hành động mà quyền hạn cho phép (ví dụ: "CreateOrder", "ViewProducts"). */
                std::optional<std::string> description;         /**< Mô tả chi tiết quyền hạn (tùy chọn). */

                // Default constructor
                PermissionDTO() = default;

                // Virtual destructor for proper polymorphic cleanup
                virtual ~PermissionDTO() = default;
            };
        } // namespace DTO
    } // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DTO_PERMISSION_H