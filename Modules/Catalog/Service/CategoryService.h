// Modules/Catalog/Service/CategoryService.h
#ifndef MODULES_CATALOG_SERVICE_CATEGORYSERVICE_H
#define MODULES_CATALOG_SERVICE_CATEGORYSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Category.h"         // Đã rút gọn include
#include "CategoryDAO.h"      // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

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
/**
 * @brief Default implementation of ICategoryService.
 * This class uses CategoryDAO and ISecurityManager.
 */
class CategoryService : public ICategoryService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for CategoryService.
     * @param categoryDAO Shared pointer to CategoryDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    CategoryService(std::shared_ptr<DAOs::CategoryDAO> categoryDAO,
                    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                    std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Catalog::DTO::CategoryDTO> createCategory(
        const ERP::Catalog::DTO::CategoryDTO& categoryDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Catalog::DTO::CategoryDTO> getCategoryById(
        const std::string& categoryId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Catalog::DTO::CategoryDTO> getCategoryByName(
        const std::string& categoryName,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Catalog::DTO::CategoryDTO> getAllCategories(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateCategory(
        const ERP::Catalog::DTO::CategoryDTO& categoryDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateCategoryStatus(
        const std::string& categoryId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteCategory(
        const std::string& categoryId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::CategoryDAO> categoryDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_
    
    // EventBus is typically accessed as a singleton, not injected.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_CATEGORYSERVICE_H