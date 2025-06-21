// Modules/Material/DAO/IssueSlipDetailDAO.h
#ifndef MODULES_MATERIAL_DAO_ISSUESLIPDETAILDAO_H
#define MODULES_MATERIAL_DAO_ISSUESLIPDETAILDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "IssueSlipDetail.h" // IssueSlipDetail DTO

namespace ERP {
    namespace Material {
        namespace DAOs {
            /**
             * @brief IssueSlipDetailDAO class provides data access operations for IssueSlipDetailDTO objects.
             * It inherits from DAOBase and interacts with the database to manage goods issue slip details.
             */
            class IssueSlipDetailDAO : public ERP::DAOBase::DAOBase<ERP::Material::DTO::IssueSlipDetailDTO> {
            public:
                IssueSlipDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~IssueSlipDetailDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Material::DTO::IssueSlipDetailDTO& detail) override;
                std::optional<ERP::Material::DTO::IssueSlipDetailDTO> findById(const std::string& id) override;
                bool update(const ERP::Material::DTO::IssueSlipDetailDTO& detail) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Material::DTO::IssueSlipDetailDTO> findAll() override;

                // Specific methods for IssueSlipDetail
                std::vector<ERP::Material::DTO::IssueSlipDetailDTO> getIssueSlipDetailsBySlipId(const std::string& issueSlipId);
                std::vector<ERP::Material::DTO::IssueSlipDetailDTO> getIssueSlipDetails(const std::map<std::string, std::any>& filters);
                int countIssueSlipDetails(const std::map<std::string, std::any>& filters);
                bool removeIssueSlipDetailsBySlipId(const std::string& issueSlipId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Material::DTO::IssueSlipDetailDTO& detail) const override;
                ERP::Material::DTO::IssueSlipDetailDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "issue_slip_details";
            };
        } // namespace DAOs
    } // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DAO_ISSUESLIPDETAILDAO_H