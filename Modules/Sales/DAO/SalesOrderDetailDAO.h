// Modules/Sales/DAO/SalesOrderDetailDAO.h
#ifndef MODULES_SALES_DAO_SALESORDERDETAILDAO_H
#define MODULES_SALES_DAO_SALESORDERDETAILDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "SalesOrderDetail.h" // SalesOrderDetail DTO

namespace ERP {
    namespace Sales {
        namespace DAOs {
            /**
             * @brief SalesOrderDetailDAO class provides data access operations for SalesOrderDetailDTO objects.
             * It inherits from DAOBase and interacts with the database to manage sales order details.
             */
            class SalesOrderDetailDAO : public ERP::DAOBase::DAOBase<ERP::Sales::DTO::SalesOrderDetailDTO> {
            public:
                SalesOrderDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~SalesOrderDetailDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Sales::DTO::SalesOrderDetailDTO& detail) override;
                std::optional<ERP::Sales::DTO::SalesOrderDetailDTO> findById(const std::string& id) override;
                bool update(const ERP::Sales::DTO::SalesOrderDetailDTO& detail) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> findAll() override;

                // Specific methods for SalesOrderDetail
                std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> getSalesOrderDetailsByOrderId(const std::string& salesOrderId);
                std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> getSalesOrderDetails(const std::map<std::string, std::any>& filters);
                int countSalesOrderDetails(const std::map<std::string, std::any>& filters);
                bool removeSalesOrderDetailsByOrderId(const std::string& salesOrderId);


            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Sales::DTO::SalesOrderDetailDTO& detail) const override;
                ERP::Sales::DTO::SalesOrderDetailDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "sales_order_details";
            };
        } // namespace DAOs
    } // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DAO_SALESORDERDETAILDAO_H