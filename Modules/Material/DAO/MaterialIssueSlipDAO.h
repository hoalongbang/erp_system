// Modules/Material/DAO/MaterialIssueSlipDAO.h
#ifndef MODULES_MATERIAL_DAO_MATERIALISSUESLIPDAO_H
#define MODULES_MATERIAL_DAO_MATERIALISSUESLIPDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Material/DTO/MaterialIssueSlip.h" // For DTOs
#include "Modules/Material/DTO/MaterialIssueSlipDetail.h" // For DTOs
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "Modules/Utils/DTOUtils.h" // For DTO conversion helpers
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <any>
#include <optional>

namespace ERP {
namespace Material {
namespace DAOs {

// MaterialIssueSlipDAO will handle two DTOs (Slip and SlipDetail).
// It will inherit from DAOBase for MaterialIssueSlipDTO, and
// have specific methods for MaterialIssueSlipDetailDTO.

class MaterialIssueSlipDAO : public ERP::DAOBase::DAOBase<ERP::Material::DTO::MaterialIssueSlipDTO> {
public:
    explicit MaterialIssueSlipDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~MaterialIssueSlipDAO() override = default;

    // Override toMap and fromMap for MaterialIssueSlipDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Material::DTO::MaterialIssueSlipDTO& dto) const override;
    ERP::Material::DTO::MaterialIssueSlipDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for MaterialIssueSlipDetailDTO
    bool createMaterialIssueSlipDetail(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail);
    std::optional<ERP::Material::DTO::MaterialIssueSlipDetailDTO> getMaterialIssueSlipDetailById(const std::string& id);
    std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> getMaterialIssueSlipDetailsByIssueSlipId(const std::string& issueSlipId);
    bool updateMaterialIssueSlipDetail(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail);
    bool removeMaterialIssueSlipDetail(const std::string& id);
    bool removeMaterialIssueSlipDetailsByIssueSlipId(const std::string& issueSlipId); // Remove all details for a slip

    // Helpers for MaterialIssueSlipDetailDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& dto);
    static ERP::Material::DTO::MaterialIssueSlipDetailDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string materialIssueSlipDetailsTableName_ = "material_issue_slip_details"; // Table for details
};

} // namespace DAOs
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DAO_MATERIALISSUESLIPDAO_H