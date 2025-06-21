// Modules/Material/DAO/IssueSlipDAO.h
#ifndef MODULES_MATERIAL_DAO_ISSUESLIPDAO_H
#define MODULES_MATERIAL_DAO_ISSUESLIPDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Material/DTO/IssueSlip.h" // For DTOs
#include "Modules/Material/DTO/IssueSlipDetail.h" // For DTOs
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

// IssueSlipDAO handles two DTOs (IssueSlip and IssueSlipDetail).
// It will inherit from DAOBase for IssueSlipDTO, and
// have specific methods for IssueSlipDetailDTO.

class IssueSlipDAO : public ERP::DAOBase::DAOBase<ERP::Material::DTO::IssueSlipDTO> {
public:
    explicit IssueSlipDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~IssueSlipDAO() override = default;

    // Override toMap and fromMap for IssueSlipDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Material::DTO::IssueSlipDTO& dto) const override;
    ERP::Material::DTO::IssueSlipDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for IssueSlipDetailDTO
    bool createIssueSlipDetail(const ERP::Material::DTO::IssueSlipDetailDTO& detail);
    std::optional<ERP::Material::DTO::IssueSlipDetailDTO> getIssueSlipDetailById(const std::string& id);
    std::vector<ERP::Material::DTO::IssueSlipDetailDTO> getIssueSlipDetailsByIssueSlipId(const std::string& issueSlipId);
    bool updateIssueSlipDetail(const ERP::Material::DTO::IssueSlipDetailDTO& detail);
    bool removeIssueSlipDetail(const std::string& id);
    bool removeIssueSlipDetailsByIssueSlipId(const std::string& issueSlipId); // Remove all details for a slip

    // Helpers for IssueSlipDetailDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Material::DTO::IssueSlipDetailDTO& dto);
    static ERP::Material::DTO::IssueSlipDetailDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string issueSlipDetailsTableName_ = "issue_slip_details"; // Table for issue slip details
};

} // namespace DAOs
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DAO_ISSUESLIPDAO_H