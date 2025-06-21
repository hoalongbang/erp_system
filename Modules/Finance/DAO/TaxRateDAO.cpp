// Modules/Finance/DAO/TaxRateDAO.cpp
#include "TaxRateDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DBConnection.h" // For raw DB connection

namespace ERP {
namespace Finance {
namespace DAOs {

TaxRateDAO::TaxRateDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Finance::DTO::TaxRateDTO>(connectionPool, "tax_rates") {
    // DAOBase constructor handles connectionPool and tableName_ initialization
    ERP::Logger::Logger::getInstance().info("TaxRateDAO: Initialized.");
}

std::map<std::string, std::any> TaxRateDAO::toMap(const ERP::Finance::DTO::TaxRateDTO& taxRate) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(taxRate); // BaseDTO fields

    data["name"] = taxRate.name;
    data["rate"] = taxRate.rate;
    ERP::DAOHelpers::putOptionalString(data, "description", taxRate.description);
    data["effective_date"] = ERP::Utils::DateUtils::formatDateTime(taxRate.effectiveDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalTime(data, "expiration_date", taxRate.expirationDate);

    return data;
}

ERP::Finance::DTO::TaxRateDTO TaxRateDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Finance::DTO::TaxRateDTO taxRate;
    ERP::Utils::DTOUtils::fromMap(data, taxRate); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "name", taxRate.name);
        ERP::DAOHelpers::getPlainValue(data, "rate", taxRate.rate);
        ERP::DAOHelpers::getOptionalStringValue(data, "description", taxRate.description);
        ERP::DAOHelpers::getPlainTimeValue(data, "effective_date", taxRate.effectiveDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "expiration_date", taxRate.expirationDate);
    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("TaxRateDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "TaxRateDAO: Data type mismatch in fromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("TaxRateDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "TaxRateDAO: Unexpected error in fromMap: " + std::string(e.what()));
    }
    return taxRate;
}

bool TaxRateDAO::save(const ERP::Finance::DTO::TaxRateDTO& taxRate) {
    // You can add pre-save logic here if needed.
    // For now, it directly calls the templated create method from DAOBase.
    return create(taxRate);
}

std::optional<ERP::Finance::DTO::TaxRateDTO> TaxRateDAO::findById(const std::string& id) {
    // Directly calls the templated getById method from DAOBase.
    return getById(id);
}

bool TaxRateDAO::update(const ERP::Finance::DTO::TaxRateDTO& taxRate) {
    // You can add pre-update logic here if needed.
    // For now, it directly calls the templated update method from DAOBase.
    return DAOBase<ERP::Finance::DTO::TaxRateDTO>::update(taxRate);
}

bool TaxRateDAO::remove(const std::string& id) {
    // You can add pre-remove logic here if needed.
    // For now, it directly calls the templated remove method from DAOBase.
    return DAOBase<ERP::Finance::DTO::TaxRateDTO>::remove(id);
}

std::vector<ERP::Finance::DTO::TaxRateDTO> TaxRateDAO::findAll() {
    // Directly calls the templated findAll method from DAOBase.
    return DAOBase<ERP::Finance::DTO::TaxRateDTO>::findAll();
}

std::optional<ERP::Finance::DTO::TaxRateDTO> TaxRateDAO::getTaxRateByName(const std::string& name) {
    std::map<std::string, std::any> filters;
    filters["name"] = name;
    std::vector<ERP::Finance::DTO::TaxRateDTO> results = get(filters); // Use templated get
    if (!results.empty()) {
        return results[0];
    }
    return std::nullopt;
}

std::vector<ERP::Finance::DTO::TaxRateDTO> TaxRateDAO::getTaxRates(const std::map<std::string, std::any>& filters) {
    return get(filters); // Use templated get
}

int TaxRateDAO::countTaxRates(const std::map<std::string, std::any>& filters) {
    return count(filters); // Use templated count
}

} // namespace DAOs
} // namespace Finance
} // namespace ERP