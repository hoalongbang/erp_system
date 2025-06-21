// Modules/Material/DAO/MaterialRequestSlipDAO.h
#ifndef MODULES_MATERIAL_DAO_MATERIALREQUESTSLIPDAO_H
#define MODULES_MATERIAL_DAO_MATERIALREQUESTSLIPDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Material/DTO/MaterialRequestSlip.h" // For DTOs
#include "Modules/Material/DTO/MaterialRequestSlipDetail.h" // For DTOs
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

// MaterialRequestSlipDAO will handle two DTOs (Slip and SlipDetail).
// It will inherit from DAOBase for MaterialRequestSlipDTO, and
// have specific methods for MaterialRequestSlipDetailDTO.

class MaterialRequestSlipDAO : public ERP::DAOBase::DAOBase<ERP::Material::DTO::MaterialRequestSlipDTO> {
public:
    explicit MaterialRequestSlipDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~MaterialRequestSlipDAO() override = default;

    // Override toMap and fromMap for MaterialRequestSlipDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Material::DTO::MaterialRequestSlipDTO& dto) const override;
    ERP::Material::DTO::MaterialRequestSlipDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for MaterialRequestSlipDetailDTO
    bool createMaterialRequestSlipDetail(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail);
    std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetailById(const std::string& id);
    std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetailsByRequestId(const std::string& requestId);
    bool updateMaterialRequestSlipDetail(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail);
    bool removeMaterialRequestSlipDetail(const std::string& id);
    bool removeMaterialRequestSlipDetailsByRequestId(const std::string& requestId); // Remove all details for a request

    // Helpers for MaterialRequestSlipDetailDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& dto);
    static ERP::Material::DTO::MaterialRequestSlipDetailDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string materialRequestSlipDetailsTableName_ = "material_request_slip_details"; // Table for details
};

} // namespace DAOs
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DAO_MATERIALREQUESTSLIPDAO_H