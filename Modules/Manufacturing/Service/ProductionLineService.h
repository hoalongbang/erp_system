// Modules/Manufacturing/Service/ProductionLineService.h
#ifndef MODULES_MANUFACTURING_SERVICE_PRODUCTIONLINESERVICE_H
#define MODULES_MANUFACTURING_SERVICE_PRODUCTIONLINESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"            // NEW: Kế thừa từ BaseService
#include "ProductionLine.h"         // Đã rút gọn include
#include "Location.h"               // Đã rút gọn include
#include "Asset.h"                  // Đã rút gọn include
#include "ProductionLineDAO.h"      // Đã rút gọn include
#include "ISecurityManager.h"       // Đã rút gọn include
#include "EventBus.h"               // Đã rút gọn include
#include "Logger.h"                 // Đã rút gọn include
#include "ErrorHandler.h"           // Đã rút gọn include
#include "Common.h"                 // Đã rút gọn include
#include "Utils.h"                  // Đã rút gọn include
#include "DateUtils.h"              // Đã rút gọn include

// Forward declarations for services that are dependencies but might cause circular includes
namespace ERP { namespace Catalog { namespace Services { class ILocationService; }}}
namespace ERP { namespace Asset { namespace Services { class IAssetManagementService; }}}

namespace ERP {
namespace Manufacturing {
namespace Services {

/**
 * @brief IProductionLineService interface defines operations for managing production lines.
 */
class IProductionLineService {
public:
    virtual ~IProductionLineService() = default;
    /**
     * @brief Creates a new production line.
     * @param productionLineDTO DTO containing new line information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionLineDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> createProductionLine(
        const ERP::Manufacturing::DTO::ProductionLineDTO& productionLineDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves production line information by ID.
     * @param lineId ID of the line to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionLineDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> getProductionLineById(
        const std::string& lineId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves production line information by line name.
     * @param lineName Name of the production line to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionLineDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> getProductionLineByName(
        const std::string& lineName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all production lines or lines matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ProductionLineDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::ProductionLineDTO> getAllProductionLines(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates production line information.
     * @param productionLineDTO DTO containing updated line information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateProductionLine(
        const ERP::Manufacturing::DTO::ProductionLineDTO& productionLineDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a production line.
     * @param lineId ID of the line.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateProductionLineStatus(
        const std::string& lineId,
        ERP::Manufacturing::DTO::ProductionLineStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a production line record by ID (soft delete).
     * @param lineId ID of the line to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteProductionLine(
        const std::string& lineId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of IProductionLineService.
 * This class uses ProductionLineDAO and ISecurityManager.
 */
class ProductionLineService : public IProductionLineService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for ProductionLineService.
     * @param productionLineDAO Shared pointer to ProductionLineDAO.
     * @param locationService Shared pointer to ILocationService (dependency).
     * @param assetManagementService Shared pointer to IAssetManagementService (dependency, can be nullptr for initial setup).
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    ProductionLineService(std::shared_ptr<DAOs::ProductionLineDAO> productionLineDAO,
                          std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService, // Dependency
                          std::shared_ptr<ERP::Asset::Services::IAssetManagementService> assetManagementService, // Dependency, can be nullptr
                          std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                          std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                          std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                          std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> createProductionLine(
        const ERP::Manufacturing::DTO::ProductionLineDTO& productionLineDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> getProductionLineById(
        const std::string& lineId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> getProductionLineByName(
        const std::string& lineName,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Manufacturing::DTO::ProductionLineDTO> getAllProductionLines(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateProductionLine(
        const ERP::Manufacturing::DTO::ProductionLineDTO& productionLineDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateProductionLineStatus(
        const std::string& lineId,
        ERP::Manufacturing::DTO::ProductionLineStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteProductionLine(
        const std::string& lineId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::ProductionLineDAO> productionLineDAO_;
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService_; // Dependency
    std::shared_ptr<ERP::Asset::Services::IAssetManagementService> assetManagementService_; // Dependency, can be nullptr
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_SERVICE_PRODUCTIONLINESERVICE_H