// Modules/Security/DAO/SessionDAO.h
#ifndef MODULES_SECURITY_DAO_SESSIONDAO_H
#define MODULES_SECURITY_DAO_SESSIONDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Security/DTO/Session.h" // For DTOs
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

class SessionDAO : public ERP::DAOBase::DAOBase<ERP::Security::DTO::SessionDTO> {
public:
    explicit SessionDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~SessionDAO() override = default;

    // Override toMap and fromMap for SessionDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Security::DTO::SessionDTO& dto) const override;
    ERP::Security::DTO::SessionDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};

} // namespace DAOs
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_DAO_SESSIONDAO_H