// Modules/Product/DAO/ProductUnitConversionDAO.h
#ifndef MODULES_PRODUCT_DAO_PRODUCTUNITCONVERSIONDAO_H
#define MODULES_PRODUCT_DAO_PRODUCTUNITCONVERSIONDAO_H

#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"                 // Base DAO template
#include "ProductUnitConversion.h"   // ProductUnitConversion DTO

namespace ERP {
    namespace Product {
        namespace DAOs {

            /**
             * @brief ProductUnitConversionDAO class provides data access operations for ProductUnitConversionDTO objects.
             * It inherits from DAOBase and interacts with the database to manage product unit conversions.
             */
            class ProductUnitConversionDAO : public ERP::DAOBase::DAOBase<ERP::Product::DTO::ProductUnitConversionDTO> {
            public:
                /**
                 * @brief Constructs a ProductUnitConversionDAO.
                 * @param connectionPool Shared pointer to the ConnectionPool.
                 */
                ProductUnitConversionDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);

                /**
                 * @brief Destructor.
                 */
                ~ProductUnitConversionDAO() override = default;

                // Override base methods
                bool save(const ERP::Product::DTO::ProductUnitConversionDTO& conversion) override;
                std::optional<ERP::Product::DTO::ProductUnitConversionDTO> findById(const std::string& id) override;
                bool update(const ERP::Product::DTO::ProductUnitConversionDTO& conversion) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Product::DTO::ProductUnitConversionDTO> findAll() override;

                // Specific methods for ProductUnitConversion
                /**
                 * @brief Retrieves product unit conversion records by product ID.
                 * @param productId ID of the product.
                 * @return A vector of ProductUnitConversionDTOs for the product.
                 */
                std::vector<ERP::Product::DTO::ProductUnitConversionDTO> getByProductId(const std::string& productId);

                /**
                 * @brief Retrieves a specific conversion record by product and unit IDs.
                 * @param productId ID of the product.
                 * @param fromUnitId ID of the from unit of measure.
                 * @param toUnitId ID of the to unit of measure.
                 * @return An optional ProductUnitConversionDTO, or std::nullopt if not found.
                 */
                std::optional<ERP::Product::DTO::ProductUnitConversionDTO> getConversion(
                    const std::string& productId,
                    const std::string& fromUnitId,
                    const std::string& toUnitId
                );

                /**
                 * @brief Counts the number of product unit conversion records matching a filter.
                 * @param filter A map representing the filter conditions.
                 * @return The number of matching records.
                 */
                int countConversions(const std::map<std::string, std::any>& filter = {});

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Product::DTO::ProductUnitConversionDTO& conversion) const override;
                ERP::Product::DTO::ProductUnitConversionDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "product_unit_conversions"; /**< Name of the database table. */
            };

        } // namespace DAOs
    } // namespace Product
} // namespace ERP

#endif // MODULES_PRODUCT_DAO_PRODUCTUNITCONVERSIONDAO_H