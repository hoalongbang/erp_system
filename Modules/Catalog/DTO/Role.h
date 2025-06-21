// Modules/Catalog/DTO/Role.h
#ifndef MODULES_CATALOG_DTO_ROLE_H
#define MODULES_CATALOG_DTO_ROLE_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include <vector>       // For permissions (though permissions are often managed via a join table)

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên class cơ sở

namespace ERP {
    namespace Catalog {
        namespace DTO {
            /**
             * @brief DTO for Role entity.
             * Represents a user role in the system (e.g., "Administrator", "Sales Manager").
             */
            struct RoleDTO : public BaseDTO {
                std::string name;                               /**< Tên vai trò (ví dụ: "Admin"). */
                std::optional<std::string> description;         /**< Mô tả vai trò (tùy chọn). */

                // Note: Permissions are typically linked via a separate many-to-many relationship table (RolePermissions).
                // They are not directly part of the RoleDTO itself for persistence, but might be loaded
                // into a std::set<std::string> of permission names when a role's permissions are queried.
                // std::set<std::string> permissions; // Example if loaded into DTO for runtime use

                // Default constructor
                RoleDTO() = default;

                // Virtual destructor for proper polymorphic cleanup
                virtual ~RoleDTO() = default;
            };
        } // namespace DTO
    } // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DTO_ROLE_H