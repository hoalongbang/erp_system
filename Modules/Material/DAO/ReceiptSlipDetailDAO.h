// Modules/Material/DAO/ReceiptSlipDetailDAO.h
#ifndef MODULES_MATERIAL_DAO_RECEIPTSLIPDETAILDAO_H
#define MODULES_MATERIAL_DAO_RECEIPTSLIPDETAILDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "ReceiptSlipDetail.h" // ReceiptSlipDetail DTO

namespace ERP {
    namespace Material {
        namespace DAOs {
            /**
             * @brief ReceiptSlipDetailDAO class provides data access operations for ReceiptSlipDetailDTO objects.
             * It inherits from DAOBase and interacts with the database to manage goods receipt slip details.
             */
            class ReceiptSlipDetailDAO : public ERP::DAOBase::DAOBase<ERP::Material::DTO::ReceiptSlipDetailDTO> {
            public:
                ReceiptSlipDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~ReceiptSlipDetailDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) override;
                std::optional<ERP::Material::DTO::ReceiptSlipDetailDTO> findById(const std::string& id) override;
                bool update(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> findAll() override;

                // Specific methods for ReceiptSlipDetail
                std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> getReceiptSlipDetailsBySlipId(const std::string& receiptSlipId);
                std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> getReceiptSlipDetails(const std::map<std::string, std::any>& filters);
                int countReceiptSlipDetails(const std::map<std::string, std::any>& filters);
                bool removeReceiptSlipDetailsBySlipId(const std::string& receiptSlipId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) const override;
                ERP::Material::DTO::ReceiptSlipDetailDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "receipt_slip_details";
            };
        } // namespace DAOs
    } // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DAO_RECEIPTSLIPDETAILDAO_H