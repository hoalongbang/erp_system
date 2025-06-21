// Modules/Warehouse/DAO/InventoryDAO.h
#ifndef MODULES_WAREHOUSE_DAO_INVENTORYDAO_H
#define MODULES_WAREHOUSE_DAO_INVENTORYDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Inventory.h" // For DTOs (InventoryDTO) - Đã rút gọn include
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

class InventoryDAO : public ERP::DAOBase::DAOBase<ERP::Warehouse::DTO::InventoryDTO> {
public:
    explicit InventoryDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~InventoryDAO() override = default;

    // Override toMap and fromMap for InventoryDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Warehouse::DTO::InventoryDTO& dto) const override;
    ERP::Warehouse::DTO::InventoryDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};

} // namespace DAOs
} // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DAO_INVENTORYDAO_H