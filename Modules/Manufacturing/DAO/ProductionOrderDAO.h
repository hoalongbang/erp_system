// Modules/Manufacturing/DAO/ProductionOrderDAO.h
#ifndef MODULES_MANUFACTURING_DAO_PRODUCTIONORDERDAO_H
#define MODULES_MANUFACTURING_DAO_PRODUCTIONORDERDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"            // Base DAO template
#include "ProductionOrder.h"    // ProductionOrder DTO

namespace ERP {
namespace Manufacturing {
namespace DAOs {
/**
 * @brief DAO class for ProductionOrder entity.
 * Handles database operations for ProductionOrderDTO.
 */
class ProductionOrderDAO : public ERP::DAOBase::DAOBase<ERP::Manufacturing::DTO::ProductionOrderDTO> {
public:
    ProductionOrderDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~ProductionOrderDAO() override = default;

    // Override base methods (optional, but good practice if custom logic is needed)
    bool save(const ERP::Manufacturing::DTO::ProductionOrderDTO& order) override;
    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> findById(const std::string& id) override;
    bool update(const ERP::Manufacturing::DTO::ProductionOrderDTO& order) override;
    bool remove(const std::string& id) override;
    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> findAll() override;

    // Specific methods for ProductionOrder
    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrderByNumber(const std::string& orderNumber);
    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrders(const std::map<std::string, std::any>& filters);
    int countProductionOrders(const std::map<std::string, std::any>& filters);

protected:
    // Required overrides for mapping between DTO and std::map<string, any>
    std::map<std::string, std::any> toMap(const ERP::Manufacturing::DTO::ProductionOrderDTO& order) const override;
    ERP::Manufacturing::DTO::ProductionOrderDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    std::string tableName_ = "production_orders";
};
} // namespace DAOs
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_DAO_PRODUCTIONORDERDAO_H