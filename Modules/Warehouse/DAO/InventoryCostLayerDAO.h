// Modules/Warehouse/DAO/InventoryCostLayerDAO.h
#ifndef MODULES_WAREHOUSE_DAO_INVENTORYCOSTLAYERDAO_H
#define MODULES_WAREHOUSE_DAO_INVENTORYCOSTLAYERDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "InventoryCostLayer.h" // For DTOs (InventoryCostLayerDTO) - Đã rút gọn include
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

class InventoryCostLayerDAO : public ERP::DAOBase::DAOBase<ERP::Warehouse::DTO::InventoryCostLayerDTO> {
public:
    explicit InventoryCostLayerDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~InventoryCostLayerDAO() override = default;

    // Override toMap and fromMap for InventoryCostLayerDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Warehouse::DTO::InventoryCostLayerDTO& dto) const override;
    ERP::Warehouse::DTO::InventoryCostLayerDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};

} // namespace DAOs
} // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DAO_INVENTORYCOSTLAYERDAO_H