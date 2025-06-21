// Modules/Finance/Service/TaxService.h
#ifndef MODULES_FINANCE_SERVICE_TAXSERVICE_H
#define MODULES_FINANCE_SERVICE_TAXSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "TaxRate.h"          // Đã rút gọn include
#include "TaxRateDAO.h"       // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

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
/**
 * @brief Default implementation of ITaxService.
 * This class uses TaxRateDAO and ISecurityManager.
 */
class TaxService : public ITaxService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for TaxService.
     * @param taxRateDAO Shared pointer to TaxRateDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    TaxService(std::shared_ptr<DAOs::TaxRateDAO> taxRateDAO,
               std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
               std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
               std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
               std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Finance::DTO::TaxRateDTO> createTaxRate(
        const ERP::Finance::DTO::TaxRateDTO& taxRateDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Finance::DTO::TaxRateDTO> getTaxRateById(
        const std::string& taxRateId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Finance::DTO::TaxRateDTO> getTaxRateByName(
        const std::string& taxRateName,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Finance::DTO::TaxRateDTO> getAllTaxRates(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateTaxRate(
        const ERP::Finance::DTO::TaxRateDTO& taxRateDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteTaxRate(
        const std::string& taxRateId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::TaxRateDAO> taxRateDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_SERVICE_TAXSERVICE_H