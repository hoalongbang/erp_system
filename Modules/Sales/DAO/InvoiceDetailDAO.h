// Modules/Sales/DAO/InvoiceDetailDAO.h
#ifndef MODULES_SALES_DAO_INVOICEDETAILDAO_H
#define MODULES_SALES_DAO_INVOICEDETAILDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "InvoiceDetail.h"  // InvoiceDetail DTO

namespace ERP {
    namespace Sales {
        namespace DAOs {
            /**
             * @brief InvoiceDetailDAO class provides data access operations for InvoiceDetailDTO objects.
             * It inherits from DAOBase and interacts with the database to manage invoice details.
             */
            class InvoiceDetailDAO : public ERP::DAOBase::DAOBase<ERP::Sales::DTO::InvoiceDetailDTO> {
            public:
                InvoiceDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~InvoiceDetailDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Sales::DTO::InvoiceDetailDTO& detail) override;
                std::optional<ERP::Sales::DTO::InvoiceDetailDTO> findById(const std::string& id) override;
                bool update(const ERP::Sales::DTO::InvoiceDetailDTO& detail) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Sales::DTO::InvoiceDetailDTO> findAll() override;

                // Specific methods for InvoiceDetail
                std::vector<ERP::Sales::DTO::InvoiceDetailDTO> getInvoiceDetailsByInvoiceId(const std::string& invoiceId);
                std::vector<ERP::Sales::DTO::InvoiceDetailDTO> getInvoiceDetails(const std::map<std::string, std::any>& filters);
                int countInvoiceDetails(const std::map<std::string, std::any>& filters);
                bool removeInvoiceDetailsByInvoiceId(const std::string& invoiceId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Sales::DTO::InvoiceDetailDTO& detail) const override;
                ERP::Sales::DTO::InvoiceDetailDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "invoice_details";
            };
        } // namespace DAOs
    } // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DAO_INVOICEDETAILDAO_H