// Modules/Security/DAO/AuditLogDAO.h
#ifndef MODULES_SECURITY_DAO_AUDITLOGDAO_H
#define MODULES_SECURITY_DAO_AUDITLOGDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Security/DTO/AuditLog.h" // For DTOs
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
namespace Security {
namespace DAOs {

class AuditLogDAO : public ERP::DAOBase::DAOBase<ERP::Security::DTO::AuditLogDTO> {
public:
    explicit AuditLogDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~AuditLogDAO() override = default;

    // Override toMap and fromMap for AuditLogDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Security::DTO::AuditLogDTO& dto) const override;
    ERP::Security::DTO::AuditLogDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};

} // namespace DAOs
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_DAO_AUDITLOGDAO_H