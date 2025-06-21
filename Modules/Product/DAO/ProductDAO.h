// Modules/Product/DAO/ProductDAO.h
#ifndef MODULES_PRODUCT_DAO_PRODUCTDAO_H
#define MODULES_PRODUCT_DAO_PRODUCTDAO_H
#include "DAOBase/DAOBase.h" // Include the new templated DAOBase
#include "Modules/Product/DTO/Product.h" // For ProductDTO
#include <memory>
#include <string>
#include <vector>
#include <optional>
// No longer need QJsonObject here

namespace ERP {
    namespace Product {
        namespace DAOs {
            /**
             * @brief DAO class for Product entity.
             * Handles database operations for ProductDTO.
             */
            class ProductDAO : public ERP::DAOBase::DAOBase<ERP::Product::DTO::ProductDTO> {
            public:
                /**
                 * @brief Constructs a ProductDAO.
                 * @param connectionPool Shared pointer to the ConnectionPool instance.
                 */
                explicit ProductDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~ProductDAO() override = default;

            protected:
                /**
                 * @brief Converts a ProductDTO object into a data map for database storage.
                 * This is an override of the pure virtual method in DAOBase.
                 * @param product ProductDTO object.
                 * @return Converted data map.
                 */
                std::map<std::string, std::any> toMap(const ERP::Product::DTO::ProductDTO& product) const override;
                /**
                 * @brief Converts a database data map into a ProductDTO object.
                 * This is an override of the pure virtual method in DAOBase.
                 * @param data Database data map.
                 * @return Converted ProductDTO object.
                 */
                ERP::Product::DTO::ProductDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                // tableName_ is now a member of DAOBase
            };
        } // namespace DAOs
    } // namespace Product
} // namespace ERP
#endif // MODULES_PRODUCT_DAO_PRODUCTDAO_H