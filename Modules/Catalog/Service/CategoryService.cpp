// Modules/Catalog/Service/CategoryService.cpp
#include "CategoryService.h" // Đã rút gọn include
#include "Category.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Catalog {
namespace Services {

CategoryService::CategoryService(
    std::shared_ptr<DAOs::CategoryDAO> categoryDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      categoryDAO_(categoryDAO) {
    if (!categoryDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "CategoryService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ danh mục.");
        ERP::Logger::Logger::getInstance().critical("CategoryService: Injected CategoryDAO is null.");
        throw std::runtime_error("CategoryService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("CategoryService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Catalog::DTO::CategoryDTO> CategoryService::createCategory(
    const ERP::Catalog::DTO::CategoryDTO& categoryDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CategoryService: Attempting to create category: " + categoryDTO.name + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.CreateCategory", "Bạn không có quyền tạo danh mục.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (categoryDTO.name.empty()) {
        ERP::Logger::Logger::getInstance().warning("CategoryService: Invalid input for category creation (empty name).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "CategoryService: Invalid input for category creation.", "Tên danh mục không được để trống.");
        return std::nullopt;
    }

    // Check if category name already exists
    std::map<std::string, std::any> filterByName;
    filterByName["name"] = categoryDTO.name;
    if (categoryDAO_->count(filterByName) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("CategoryService: Category with name " + categoryDTO.name + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "CategoryService: Category with name " + categoryDTO.name + " already exists.", "Tên danh mục đã tồn tại. Vui lòng chọn tên khác.");
        return std::nullopt;
    }

    // Validate parent category existence if specified
    if (categoryDTO.parentCategoryId && !getCategoryById(*categoryDTO.parentCategoryId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("CategoryService: Parent category " + *categoryDTO.parentCategoryId + " not found for category creation.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Danh mục cha không tồn tại.");
        return std::nullopt;
    }

    ERP::Catalog::DTO::CategoryDTO newCategory = categoryDTO;
    newCategory.id = ERP::Utils::generateUUID();
    newCategory.createdAt = ERP::Utils::DateUtils::now();
    newCategory.createdBy = currentUserId;
    newCategory.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Catalog::DTO::CategoryDTO> createdCategory = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Note: DAO methods already acquire/release connections internally now.
            // The transaction context ensures atomicity across multiple DAO calls if needed.
            if (!categoryDAO_->create(newCategory)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("CategoryService: Failed to create category " + newCategory.name + " in DAO.");
                return false;
            }
            createdCategory = newCategory;
            eventBus_.publish(std::make_shared<EventBus::CategoryCreatedEvent>(newCategory.id, newCategory.name));
            return true;
        },
        "CategoryService", "createCategory"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("CategoryService: Category " + newCategory.name + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Category", newCategory.id, "Category", newCategory.name,
                       std::nullopt, newCategory.toMap(), "Category created.");
        return createdCategory;
    }
    return std::nullopt;
}

std::optional<ERP::Catalog::DTO::CategoryDTO> CategoryService::getCategoryById(
    const std::string& categoryId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("CategoryService: Retrieving category by ID: " + categoryId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewCategories", "Bạn không có quyền xem danh mục.")) {
        return std::nullopt;
    }

    return categoryDAO_->getById(categoryId); // Using getById from DAOBase template
}

std::optional<ERP::Catalog::DTO::CategoryDTO> CategoryService::getCategoryByName(
    const std::string& categoryName,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("CategoryService: Retrieving category by name: " + categoryName + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewCategories", "Bạn không có quyền xem danh mục.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["name"] = categoryName;
    std::vector<ERP::Catalog::DTO::CategoryDTO> categories = categoryDAO_->get(filter); // Using get from DAOBase template
    if (!categories.empty()) {
        return categories[0];
    }
    ERP::Logger::Logger::getInstance().debug("CategoryService: Category with name " + categoryName + " not found.");
    return std::nullopt;
}

std::vector<ERP::Catalog::DTO::CategoryDTO> CategoryService::getAllCategories(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CategoryService: Retrieving all categories with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewCategories", "Bạn không có quyền xem tất cả danh mục.")) {
        return {};
    }

    return categoryDAO_->get(filter); // Using get from DAOBase template
}

bool CategoryService::updateCategory(
    const ERP::Catalog::DTO::CategoryDTO& categoryDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CategoryService: Attempting to update category: " + categoryDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.UpdateCategory", "Bạn không có quyền cập nhật danh mục.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::CategoryDTO> oldCategoryOpt = categoryDAO_->getById(categoryDTO.id); // Using getById from DAOBase
    if (!oldCategoryOpt) {
        ERP::Logger::Logger::getInstance().warning("CategoryService: Category with ID " + categoryDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy danh mục cần cập nhật.");
        return false;
    }

    // If category name is changed, check for uniqueness
    if (categoryDTO.name != oldCategoryOpt->name) {
        std::map<std::string, std::any> filterByName;
        filterByName["name"] = categoryDTO.name;
        if (categoryDAO_->count(filterByName) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("CategoryService: New category name " + categoryDTO.name + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "CategoryService: New category name " + categoryDTO.name + " already exists.", "Tên danh mục mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }

    // Validate parent category existence if specified or changed
    if (categoryDTO.parentCategoryId && !getCategoryById(*categoryDTO.parentCategoryId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("CategoryService: Parent category " + *categoryDTO.parentCategoryId + " not found for category update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Danh mục cha không tồn tại.");
        return false;
    }
    // Prevent setting self as parent category
    if (categoryDTO.parentCategoryId && *categoryDTO.parentCategoryId == categoryDTO.id) {
        ERP::Logger::Logger::getInstance().warning("CategoryService: Cannot set category " + categoryDTO.id + " as its own parent.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Không thể đặt danh mục làm danh mục cha của chính nó.");
        return false;
    }

    ERP::Catalog::DTO::CategoryDTO updatedCategory = categoryDTO;
    updatedCategory.updatedAt = ERP::Utils::DateUtils::now();
    updatedCategory.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!categoryDAO_->update(updatedCategory)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("CategoryService: Failed to update category " + updatedCategory.id + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::CategoryUpdatedEvent>(updatedCategory.id, updatedCategory.name));
            return true;
        },
        "CategoryService", "updateCategory"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("CategoryService: Category " + updatedCategory.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Category", updatedCategory.id, "Category", updatedCategory.name,
                       oldCategoryOpt->toMap(), updatedCategory.toMap(), "Category updated.");
        return true;
    }
    return false;
}

bool CategoryService::updateCategoryStatus(
    const std::string& categoryId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CategoryService: Attempting to update status for category: " + categoryId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ChangeCategoryStatus", "Bạn không có quyền cập nhật trạng thái danh mục.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::CategoryDTO> categoryOpt = categoryDAO_->getById(categoryId); // Using getById from DAOBase
    if (!categoryOpt) {
        ERP::Logger::Logger::getInstance().warning("CategoryService: Category with ID " + categoryId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy danh mục để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Catalog::DTO::CategoryDTO oldCategory = *categoryOpt;
    if (oldCategory.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("CategoryService: Category " + categoryId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Catalog::DTO::CategoryDTO updatedCategory = oldCategory;
    updatedCategory.status = newStatus;
    updatedCategory.updatedAt = ERP::Utils::DateUtils::now();
    updatedCategory.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!categoryDAO_->update(updatedCategory)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("CategoryService: Failed to update status for category " + categoryId + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::CategoryStatusChangedEvent>(categoryId, newStatus));
            return true;
        },
        "CategoryService", "updateCategoryStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("CategoryService: Status for category " + categoryId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "CategoryStatus", categoryId, "Category", oldCategory.name,
                       oldCategory.toMap(), updatedCategory.toMap(), "Category status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool CategoryService::deleteCategory(
    const std::string& categoryId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("CategoryService: Attempting to delete category: " + categoryId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.DeleteCategory", "Bạn không có quyền xóa danh mục.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::CategoryDTO> categoryOpt = categoryDAO_->getById(categoryId); // Using getById from DAOBase
    if (!categoryOpt) {
        ERP::Logger::Logger::getInstance().warning("CategoryService: Category with ID " + categoryId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy danh mục cần xóa.");
        return false;
    }

    ERP::Catalog::DTO::CategoryDTO categoryToDelete = *categoryOpt;

    // Additional checks: Prevent deletion if category has associated products or subcategories
    // This would require dependencies on ProductService or CategoryService itself for checking subcategories
    std::map<std::string, std::any> productFilter;
    productFilter["category_id"] = categoryId;
    if (securityManager_->getProductService()->getAllProducts(productFilter).size() > 0) { // Assuming ProductService can query by category
        ERP::Logger::Logger::getInstance().warning("CategoryService: Cannot delete category " + categoryId + " as it has associated products.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa danh mục có sản phẩm liên quan.");
        return false;
    }
    std::map<std::string, std::any> subCategoryFilter;
    subCategoryFilter["parent_id"] = categoryId;
    if (categoryDAO_->count(subCategoryFilter) > 0) { // Using count from DAOBase
        ERP::Logger::Logger::getInstance().warning("CategoryService: Cannot delete category " + categoryId + " as it has subcategories.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa danh mục có danh mục con.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!categoryDAO_->remove(categoryId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("CategoryService: Failed to delete category " + categoryId + " in DAO.");
                return false;
            }
            return true;
        },
        "CategoryService", "deleteCategory"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("CategoryService: Category " + categoryId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "Category", categoryId, "Category", categoryToDelete.name,
                       categoryToDelete.toMap(), std::nullopt, "Category deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Catalog
} // namespace ERP