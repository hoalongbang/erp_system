// Modules/Product/Service/ProductService.cpp
#include "ProductService.h" // Standard includes
#include "Product.h"               // Product DTO
#include "ProductUnitConversion.h" // ProductUnitConversion DTO
#include "Category.h"              // Category DTO
#include "UnitOfMeasure.h"         // UnitOfMeasure DTO
#include "Event.h"                 // Event DTO
#include "ConnectionPool.h"        // ConnectionPool
#include "DBConnection.h"          // DBConnection
#include "Common.h"                // Common Enums/Constants
#include "Utils.h"                 // Utility functions
#include "DateUtils.h"             // Date utility functions
#include "AutoRelease.h"           // RAII helper
#include "ISecurityManager.h"      // Security Manager interface
#include "UserService.h"           // User Service (for audit logging)
#include "CategoryService.h"       // CategoryService (for category validation)
#include "UnitOfMeasureService.h"  // UnitOfMeasureService (for UoM validation)
#include "ProductUnitConversionDAO.h" // ProductUnitConversionDAO (for direct access)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
    namespace Product {
        namespace Services {

            ProductService::ProductService(
                std::shared_ptr<DAOs::ProductDAO> productDAO,
                std::shared_ptr<ERP::Catalog::Services::ICategoryService> categoryService,
                std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> unitOfMeasureService,
                std::shared_ptr<DAOs::ProductUnitConversionDAO> productUnitConversionDAO, // NEW
                std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
                : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
                productDAO_(productDAO),
                categoryService_(categoryService),
                unitOfMeasureService_(unitOfMeasureService),
                productUnitConversionDAO_(productUnitConversionDAO) { // NEW

                if (!productDAO_ || !categoryService_ || !unitOfMeasureService_ || !productUnitConversionDAO_ || !securityManager_) { // BaseService checks its own dependencies
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ProductService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ sản phẩm.");
                    ERP::Logger::Logger::getInstance().critical("ProductService: One or more injected DAOs/Services are null.");
                    throw std::runtime_error("ProductService: Null dependencies.");
                }
                ERP::Logger::Logger::getInstance().info("ProductService: Initialized.");
            }

            // Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

            std::optional<ERP::Product::DTO::ProductDTO> ProductService::createProduct(
                const ERP::Product::DTO::ProductDTO& productDTO,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("ProductService: Attempting to create product: " + productDTO.productCode + " - " + productDTO.name + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.CreateProduct", "Bạn không có quyền tạo sản phẩm.")) {
                    return std::nullopt;
                }

                // 1. Validate input DTO
                if (productDTO.productCode.empty() || productDTO.name.empty() || productDTO.categoryId.empty() || productDTO.baseUnitOfMeasureId.empty()) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Invalid input for product creation (missing code, name, category, or base UoM).");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ProductService: Invalid input for product creation.", "Thông tin sản phẩm không đầy đủ.");
                    return std::nullopt;
                }

                // Check if product code already exists
                std::map<std::string, std::any> filterByCode;
                filterByCode["product_code"] = productDTO.productCode;
                if (productDAO_->countProducts(filterByCode) > 0) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().warning("ProductService: Product with code " + productDTO.productCode + " already exists.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ProductService: Product with code " + productDTO.productCode + " already exists.", "Mã sản phẩm đã tồn tại. Vui lòng chọn mã khác.");
                    return std::nullopt;
                }

                // Validate Category existence
                std::optional<ERP::Catalog::DTO::CategoryDTO> category = categoryService_->getCategoryById(productDTO.categoryId, userRoleIds);
                if (!category || category->status != ERP::Common::EntityStatus::ACTIVE) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Invalid Category ID provided or category is not active: " + productDTO.categoryId);
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID danh mục không hợp lệ hoặc danh mục không hoạt động.");
                    return std::nullopt;
                }

                // Validate Base Unit of Measure existence
                std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> baseUoM = unitOfMeasureService_->getUnitOfMeasureById(productDTO.baseUnitOfMeasureId, userRoleIds);
                if (!baseUoM || baseUoM->status != ERP::Common::EntityStatus::ACTIVE) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Invalid Base Unit of Measure ID provided or UoM is not active: " + productDTO.baseUnitOfMeasureId);
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID đơn vị đo cơ sở không hợp lệ hoặc đơn vị đo không hoạt động.");
                    return std::nullopt;
                }

                ERP::Product::DTO::ProductDTO newProduct = productDTO;
                newProduct.id = ERP::Utils::generateUUID();
                newProduct.createdAt = ERP::Utils::DateUtils::now();
                newProduct.createdBy = currentUserId;
                newProduct.status = ERP::Common::EntityStatus::ACTIVE; // Default status

                std::optional<ERP::Product::DTO::ProductDTO> createdProduct = std::nullopt;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!productDAO_->create(newProduct)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("ProductService: Failed to create product in DAO.");
                            return false;
                        }
                        createdProduct = newProduct;
                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::ProductCreatedEvent>(newProduct.id, newProduct.name));
                        return true;
                    },
                    "ProductService", "createProduct"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("ProductService: Product " + newProduct.productCode + " created successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                        "Product", "Product", newProduct.id, "Product", newProduct.productCode,
                        std::nullopt, newProduct.toMap(), "Product created.");
                    return createdProduct;
                }
                return std::nullopt;
            }

            std::optional<ERP::Product::DTO::ProductDTO> ProductService::getProductById(
                const std::string& productId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("ProductService: Retrieving product by ID: " + productId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.ViewProducts", "Bạn không có quyền xem sản phẩm.")) {
                    return std::nullopt;
                }

                return productDAO_->getProductById(productId); // Specific DAO method
            }

            std::optional<ERP::Product::DTO::ProductDTO> ProductService::getProductByCode(
                const std::string& productCode,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("ProductService: Retrieving product by code: " + productCode + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.ViewProducts", "Bạn không có quyền xem sản phẩm.")) {
                    return std::nullopt;
                }

                return productDAO_->getProductByCode(productCode); // Specific DAO method
            }

            std::vector<ERP::Product::DTO::ProductDTO> ProductService::getAllProducts(
                const std::map<std::string, std::any>& filter,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("ProductService: Retrieving all products with filter.");

                if (!checkPermission(currentUserId, userRoleIds, "Product.ViewProducts", "Bạn không có quyền xem tất cả sản phẩm.")) {
                    return {};
                }

                return productDAO_->getProducts(filter); // Specific DAO method
            }

            bool ProductService::updateProduct(
                const ERP::Product::DTO::ProductDTO& productDTO,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("ProductService: Attempting to update product: " + productDTO.id + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.UpdateProduct", "Bạn không có quyền cập nhật sản phẩm.")) {
                    return false;
                }

                std::optional<ERP::Product::DTO::ProductDTO> oldProductOpt = productDAO_->getProductById(productDTO.id); // Specific DAO method
                if (!oldProductOpt) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Product with ID " + productDTO.id + " not found for update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy sản phẩm cần cập nhật.");
                    return false;
                }

                // If product code is changed, check for uniqueness
                if (productDTO.productCode != oldProductOpt->productCode) {
                    std::map<std::string, std::any> filterByCode;
                    filterByCode["product_code"] = productDTO.productCode;
                    if (productDAO_->countProducts(filterByCode) > 0) { // Specific DAO method
                        ERP::Logger::Logger::getInstance().warning("ProductService: New product code " + productDTO.productCode + " already exists.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ProductService: New product code " + productDTO.productCode + " already exists.", "Mã sản phẩm mới đã tồn tại. Vui lòng chọn mã khác.");
                        return false;
                    }
                }

                // Validate Category existence
                if (productDTO.categoryId != oldProductOpt->categoryId) { // Only check if changed
                    std::optional<ERP::Catalog::DTO::CategoryDTO> category = categoryService_->getCategoryById(productDTO.categoryId, userRoleIds);
                    if (!category || category->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("ProductService: Invalid Category ID provided for update or category is not active: " + productDTO.categoryId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID danh mục không hợp lệ hoặc danh mục không hoạt động.");
                        return false;
                    }
                }

                // Validate Base Unit of Measure existence
                if (productDTO.baseUnitOfMeasureId != oldProductOpt->baseUnitOfMeasureId) { // Only check if changed
                    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> baseUoM = unitOfMeasureService_->getUnitOfMeasureById(productDTO.baseUnitOfMeasureId, userRoleIds);
                    if (!baseUoM || baseUoM->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("ProductService: Invalid Base Unit of Measure ID provided for update or UoM is not active: " + productDTO.baseUnitOfMeasureId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID đơn vị đo cơ sở không hợp lệ hoặc đơn vị đo không hoạt động.");
                        return false;
                    }
                }

                ERP::Product::DTO::ProductDTO updatedProduct = productDTO;
                updatedProduct.updatedAt = ERP::Utils::DateUtils::now();
                updatedProduct.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!productDAO_->update(updatedProduct)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("ProductService: Failed to update product " + updatedProduct.id + " in DAO.");
                            return false;
                        }
                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::ProductUpdatedEvent>(updatedProduct.id, updatedProduct.name));
                        return true;
                    },
                    "ProductService", "updateProduct"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("ProductService: Product " + updatedProduct.id + " updated successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Product", "Product", updatedProduct.id, "Product", updatedProduct.productCode,
                        oldProductOpt->toMap(), updatedProduct.toMap(), "Product updated.");
                    return true;
                }
                return false;
            }

            bool ProductService::updateProductStatus(
                const std::string& productId,
                ERP::Common::EntityStatus newStatus,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("ProductService: Attempting to update status for product: " + productId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.UpdateProduct", "Bạn không có quyền cập nhật trạng thái sản phẩm.")) { // Reusing general update perm
                    return false;
                }

                std::optional<ERP::Product::DTO::ProductDTO> productOpt = productDAO_->getProductById(productId); // Specific DAO method
                if (!productOpt) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Product with ID " + productId + " not found for status update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy sản phẩm để cập nhật trạng thái.");
                    return false;
                }

                ERP::Product::DTO::ProductDTO oldProduct = *productOpt;
                if (oldProduct.status == newStatus) {
                    ERP::Logger::Logger::getInstance().info("ProductService: Product " + productId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
                    return true; // Already in desired status
                }

                // Add state transition validation logic here
                // For example: Cannot change from DELETED to ACTIVE.

                ERP::Product::DTO::ProductDTO updatedProduct = oldProduct;
                updatedProduct.status = newStatus;
                updatedProduct.updatedAt = ERP::Utils::DateUtils::now();
                updatedProduct.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!productDAO_->update(updatedProduct)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("ProductService: Failed to update status for product " + productId + " in DAO.");
                            return false;
                        }
                        // Optionally, publish event for status change
                        eventBus_.publish(std::make_shared<EventBus::ProductStatusChangedEvent>(productId, newStatus));
                        return true;
                    },
                    "ProductService", "updateProductStatus"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("ProductService: Status for product " + productId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Product", "ProductStatus", productId, "Product", oldProduct.productCode,
                        oldProduct.toMap(), updatedProduct.toMap(), "Product status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
                    return true;
                }
                return false;
            }

            bool ProductService::deleteProduct(
                const std::string& productId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("ProductService: Attempting to delete product: " + productId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.DeleteProduct", "Bạn không có quyền xóa sản phẩm.")) {
                    return false;
                }

                std::optional<ERP::Product::DTO::ProductDTO> productOpt = productDAO_->getProductById(productId); // Specific DAO method
                if (!productOpt) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Product with ID " + productId + " not found for deletion.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy sản phẩm cần xóa.");
                    return false;
                }

                ERP::Product::DTO::ProductDTO productToDelete = *productOpt;

                // Additional checks: Prevent deletion if product is in use (e.g., in inventory, sales orders, BOMs)
                // This requires cross-module service dependencies
                // For now, very basic check, more robust checks involve InventoryService, SalesOrderService, BillOfMaterialService
                // Example: if (securityManager_->getInventoryManagementService()->countInventoryByProduct(productId) > 0) ...
                // Example: if (securityManager_->getSalesOrderService()->countSalesOrdersByProduct(productId) > 0) ...
                // Example: if (securityManager_->getBillOfMaterialService()->countBOMsByProduct(productId) > 0) ...

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!productDAO_->remove(productId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("ProductService: Failed to delete product " + productId + " in DAO.");
                            return false;
                        }
                        // Optionally, delete related unit conversions
                        if (!productUnitConversionDAO_->removeByProductId(productId)) { // Assuming this method exists in ProductUnitConversionDAO
                            ERP::Logger::Logger::getInstance().error("ProductService: Failed to remove associated unit conversions for product " + productId + ".");
                            return false;
                        }
                        return true;
                    },
                    "ProductService", "deleteProduct"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("ProductService: Product " + productId + " deleted successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                        "Product", "Product", productId, "Product", productToDelete.productCode,
                        productToDelete.toMap(), std::nullopt, "Product deleted.");
                    return true;
                }
                return false;
            }


            // NEW: Product Unit Conversion Management Implementations

            std::optional<ERP::Product::DTO::ProductUnitConversionDTO> ProductService::createProductUnitConversion(
                const ERP::Product::DTO::ProductUnitConversionDTO& conversionDTO,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("ProductService: Attempting to create product unit conversion for product " + conversionDTO.productId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.CreateProductUnitConversion", "Bạn không có quyền tạo quy tắc chuyển đổi đơn vị sản phẩm.")) {
                    return std::nullopt;
                }

                // 1. Validate input DTO
                if (conversionDTO.productId.empty() || conversionDTO.fromUnitOfMeasureId.empty() || conversionDTO.toUnitOfMeasureId.empty() || conversionDTO.conversionFactor <= 0) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Invalid input for product unit conversion creation.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin chuyển đổi đơn vị không đầy đủ hoặc không hợp lệ.");
                    return std::nullopt;
                }
                if (conversionDTO.fromUnitOfMeasureId == conversionDTO.toUnitOfMeasureId) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Cannot create conversion from a unit to itself.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Không thể chuyển đổi từ một đơn vị sang chính nó.");
                    return std::nullopt;
                }

                // Validate Product existence
                std::optional<ERP::Product::DTO::ProductDTO> product = getProductById(conversionDTO.productId, userRoleIds);
                if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Product " + conversionDTO.productId + " not found or not active.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm không tồn tại hoặc không hoạt động.");
                    return std::nullopt;
                }
                // Ensure `fromUnitOfMeasureId` is the product's base UoM
                if (product->baseUnitOfMeasureId != conversionDTO.fromUnitOfMeasureId) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: FromUnitOfMeasureId " + conversionDTO.fromUnitOfMeasureId + " is not the product's base unit of measure.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Đơn vị gốc không phải là đơn vị cơ sở của sản phẩm.");
                    return std::nullopt;
                }

                // Validate UnitOfMeasure existence for toUnitId
                std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> toUom = unitOfMeasureService_->getUnitOfMeasureById(conversionDTO.toUnitOfMeasureId, userRoleIds);
                if (!toUom || toUom->status != ERP::Common::EntityStatus::ACTIVE) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: ToUnitOfMeasureId " + conversionDTO.toUnitOfMeasureId + " not found or not active.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Đơn vị đích không tồn tại hoặc không hoạt động.");
                    return std::nullopt;
                }

                // Check for duplicate conversion rule (Product, FromUnit, ToUnit)
                if (productUnitConversionDAO_->getConversion(conversionDTO.productId, conversionDTO.fromUnitOfMeasureId, conversionDTO.toUnitOfMeasureId)) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Duplicate conversion rule already exists for product " + conversionDTO.productId + " from " + conversionDTO.fromUnitOfMeasureId + " to " + conversionDTO.toUnitOfMeasureId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Quy tắc chuyển đổi đơn vị đã tồn tại.");
                    return std::nullopt;
                }

                ERP::Product::DTO::ProductUnitConversionDTO newConversion = conversionDTO;
                newConversion.id = ERP::Utils::generateUUID();
                newConversion.createdAt = ERP::Utils::DateUtils::now();
                newConversion.createdBy = currentUserId;
                newConversion.status = ERP::Common::EntityStatus::ACTIVE; // Default active

                std::optional<ERP::Product::DTO::ProductUnitConversionDTO> createdConversion = std::nullopt;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!productUnitConversionDAO_->create(newConversion)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("ProductService: Failed to create product unit conversion in DAO.");
                            return false;
                        }
                        createdConversion = newConversion;
                        return true;
                    },
                    "ProductService", "createProductUnitConversion"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("ProductService: Product unit conversion for product " + product->productCode + " created successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                        "Product", "UnitConversion", newConversion.id, "ProductUnitConversion", product->productCode + ":" + conversionDTO.fromUnitOfMeasureId + "->" + conversionDTO.toUnitOfMeasureId,
                        std::nullopt, newConversion.toMap(), "Product unit conversion created.");
                    return createdConversion;
                }
                return std::nullopt;
            }

            std::optional<ERP::Product::DTO::ProductUnitConversionDTO> ProductService::getProductUnitConversionById(
                const std::string& conversionId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("ProductService: Retrieving product unit conversion by ID: " + conversionId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.ViewProductUnitConversion", "Bạn không có quyền xem quy tắc chuyển đổi đơn vị sản phẩm.")) {
                    return std::nullopt;
                }

                return productUnitConversionDAO_->findById(conversionId); // Specific DAO method
            }

            std::vector<ERP::Product::DTO::ProductUnitConversionDTO> ProductService::getAllProductUnitConversions(
                const std::string& productId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("ProductService: Retrieving all product unit conversions for product ID: " + productId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.ViewProductUnitConversion", "Bạn không có quyền xem quy tắc chuyển đổi đơn vị sản phẩm.")) {
                    return {};
                }

                return productUnitConversionDAO_->getByProductId(productId); // Specific DAO method
            }

            bool ProductService::updateProductUnitConversion(
                const ERP::Product::DTO::ProductUnitConversionDTO& conversionDTO,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("ProductService: Attempting to update product unit conversion: " + conversionDTO.id + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.UpdateProductUnitConversion", "Bạn không có quyền cập nhật quy tắc chuyển đổi đơn vị sản phẩm.")) {
                    return false;
                }

                std::optional<ERP::Product::DTO::ProductUnitConversionDTO> oldConversionOpt = productUnitConversionDAO_->findById(conversionDTO.id); // Specific DAO method
                if (!oldConversionOpt) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Product unit conversion with ID " + conversionDTO.id + " not found for update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy quy tắc chuyển đổi đơn vị cần cập nhật.");
                    return false;
                }

                // Validate new product/units if they are changed
                if (conversionDTO.productId != oldConversionOpt->productId || conversionDTO.fromUnitOfMeasureId != oldConversionOpt->fromUnitOfMeasureId || conversionDTO.toUnitOfMeasureId != oldConversionOpt->toUnitOfMeasureId) {
                    // Validate Product existence
                    std::optional<ERP::Product::DTO::ProductDTO> product = getProductById(conversionDTO.productId, userRoleIds);
                    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("ProductService: Product " + conversionDTO.productId + " not found or not active for update.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Sản phẩm không tồn tại hoặc không hoạt động.");
                        return std::nullopt;
                    }
                    // Ensure `fromUnitOfMeasureId` is the product's base UoM
                    if (product->baseUnitOfMeasureId != conversionDTO.fromUnitOfMeasureId) {
                        ERP::Logger::Logger::getInstance().warning("ProductService: FromUnitOfMeasureId " + conversionDTO.fromUnitOfMeasureId + " is not the product's base unit of measure for update.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Đơn vị gốc không phải là đơn vị cơ sở của sản phẩm.");
                        return std::nullopt;
                    }
                    // Validate UnitOfMeasure existence for toUnitId
                    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> toUom = unitOfMeasureService_->getUnitOfMeasureById(conversionDTO.toUnitOfMeasureId, userRoleIds);
                    if (!toUom || toUom->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("ProductService: ToUnitOfMeasureId " + conversionDTO.toUnitOfMeasureId + " not found or not active for update.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Đơn vị đích không tồn tại hoặc không hoạt động.");
                        return std::nullopt;
                    }
                    // Check for duplicate conversion rule (Product, FromUnit, ToUnit) if key fields change
                    std::optional<ERP::Product::DTO::ProductUnitConversionDTO> duplicateCheck = productUnitConversionDAO_->getConversion(conversionDTO.productId, conversionDTO.fromUnitOfMeasureId, conversionDTO.toUnitOfMeasureId);
                    if (duplicateCheck && duplicateCheck->id != conversionDTO.id) { // Duplicate exists and is not current record
                        ERP::Logger::Logger::getInstance().warning("ProductService: Duplicate conversion rule already exists for product " + conversionDTO.productId + " from " + conversionDTO.fromUnitOfMeasureId + " to " + conversionDTO.toUnitOfMeasureId + " during update.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Quy tắc chuyển đổi đơn vị đã tồn tại.");
                        return false;
                    }
                }

                ERP::Product::DTO::ProductUnitConversionDTO updatedConversion = conversionDTO;
                updatedConversion.updatedAt = ERP::Utils::DateUtils::now();
                updatedConversion.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!productUnitConversionDAO_->update(updatedConversion)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("ProductService: Failed to update product unit conversion " + updatedConversion.id + " in DAO.");
                            return false;
                        }
                        return true;
                    },
                    "ProductService", "updateProductUnitConversion"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("ProductService: Product unit conversion " + updatedConversion.id + " updated successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Product", "UnitConversion", updatedConversion.id, "ProductUnitConversion",
                        oldConversionOpt->productId + ":" + oldConversionOpt->fromUnitOfMeasureId + "->" + oldConversionOpt->toUnitOfMeasureId,
                        oldConversionOpt->toMap(), updatedConversion.toMap(), "Product unit conversion updated.");
                    return true;
                }
                return false;
            }

            bool ProductService::deleteProductUnitConversion(
                const std::string& conversionId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("ProductService: Attempting to delete product unit conversion: " + conversionId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.DeleteProductUnitConversion", "Bạn không có quyền xóa quy tắc chuyển đổi đơn vị sản phẩm.")) {
                    return false;
                }

                std::optional<ERP::Product::DTO::ProductUnitConversionDTO> conversionOpt = productUnitConversionDAO_->findById(conversionId); // Specific DAO method
                if (!conversionOpt) {
                    ERP::Logger::Logger::getInstance().warning("ProductService: Product unit conversion with ID " + conversionId + " not found for deletion.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy quy tắc chuyển đổi đơn vị cần xóa.");
                    return false;
                }

                ERP::Product::DTO::ProductUnitConversionDTO conversionToDelete = *conversionOpt;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!productUnitConversionDAO_->remove(conversionId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("ProductService: Failed to delete product unit conversion " + conversionId + " in DAO.");
                            return false;
                        }
                        return true;
                    },
                    "ProductService", "deleteProductUnitConversion"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("ProductService: Product unit conversion " + conversionId + " deleted successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                        "Product", "UnitConversion", conversionId, "ProductUnitConversion",
                        conversionToDelete.productId + ":" + conversionToDelete.fromUnitOfMeasureId + "->" + conversionToDelete.toUnitOfMeasureId,
                        conversionToDelete.toMap(), std::nullopt, "Product unit conversion deleted.");
                    return true;
                }
                return false;
            }

            double ProductService::getConversionFactor(
                const std::string& productId,
                const std::string& fromUnitId,
                const std::string& toUnitId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("ProductService: Getting conversion factor for product " + productId + " from " + fromUnitId + " to " + toUnitId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Product.ViewProductUnitConversion", "Bạn không có quyền lấy hệ số chuyển đổi đơn vị sản phẩm.")) {
                    return 0.0;
                }

                if (fromUnitId == toUnitId) {
                    return 1.0; // Conversion to itself is 1:1
                }

                // Attempt to get direct conversion
                std::optional<ERP::Product::DTO::ProductUnitConversionDTO> conversionOpt = productUnitConversionDAO_->getConversion(productId, fromUnitId, toUnitId);
                if (conversionOpt) {
                    return conversionOpt->conversionFactor;
                }

                // If direct conversion not found, try inverse (from toUnit to fromUnit)
                std::optional<ERP::Product::DTO::ProductUnitConversionDTO> inverseConversionOpt = productUnitConversionDAO_->getConversion(productId, toUnitId, fromUnitId);
                if (inverseConversionOpt && inverseConversionOpt->conversionFactor != 0.0) {
                    return 1.0 / inverseConversionOpt->conversionFactor;
                }

                // If still not found, try to convert via base unit (assuming all conversions are from base unit)
                // First, check if fromUnitId is the base unit.
                std::optional<ERP::Product::DTO::ProductDTO> product = getProductById(productId, userRoleIds);
                if (!product) {
                    ERP::Logger::Logger::getInstance().error("ProductService: Product " + productId + " not found when calculating conversion factor.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm không tồn tại khi tính hệ số chuyển đổi.");
                    return 0.0;
                }

                // Case 1: fromUnit is base, toUnit is not
                if (fromUnitId == product->baseUnitOfMeasureId) {
                    // We already checked this with direct conversion, if it's not found, no direct path from base to toUnit
                    ERP::Logger::Logger::getInstance().warning("ProductService: No direct conversion found from base unit " + fromUnitId + " to " + toUnitId + " for product " + productId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy quy tắc chuyển đổi đơn vị.");
                    return 0.0;
                }
                // Case 2: toUnit is base, fromUnit is not
                else if (toUnitId == product->baseUnitOfMeasureId) {
                    // Need to convert from fromUnit to base unit. Find conversion from fromUnit to base.
                    std::optional<ERP::Product::DTO::ProductUnitConversionDTO> convFromToUnitToBase = productUnitConversionDAO_->getConversion(productId, fromUnitId, product->baseUnitOfMeasureId);
                    if (convFromToUnitToBase) {
                        return convFromToUnitToBase->conversionFactor; // Factor to convert from fromUnit to base
                    }
                    // Also check inverse from base to fromUnit
                    std::optional<ERP::Product::DTO::ProductUnitConversionDTO> convBaseToFromUnit = productUnitConversionDAO_->getConversion(productId, product->baseUnitOfMeasureId, fromUnitId);
                    if (convBaseToFromUnit && convBaseToFromUnit->conversionFactor != 0.0) {
                        return 1.0 / convBaseToFromUnit->conversionFactor; // Inverse of factor from base to fromUnit
                    }
                }
                // Case 3: Neither is base. Convert fromUnit to base, then base to toUnit.
                else {
                    double factorFromToBase = getConversionFactor(productId, fromUnitId, product->baseUnitOfMeasureId, userRoleIds);
                    double factorBaseToTo = getConversionFactor(productId, product->baseUnitOfMeasureId, toUnitId, userRoleIds);

                    // If either intermediate conversion fails (returns 0.0), then overall fails.
                    if (factorFromToBase != 0.0 && factorBaseToTo != 0.0) {
                        return factorFromToBase * factorBaseToTo;
                    }
                }


                ERP::Logger::Logger::getInstance().error("ProductService: No valid conversion path found for product " + productId + " from " + fromUnitId + " to " + toUnitId + ".");
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không tìm thấy đường dẫn chuyển đổi đơn vị hợp lệ.");
                return 0.0;
            }

        } // namespace Services
    } // namespace Product
} // namespace ERP