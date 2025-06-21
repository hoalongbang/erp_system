// Modules/Material/DAO/MaterialIssueSlipDetailDAO.h
#ifndef MODULES_MATERIAL_DAO_MATERIALISSUESLIPDETAILDAO_H
#define MODULES_MATERIAL_DAO_MATERIALISSUESLIPDETAILDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "MaterialIssueSlipDetail.h" // MaterialIssueSlipDetail DTO

namespace ERP {
    namespace Material {
        namespace DAOs {
            /**
             * @brief MaterialIssueSlipDetailDAO class provides data access operations for MaterialIssueSlipDetailDTO objects.
             * It inherits from DAOBase and interacts with the database to manage material issue slip details for manufacturing.
             */
            class MaterialIssueSlipDetailDAO : public ERP::DAOBase::DAOBase<ERP::Material::DTO::MaterialIssueSlipDetailDTO> {
            public:
                MaterialIssueSlipDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~MaterialIssueSlipDetailDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail) override;
                std::optional<ERP::Material::DTO::MaterialIssueSlipDetailDTO> findById(const std::string& id) override;
                bool update(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> findAll() override;

                // Specific methods for MaterialIssueSlipDetail
                std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> getMaterialIssueSlipDetailsBySlipId(const std::string& materialIssueSlipId);
                std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> getMaterialIssueSlipDetails(const std::map<std::string, std::any>& filters);
                int countMaterialIssueSlipDetails(const std::map<std::string, std::any>& filters);
                bool removeMaterialIssueSlipDetailsBySlipId(const std::string& materialIssueSlipId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail) const override;
                ERP::Material::DTO::MaterialIssueSlipDetailDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "material_issue_slip_details";
            };
        } // namespace DAOs
    } // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DAO_MATERIALISSUESLIPDETAILDAO_H