// Modules/Warehouse/DAO/StocktakeRequestDAO.h
#ifndef MODULES_WAREHOUSE_DAO_STOCKTAKEREQUESTDAO_H
#define MODULES_WAREHOUSE_DAO_STOCKTAKEREQUESTDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"            // Base DAO template
#include "StocktakeRequest.h"   // StocktakeRequest DTO
#include "StocktakeDetail.h"    // StocktakeDetail DTO (for related operations)

namespace ERP {
    namespace Warehouse {
        namespace DAOs {
            /**
             * @brief StocktakeRequestDAO class provides data access operations for StocktakeRequestDTO objects.
             * It inherits from DAOBase and interacts with the database to manage stocktake requests and their details.
             */
            class StocktakeRequestDAO : public ERP::DAOBase::DAOBase<ERP::Warehouse::DTO::StocktakeRequestDTO> {
            public:
                StocktakeRequestDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~StocktakeRequestDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Warehouse::DTO::StocktakeRequestDTO& request) override;
                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> findById(const std::string& id) override;
                bool update(const ERP::Warehouse::DTO::StocktakeRequestDTO& request) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> findAll() override;

                // Specific methods for StocktakeRequest
                std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> getStocktakeRequests(const std::map<std::string, std::any>& filters);
                int countStocktakeRequests(const std::map<std::string, std::any>& filters);

                // StocktakeDetail operations (nested/related entities)
                bool createStocktakeDetail(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail);
                std::optional<ERP::Warehouse::DTO::StocktakeDetailDTO> getStocktakeDetailById(const std::string& id);
                std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> getStocktakeDetailsByRequestId(const std::string& stocktakeRequestId);
                std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> getStocktakeDetails(const std::map<std::string, std::any>& filters);
                int countStocktakeDetails(const std::map<std::string, std::any>& filters);
                bool updateStocktakeDetail(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail);
                bool removeStocktakeDetail(const std::string& id);
                bool removeStocktakeDetailsByRequestId(const std::string& stocktakeRequestId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Warehouse::DTO::StocktakeRequestDTO& request) const override;
                ERP::Warehouse::DTO::StocktakeRequestDTO fromMap(const std::map<std::string, std::any>& data) const override;

                // Mapping for StocktakeDetailDTO (internal helper)
                std::map<std::string, std::any> stocktakeDetailToMap(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) const;
                ERP::Warehouse::DTO::StocktakeDetailDTO stocktakeDetailFromMap(const std::map<std::string, std::any>& data) const;

            private:
                std::string tableName_ = "stocktake_requests";
                std::string detailsTableName_ = "stocktake_details"; // Table for details
            };
        } // namespace DAOs
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DAO_STOCKTAKEREQUESTDAO_H