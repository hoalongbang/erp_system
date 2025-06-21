// Modules/Material/DAO/MaterialRequestSlipDetailDAO.h
#ifndef MODULES_MATERIAL_DAO_MATERIALREQUESTSLIPDETAILDAO_H
#define MODULES_MATERIAL_DAO_MATERIALREQUESTSLIPDETAILDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "MaterialRequestSlipDetail.h" // MaterialRequestSlipDetail DTO

namespace ERP {
    namespace Material {
        namespace DAOs {
            /**
             * @brief MaterialRequestSlipDetailDAO class provides data access operations for MaterialRequestSlipDetailDTO objects.
             * It inherits from DAOBase and interacts with the database to manage material request slip details.
             */
            class MaterialRequestSlipDetailDAO : public ERP::DAOBase::DAOBase<ERP::Material::DTO::MaterialRequestSlipDetailDTO> {
            public:
                MaterialRequestSlipDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~MaterialRequestSlipDetailDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail) override;
                std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> findById(const std::string& id) override;
                bool update(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> findAll() override;

                // Specific methods for MaterialRequestSlipDetail
                std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetailsBySlipId(const std::string& requestSlipId);
                std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetails(const std::map<std::string, std::any>& filters);
                int countMaterialRequestSlipDetails(const std::map<std::string, std::any>& filters);
                bool removeMaterialRequestSlipDetailsBySlipId(const std::string& requestSlipId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail) const override;
                ERP::Material::DTO::MaterialRequestSlipDetailDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "material_request_slip_details";
            };
        } // namespace DAOs
    } // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DAO_MATERIALREQUESTSLIPDETAILDAO_H