// Modules/Manufacturing/DAO/ProductionLineDAO.h
#ifndef MODULES_MANUFACTURING_DAO_PRODUCTIONLINEDAO_H
#define MODULES_MANUFACTURING_DAO_PRODUCTIONLINEDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Manufacturing/DTO/ProductionLine.h" // For DTOs
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
namespace Manufacturing {
namespace DAOs {

class ProductionLineDAO : public ERP::DAOBase::DAOBase<ERP::Manufacturing::DTO::ProductionLineDTO> {
public:
    explicit ProductionLineDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~ProductionLineDAO() override = default;

    // Override toMap and fromMap for ProductionLineDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Manufacturing::DTO::ProductionLineDTO& dto) const override;
    ERP::Manufacturing::DTO::ProductionLineDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};

} // namespace DAOs
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_DAO_PRODUCTIONLINEDAO_H