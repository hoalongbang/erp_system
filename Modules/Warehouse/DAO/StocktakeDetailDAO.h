// Modules/Warehouse/DAO/StocktakeDetailDAO.h
#ifndef MODULES_WAREHOUSE_DAO_STOCKTAKEDETAILDAO_H
#define MODULES_WAREHOUSE_DAO_STOCKTAKEDETAILDAO_H

#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"            // Base DAO template
#include "StocktakeDetail.h"    // StocktakeDetail DTO

namespace ERP {
    namespace Warehouse {
        namespace DAOs {

            /**
             * @brief StocktakeDetailDAO class provides data access operations for StocktakeDetailDTO objects.
             * It inherits from DAOBase and interacts with the database to manage stocktake details.
             */
            class StocktakeDetailDAO : public ERP::DAOBase::DAOBase<ERP::Warehouse::DTO::StocktakeDetailDTO> {
            public:
                StocktakeDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~StocktakeDetailDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) override;
                std::optional<ERP::Warehouse::DTO::StocktakeDetailDTO> findById(const std::string& id) override;
                bool update(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> findAll() override;

                // Specific methods for StocktakeDetail
                /**
                 * @brief Retrieves stocktake detail records by stocktake request ID.
                 * @param stocktakeRequestId ID of the stocktake request.
                 * @return A vector of StocktakeDetailDTOs for the request.
                 */
                std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> getStocktakeDetailsByRequestId(const std::string& stocktakeRequestId);

                /**
                 * @brief Retrieves stocktake detail records based on a filter.
                 * @param filters A map representing the filter conditions.
                 * @return A vector of StocktakeDetailDTOs.
                 */
                std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> getStocktakeDetails(const std::map<std::string, std::any>& filters);

                /**
                 * @brief Counts the number of stocktake detail records matching a filter.
                 * @param filters A map representing the filter conditions.
                 * @return The number of matching records.
                 */
                int countStocktakeDetails(const std::map<std::string, std::any>& filters);

                /**
                 * @brief Removes stocktake detail records from the database based on stocktake request ID.
                 * @param stocktakeRequestId ID of the stocktake request.
                 * @return true if the records were deleted successfully, false otherwise.
                 */
                bool removeStocktakeDetailsByRequestId(const std::string& stocktakeRequestId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) const override;
                ERP::Warehouse::DTO::StocktakeDetailDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "stocktake_details";
            };

        } // namespace DAOs
    } // namespace Warehouse
} // namespace ERP

#endif // MODULES_WAREHOUSE_DAO_STOCKTAKEDETAILDAO_H