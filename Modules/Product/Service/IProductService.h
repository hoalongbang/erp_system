// Modules/Product/Service/IProductService.h
#ifndef MODULES_PRODUCT_SERVICE_IPRODUCTSERVICE_H
#define MODULES_PRODUCT_SERVICE_IPRODUCTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "BaseService.h"           // Base Service
#include "Product.h"               // DTO
#include "ProductUnitConversion.h" // DTO for unit conversions

namespace ERP {
    namespace Product {
        namespace Services {

            /**
             * @brief IProductService interface defines operations for managing products.
             */
            class IProductService {
            public:
                virtual ~IProductService() = default;
                /**
                 * @brief Creates a new product.
                 * @param productDTO DTO containing new product information.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional ProductDTO if creation is successful, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Product::DTO::ProductDTO> createProduct(
                    const ERP::Product::DTO::ProductDTO& productDTO,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves product information by ID.
                 * @param productId ID of the product to retrieve.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional ProductDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Product::DTO::ProductDTO> getProductById(
                    const std::string& productId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves product information by product code.
                 * @param productCode Product code to retrieve.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional ProductDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Product::DTO::ProductDTO> getProductByCode(
                    const std::string& productCode,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves all products or products matching a filter.
                 * @param filter Map of filter conditions.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return Vector of matching ProductDTOs.
                 */
                virtual std::vector<ERP::Product::DTO::ProductDTO> getAllProducts(
                    const std::map<std::string, std::any>& filter = {},
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Updates product information.
                 * @param productDTO DTO containing updated product information (must have ID).
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if update is successful, false otherwise.
                 */
                virtual bool updateProduct(
                    const ERP::Product::DTO::ProductDTO& productDTO,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Updates the status of a product.
                 * @param productId ID of the product.
                 * @param newStatus New status.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if status update is successful, false otherwise.
                 */
                virtual bool updateProductStatus(
                    const std::string& productId,
                    ERP::Common::EntityStatus newStatus,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Deletes a product record by ID (soft delete).
                 * @param productId ID of the product to delete.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if deletion is successful, false otherwise.
                 */
                virtual bool deleteProduct(
                    const std::string& productId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;

                // NEW: Product Unit Conversion Management
                /**
                 * @brief Creates a new product unit conversion rule.
                 * @param conversionDTO DTO containing new conversion rule information.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional ProductUnitConversionDTO if creation is successful, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Product::DTO::ProductUnitConversionDTO> createProductUnitConversion(
                    const ERP::Product::DTO::ProductUnitConversionDTO& conversionDTO,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves product unit conversion information by ID.
                 * @param conversionId ID of the conversion to retrieve.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional ProductUnitConversionDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::Product::DTO::ProductUnitConversionDTO> getProductUnitConversionById(
                    const std::string& conversionId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves all product unit conversions for a specific product.
                 * @param productId ID of the product.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return Vector of matching ProductUnitConversionDTOs.
                 */
                virtual std::vector<ERP::Product::DTO::ProductUnitConversionDTO> getAllProductUnitConversions(
                    const std::string& productId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Updates an existing product unit conversion rule.
                 * @param conversionDTO DTO containing updated conversion rule information (must have ID).
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if update is successful, false otherwise.
                 */
                virtual bool updateProductUnitConversion(
                    const ERP::Product::DTO::ProductUnitConversionDTO& conversionDTO,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Deletes a product unit conversion rule by ID.
                 * @param conversionId ID of the conversion to delete.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if deletion is successful, false otherwise.
                 */
                virtual bool deleteProductUnitConversion(
                    const std::string& conversionId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Gets the conversion factor between two units for a specific product.
                 * @param productId ID of the product.
                 * @param fromUnitId ID of the unit to convert from (e.g., base unit).
                 * @param toUnitId ID of the unit to convert to.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return The conversion factor, or 0.0 if not found or invalid (error logged internally).
                 */
                virtual double getConversionFactor(
                    const std::string& productId,
                    const std::string& fromUnitId,
                    const std::string& toUnitId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
            };

        } // namespace Services
    } // namespace Product
} // namespace ERP
#endif // MODULES_PRODUCT_SERVICE_IPRODUCTSERVICE_H