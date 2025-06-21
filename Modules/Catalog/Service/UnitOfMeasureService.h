// Modules/Catalog/Service/UnitOfMeasureService.h
#ifndef MODULES_CATALOG_SERVICE_UNITOFMEASURESERVICE_H
#define MODULES_CATALOG_SERVICE_UNITOFMEASURESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "UnitOfMeasure.h"    // Đã rút gọn include
#include "UnitOfMeasureDAO.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

namespace ERP {
namespace Catalog {
namespace Services {

/**
 * @brief IUnitOfMeasureService interface defines operations for managing units of measure.
 */
class IUnitOfMeasureService {
public:
    virtual ~IUnitOfMeasureService() = default;
    /**
     * @brief Creates a new unit of measure.
     * @param uomDTO DTO containing new unit of measure information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional UnitOfMeasureDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> createUnitOfMeasure(
        const ERP::Catalog::DTO::UnitOfMeasureDTO& uomDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves unit of measure information by ID.
     * @param uomId ID of the unit of measure to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional UnitOfMeasureDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> getUnitOfMeasureById(
        const std::string& uomId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves unit of measure information by name or symbol.
     * @param nameOrSymbol Name or symbol of the unit of measure to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional UnitOfMeasureDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> getUnitOfMeasureByNameOrSymbol(
        const std::string& nameOrSymbol,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all units of measure or UoMs matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching UnitOfMeasureDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> getAllUnitsOfMeasure(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates unit of measure information.
     * @param uomDTO DTO containing updated UoM information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateUnitOfMeasure(
        const ERP::Catalog::DTO::UnitOfMeasureDTO& uomDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a unit of measure.
     * @param uomId ID of the unit of measure.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateUnitOfMeasureStatus(
        const std::string& uomId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a unit of measure record by ID (soft delete).
     * @param uomId ID of the unit of measure to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteUnitOfMeasure(
        const std::string& uomId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of IUnitOfMeasureService.
 * This class uses UnitOfMeasureDAO and ISecurityManager.
 */
class UnitOfMeasureService : public IUnitOfMeasureService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for UnitOfMeasureService.
     * @param uomDAO Shared pointer to UnitOfMeasureDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    UnitOfMeasureService(std::shared_ptr<DAOs::UnitOfMeasureDAO> uomDAO,
                         std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                         std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                         std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                         std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> createUnitOfMeasure(
        const ERP::Catalog::DTO::UnitOfMeasureDTO& uomDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> getUnitOfMeasureById(
        const std::string& uomId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> getUnitOfMeasureByNameOrSymbol(
        const std::string& nameOrSymbol,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> getAllUnitsOfMeasure(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateUnitOfMeasure(
        const ERP::Catalog::DTO::UnitOfMeasureDTO& uomDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateUnitOfMeasureStatus(
        const std::string& uomId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteUnitOfMeasure(
        const std::string& uomId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::UnitOfMeasureDAO> uomDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_UNITOFMEASURESERVICE_H