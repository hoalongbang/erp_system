// Modules/Catalog/DTO/Category.h
#ifndef MODULES_CATALOG_DTO_CATEGORY_H
#define MODULES_CATALOG_DTO_CATEGORY_H
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
             * @brief DTO for Category entity.
             */
            struct CategoryDTO : public BaseDTO {
                std::string name;                               /**< Tên danh mục. */
                std::optional<std::string> description;         /**< Mô tả danh mục (tùy chọn). */
                std::optional<std::string> parentCategoryId;    /**< Foreign key to another CategoryDTO (danh mục cha) (tùy chọn). */
                int sortOrder = 0;                              /**< Thứ tự sắp xếp hiển thị. */
                bool isActive = true;                           /**< Trạng thái hoạt động của danh mục. */

                // Default constructor
                CategoryDTO() = default;

                // Virtual destructor for proper polymorphic cleanup
                virtual ~CategoryDTO() = default;

                /**
                 * @brief Chuyển đổi trạng thái EntityStatus sang chuỗi.
                 * This function is already in Common.h, so no need to duplicate here if Common.h is accessible.
                 * However, if you need a DTO-specific string conversion or to ensure this method is available
                 * directly on DTO instances without needing Common::entityStatusToString(status), you can keep it.
                 * For now, relying on Common.h helper.
                 */
                // std::string getStatusString() const {
                //     return ERP::Common::entityStatusToString(status);
                // }
            };
        } // namespace DTO
    } // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DTO_CATEGORY_H