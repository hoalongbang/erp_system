// Modules/Finance/DAO/TaxRateDAO.h
#ifndef MODULES_FINANCE_DAO_TAXRATEDAO_H
#define MODULES_FINANCE_DAO_TAXRATEDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "TaxRate.h"        // TaxRate DTO

namespace ERP {
namespace Finance {
namespace DAOs {
/**
 * @brief DAO class for TaxRate entity.
 * Handles database operations for TaxRateDTO.
 */
class TaxRateDAO : public ERP::DAOBase::DAOBase<ERP::Finance::DTO::TaxRateDTO> {
public:
    TaxRateDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~TaxRateDAO() override = default;

    // Override base methods (optional, but good practice if custom logic is needed)
    bool save(const ERP::Finance::DTO::TaxRateDTO& taxRate) override;
    std::optional<ERP::Finance::DTO::TaxRateDTO> findById(const std::string& id) override;
    bool update(const ERP::Finance::DTO::TaxRateDTO& taxRate) override;
    bool remove(const std::string& id) override;
    std::vector<ERP::Finance::DTO::TaxRateDTO> findAll() override;

    // Specific methods for TaxRate if needed (e.g., find by name, get active rates)
    std::optional<ERP::Finance::DTO::TaxRateDTO> getTaxRateByName(const std::string& name);
    std::vector<ERP::Finance::DTO::TaxRateDTO> getTaxRates(const std::map<std::string, std::any>& filters);
    int countTaxRates(const std::map<std::string, std::any>& filters);

protected:
    // Required overrides for mapping between DTO and std::map<string, any>
    std::map<std::string, std::any> toMap(const ERP::Finance::DTO::TaxRateDTO& taxRate) const override;
    ERP::Finance::DTO::TaxRateDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    std::string tableName_ = "tax_rates";
};
} // namespace DAOs
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DAO_TAXRATEDAO_H