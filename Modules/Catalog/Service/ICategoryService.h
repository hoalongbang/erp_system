// Modules/Catalog/Service/ICategoryService.h
#ifndef MODULES_CATALOG_SERVICE_ICATEGORYSERVICE_H
#define MODULES_CATALOG_SERVICE_ICATEGORYSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "Category.h"      // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Catalog {
namespace Services {

/**
 * @brief ICategoryService interface defines operations for managing product categories.
 */
class ICategoryService {
public:
    virtual ~ICategoryService() = default;
    /**
     * @brief Creates a new product category.
     * @param categoryDTO DTO containing new category information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional CategoryDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::CategoryDTO> createCategory(
        const ERP::Catalog::DTO::CategoryDTO& categoryDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves category information by ID.
     * @param categoryId ID of the category to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional CategoryDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::CategoryDTO> getCategoryById(
        const std::string& categoryId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves category information by name.
     * @param categoryName Name of the category to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional CategoryDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::CategoryDTO> getCategoryByName(
        const std::string& categoryName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all categories or categories matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching CategoryDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::CategoryDTO> getAllCategories(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates category information.
     * @param categoryDTO DTO containing updated category information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateCategory(
        const ERP::Catalog::DTO::CategoryDTO& categoryDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a category.
     * @param categoryId ID of the category.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateCategoryStatus(
        const std::string& categoryId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a category record by ID (soft delete).
     * @param categoryId ID of the category to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteCategory(
        const std::string& categoryId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_ICATEGORYSERVICE_H