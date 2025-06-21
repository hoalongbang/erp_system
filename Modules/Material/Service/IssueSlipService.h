// Modules/Material/Service/IssueSlipService.h
#ifndef MODULES_MATERIAL_SERVICE_ISSUESLIPSERVICE_H
#define MODULES_MATERIAL_SERVICE_ISSUESLIPSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "IssueSlip.h"          // Đã rút gọn include
#include "IssueSlipDetail.h"    // Đã rút gọn include
#include "IssueSlipDAO.h"       // Đã rút gọn include
#include "ProductService.h"     // For Product validation
#include "WarehouseService.h"   // For Warehouse/Location validation
#include "InventoryManagementService.h" // For Inventory operations
#include "MaterialRequestSlipService.h" // For MaterialRequestSlip linkage
#include "ISecurityManager.h"   // Đã rút gọn include
#include "EventBus.h"           // Đã rút gọn include
#include "Logger.h"             // Đã rút gọn include
#include "ErrorHandler.h"       // Đã rút gọn include
#include "Common.h"             // Đã rút gọn include
#include "Utils.h"              // Đã rút gọn include
#include "DateUtils.h"          // Đã rút gọn include

namespace ERP {
namespace Material {
namespace Services {

// Forward declare if other services are only used via pointer/reference
// class IProductService;
// class IWarehouseService;
// class IInventoryManagementService;
// class IMaterialRequestSlipService;

/**
 * @brief IIssueSlipService interface defines operations for managing material issue slips (phiếu xuất kho).
 * This is for general material issues, potentially for sales, and distinct from manufacturing material issues.
 */
class IIssueSlipService {
public:
    virtual ~IIssueSlipService() = default;
    /**
     * @brief Creates a new material issue slip.
     * @param issueSlipDTO DTO containing new issue slip information.
     * @param issueSlipDetails Vector of IssueSlipDetailDTOs for the slip.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional IssueSlipDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::IssueSlipDTO> createIssueSlip(
        const ERP::Material::DTO::IssueSlipDTO& issueSlipDTO,
        const std::vector<ERP::Material::DTO::IssueSlipDetailDTO>& issueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves issue slip information by ID.
     * @param issueSlipId ID of the issue slip to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional IssueSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::IssueSlipDTO> getIssueSlipById(
        const std::string& issueSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves issue slip information by issue number.
     * @param issueNumber Issue number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional IssueSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::IssueSlipDTO> getIssueSlipByNumber(
        const std::string& issueNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all issue slips or slips matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching IssueSlipDTOs.
     */
    virtual std::vector<ERP::Material::DTO::IssueSlipDTO> getAllIssueSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates issue slip information.
     * @param issueSlipDTO DTO containing updated issue slip information (must have ID).
     * @param issueSlipDetails Vector of IssueSlipDetailDTOs for the slip (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateIssueSlip(
        const ERP::Material::DTO::IssueSlipDTO& issueSlipDTO,
        const std::vector<ERP::Material::DTO::IssueSlipDetailDTO>& issueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of an issue slip.
     * @param issueSlipId ID of the issue slip.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateIssueSlipStatus(
        const std::string& issueSlipId,
        ERP::Material::DTO::IssueSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes an issue slip record by ID (soft delete).
     * @param issueSlipId ID of the issue slip to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteIssueSlip(
        const std::string& issueSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records actual issued quantity for a specific issue slip detail.
     * This also creates an inventory transaction.
     * @param detailId ID of the issue slip detail.
     * @param issuedQuantity Actual quantity issued.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if successful, false otherwise.
     */
    virtual bool recordIssuedQuantity(
        const std::string& detailId,
        double issuedQuantity,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all details for a specific issue slip.
     * @param issueSlipId ID of the issue slip.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of IssueSlipDetailDTOs.
     */
    virtual std::vector<ERP::Material::DTO::IssueSlipDetailDTO> getIssueSlipDetails(
        const std::string& issueSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};
/**
 * @brief Default implementation of IIssueSlipService.
 * This class uses IssueSlipDAO, and other services for validation and inventory operations.
 */
class IssueSlipService : public IIssueSlipService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for IssueSlipService.
     * @param issueSlipDAO Shared pointer to IssueSlipDAO.
     * @param productService Shared pointer to IProductService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param inventoryManagementService Shared pointer to IInventoryManagementService.
     * @param materialRequestSlipService Shared pointer to IMaterialRequestSlipService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    IssueSlipService(std::shared_ptr<DAOs::IssueSlipDAO> issueSlipDAO,
                     std::shared_ptr<ERP::Product::Services::IProductService> productService,
                     std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                     std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
                     std::shared_ptr<IMaterialRequestSlipService> materialRequestSlipService,
                     std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                     std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                     std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                     std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Material::DTO::IssueSlipDTO> createIssueSlip(
        const ERP::Material::DTO::IssueSlipDTO& issueSlipDTO,
        const std::vector<ERP::Material::DTO::IssueSlipDetailDTO>& issueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Material::DTO::IssueSlipDTO> getIssueSlipById(
        const std::string& issueSlipId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Material::DTO::IssueSlipDTO> getIssueSlipByNumber(
        const std::string& issueNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Material::DTO::IssueSlipDTO> getAllIssueSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateIssueSlip(
        const ERP::Material::DTO::IssueSlipDTO& issueSlipDTO,
        const std::vector<ERP::Material::DTO::IssueSlipDetailDTO>& issueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateIssueSlipStatus(
        const std::string& issueSlipId,
        ERP::Material::DTO::IssueSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteIssueSlip(
        const std::string& issueSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool recordIssuedQuantity(
        const std::string& detailId,
        double issuedQuantity,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Material::DTO::IssueSlipDetailDTO> getIssueSlipDetails(
        const std::string& issueSlipId,
        const std::vector<std::string>& userRoleIds = {}) override;

private:
    std::shared_ptr<DAOs::IssueSlipDAO> issueSlipDAO_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
    std::shared_ptr<IMaterialRequestSlipService> materialRequestSlipService_; // For updating MRS issued quantity
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_SERVICE_ISSUESLIPSERVICE_H