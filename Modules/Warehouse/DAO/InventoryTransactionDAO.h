// Modules/Warehouse/DAO/InventoryTransactionDAO.h
#ifndef MODULES_WAREHOUSE_DAO_INVENTORYTRANSACTIONDAO_H
#define MODULES_WAREHOUSE_DAO_INVENTORYTRANSACTIONDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "InventoryTransaction.h" // For DTOs (InventoryTransactionDTO) - Đã rút gọn include
#include "Logger.h" // Đã rút gọn include
#include "ErrorHandler.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "DTOUtils.h" // Đã rút gọn include
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <any>
#include <optional>

namespace ERP {
namespace Warehouse {
namespace DAOs {

class InventoryTransactionDAO : public ERP::DAOBase::DAOBase<ERP::Warehouse::DTO::InventoryTransactionDTO> {
public:
    explicit InventoryTransactionDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~InventoryTransactionDAO() override = default;

    // Override toMap and fromMap for InventoryTransactionDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Warehouse::DTO::InventoryTransactionDTO& dto) const override;
    ERP::Warehouse::DTO::InventoryTransactionDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};

} // namespace DAOs
} // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DAO_INVENTORYTRANSACTIONDAO_H