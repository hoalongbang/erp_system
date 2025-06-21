// Modules/Manufacturing/DAO/BillOfMaterialDAO.h
#ifndef MODULES_MANUFACTURING_DAO_BILLOFMATERIALDAO_H
#define MODULES_MANUFACTURING_DAO_BILLOFMATERIALDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Manufacturing/DTO/BillOfMaterial.h" // For DTOs
#include "Modules/Manufacturing/DTO/BillOfMaterialItem.h" // For DTOs
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

// BillOfMaterialDAO will handle two DTOs (BOM and BOM Item).
// It will inherit from DAOBase for BillOfMaterialDTO, and
// have specific methods for BillOfMaterialItemDTO.
// This is a common pattern when a DAO handles a "header-detail" relationship.

class BillOfMaterialDAO : public ERP::DAOBase::DAOBase<ERP::Manufacturing::DTO::BillOfMaterialDTO> {
public:
    explicit BillOfMaterialDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~BillOfMaterialDAO() override = default;

    // Override toMap and fromMap for BillOfMaterialDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Manufacturing::DTO::BillOfMaterialDTO& dto) const override;
    ERP::Manufacturing::DTO::BillOfMaterialDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for BillOfMaterialItemDTO
    bool createBomItem(const ERP::Manufacturing::DTO::BillOfMaterialItemDTO& item, const std::string& bomId);
    std::optional<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> getBomItemById(const std::string& id);
    std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> getBomItemsByBomId(const std::string& bomId);
    bool updateBomItem(const ERP::Manufacturing::DTO::BillOfMaterialItemDTO& item);
    bool removeBomItem(const std::string& id);
    bool removeBomItemsByBomId(const std::string& bomId); // Remove all items for a BOM

    // Helpers for BillOfMaterialItemDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Manufacturing::DTO::BillOfMaterialItemDTO& dto);
    static ERP::Manufacturing::DTO::BillOfMaterialItemDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string bomItemsTableName_ = "bill_of_material_items"; // Table for BOM items
};

} // namespace DAOs
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_DAO_BILLOFMATERIALDAO_H