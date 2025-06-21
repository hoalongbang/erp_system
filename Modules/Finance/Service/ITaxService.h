// Modules/Finance/Service/ITaxService.h
#ifndef MODULES_FINANCE_SERVICE_ITAXSERVICE_H
#define MODULES_FINANCE_SERVICE_ITAXSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "TaxRate.h"       // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Finance {
namespace Services {

/**
 * @brief ITaxService interface defines operations for managing tax rates.
 */
class ITaxService {
public:
    virtual ~ITaxService() = default;
    /**
     * @brief Creates a new tax rate.
     * @param taxRateDTO DTO containing new tax rate information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional TaxRateDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::TaxRateDTO> createTaxRate(
        const ERP::Finance::DTO::TaxRateDTO& taxRateDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves tax rate information by ID.
     * @param taxRateId ID of the tax rate to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional TaxRateDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::TaxRateDTO> getTaxRateById(
        const std::string& taxRateId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves tax rate information by name.
     * @param taxRateName Name of the tax rate to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional TaxRateDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::TaxRateDTO> getTaxRateByName(
        const std::string& taxRateName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all tax rates or tax rates matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching TaxRateDTOs.
     */
    virtual std::vector<ERP::Finance::DTO::TaxRateDTO> getAllTaxRates(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates tax rate information.
     * @param taxRateDTO DTO containing updated tax rate information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateTaxRate(
        const ERP::Finance::DTO::TaxRateDTO& taxRateDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a tax rate record by ID (soft delete).
     * @param taxRateId ID of the tax rate to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteTaxRate(
        const std::string& taxRateId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_SERVICE_ITAXSERVICE_H